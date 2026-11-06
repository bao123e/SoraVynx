"""Summarize only game-related DNS and connection outcomes from test-emulator traffic.

Does not export packet bodies, SDK configuration, cookies or authorization headers.
The raw packet capture stays in the disposable runner's temporary directory.
"""
import json
import socket
import sys
from collections import Counter, defaultdict
from pathlib import Path
import dpkt

capture = Path(sys.argv[1])
output = Path(sys.argv[2])
names = defaultdict(set)
connections = defaultdict(Counter)
flows = defaultdict(Counter)
output.parent.mkdir(parents=True, exist_ok=True)
if not capture.exists():
    output.write_text(json.dumps({'capture_available': False}))
    raise SystemExit(0)
with capture.open('rb') as stream:
    packets = dpkt.pcap.Reader(stream)
    while True:
        try:
            timestamp, packet = next(packets)
        except (StopIteration, dpkt.dpkt.NeedData, ValueError):
            break
        try:
            frame = dpkt.ethernet.Ethernet(packet)
            ip = frame.data
            if not isinstance(ip, (dpkt.ip.IP, dpkt.ip6.IP6)):
                continue
            family = socket.AF_INET if isinstance(ip, dpkt.ip.IP) else socket.AF_INET6
            source, destination = socket.inet_ntop(family, ip.src), socket.inet_ntop(family, ip.dst)
            layer = ip.data
            if isinstance(layer, dpkt.udp.UDP) and layer.sport == 53:
                response = dpkt.dns.DNS(layer.data)
                relevant = [q.name for q in response.qd if 'stellasora' in q.name.lower() or 'yostar' in q.name.lower()]
                for answer in response.an:
                    if answer.type in (dpkt.dns.DNS_A, dpkt.dns.DNS_AAAA):
                        address = socket.inet_ntop(socket.AF_INET if answer.type == dpkt.dns.DNS_A else socket.AF_INET6, answer.rdata)
                        for name in relevant:
                            names[address].add(name)
            if isinstance(layer, dpkt.tcp.TCP):
                outgoing = layer.dport in (80, 443)
                incoming = layer.sport in (80, 443)
                if not outgoing and not incoming:
                    continue
                remote = destination if outgoing else source
                counters = connections[remote]
                client_port = layer.sport if outgoing else layer.dport
                flow = flows[(remote, client_port)]
                if not flow.get('first_seen_unix'): flow['first_seen_unix'] = timestamp
                flow['last_seen_unix'] = timestamp
                flow['received_bytes' if incoming else 'sent_bytes'] += len(layer.data)
                if layer.flags & dpkt.tcp.TH_SYN: flow['syn_ack' if layer.flags & dpkt.tcp.TH_ACK else 'syn'] += 1
                if layer.flags & dpkt.tcp.TH_RST: flow['reset'] += 1
                if layer.flags & dpkt.tcp.TH_SYN:
                    counters['syn_ack' if layer.flags & dpkt.tcp.TH_ACK else 'syn'] += 1
                if layer.flags & dpkt.tcp.TH_RST:
                    counters['reset'] += 1
                counters['received_bytes' if incoming else 'sent_bytes'] += len(layer.data)
                if layer.data and layer.data[0] == 21:
                    counters['possible_tls_alert_records'] += 1
        except (dpkt.dpkt.Error, ValueError, OSError):
            continue
result = {'capture_available': True, 'game_hosts': [
    {'address': address, 'names': sorted(hosts), 'traffic': dict(connections[address]),
     'flows': [dict(client_port=port, **stats) for (remote, port), stats in flows.items() if remote == address]}
    for address, hosts in sorted(names.items())
]}
output.write_text(json.dumps(result, indent=2))
print(json.dumps(result, indent=2))
