"""Loopback-only CONNECT relay for disposable emulator tests.

Uses the runner's public IPv4 connectivity. TLS remains end-to-end: this proxy
does not decrypt TLS, install certificates, change application payloads or log request bodies.
Only host names and connection outcomes enter the diagnostic file.
"""
import http.server
import ipaddress
import json
import os
import select
import socket
import socketserver
import sys
import threading
import time
from pathlib import Path
from urllib.parse import urlsplit

events = Path(sys.argv[1])
lock = threading.Lock()


def record(host, port, status):
    item = {'host': host, 'port': port, 'status': status, 'time_unix': time.time()}
    with lock:
        with events.open('a') as stream:
            stream.write(json.dumps(item) + '\n')


def connect(host, port):
    if port not in (80, 443):
        raise OSError('Only standard public HTTP/S ports allowed')
    addresses = socket.getaddrinfo(host, port, socket.AF_INET, socket.SOCK_STREAM)
    for _, _, _, _, address in addresses:
        if not ipaddress.ip_address(address[0]).is_global:
            continue
        try:
            channel = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            channel.settimeout(12)
            channel.connect(address)
            return channel
        except OSError:
            channel.close()
    raise OSError('Public IPv4 connection unavailable')


def relay(left, right):
    left.setblocking(False)
    right.setblocking(False)
    while True:
        readable, _, errors = select.select([left, right], [], [left, right], 30)
        if errors or not readable:
            return
        for source in readable:
            data = source.recv(65536)
            if not data:
                return
            target = right if source is left else left
            target.setblocking(True)
            target.settimeout(15)
            target.sendall(data)
            target.setblocking(False)


class Proxy(http.server.BaseHTTPRequestHandler):
    protocol_version = 'HTTP/1.1'

    def log_message(self, *args):
        pass

    def do_CONNECT(self):
        host, _, value = self.path.rpartition(':')
        try:
            port = int(value)
            with connect(host, port) as remote:
                record(host, port, 'connected')
                self.send_response(200, 'Connection established')
                self.end_headers()
                self.wfile.flush()
                relay(self.connection, remote)
        except (OSError, ValueError):
            record(host, int(value) if value.isdigit() else 0, 'connection_failed_or_closed')
            self.close_connection = True
        finally:
            self.close_connection = True

    def forward(self):
        parsed = urlsplit(self.path)
        host, port = parsed.hostname, parsed.port or 80
        if parsed.scheme != 'http' or not host:
            self.send_error(400)
            return
        try:
            with connect(host, port) as remote:
                record(host, port, 'connected')
                path = (parsed.path or '/') + (('?' + parsed.query) if parsed.query else '')
                headers = ''.join(f'{k}: {v}\r\n' for k, v in self.headers.items()
                                  if k.lower() not in ('proxy-connection', 'proxy-authorization', 'connection'))
                remote.sendall((f'{self.command} {path} HTTP/1.1\r\n' + headers + 'Connection: close\r\n\r\n').encode('latin-1'))
                length = int(self.headers.get('Content-Length', '0'))
                if length > 8 * 1024 * 1024:
                    self.send_error(413)
                    return
                if length:
                    remote.sendall(self.rfile.read(length))
                self.wfile.flush()
                relay(self.connection, remote)
        except (OSError, ValueError):
            record(host, port, 'connection_failed_or_closed')
        self.close_connection = True

    do_GET = forward
    do_HEAD = forward
    do_POST = forward


class Server(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True


if __name__ == '__main__':
    events.parent.mkdir(parents=True, exist_ok=True)
    descriptor = os.open(events, os.O_WRONLY | os.O_CREAT | os.O_APPEND, 0o600)
    os.close(descriptor)
    Server(('127.0.0.1', 8765), Proxy).serve_forever()
