#!/usr/bin/env python3
"""Compare stock and released game startup on a fresh, disposable emulator.

Does not log in, claim rewards, enable cheats or click resource/download prompts.
Diagnostic output belongs to CI, not to either production APK.
"""
import json
import os
import re
from startup_health import healthy_start
import subprocess
import time
from pathlib import Path

OUT = Path('startup-results')
OUT.mkdir(exist_ok=True)
GAME = 'com.YoStarEN.StellaSora'
COMPONENT = GAME + '/com.yostar.sdk.bridge.YostarSDKPlayerAct'


def adb(*args, timeout=30):
    result = subprocess.run(['adb', *args], capture_output=True, timeout=timeout)
    return {'code': result.returncode, 'stdout': result.stdout.decode(errors='replace'),
            'stderr': result.stderr.decode(errors='replace')}


def screen(name):
    result = subprocess.run(['adb', 'exec-out', 'screencap', '-p'], capture_output=True, timeout=20)
    if result.returncode == 0 and result.stdout.startswith(b'\x89PNG'):
        (OUT / (name + '.png')).write_bytes(result.stdout)
    adb('shell', 'uiautomator', 'dump', '/sdcard/startup.xml', timeout=15)
    xml = adb('exec-out', 'cat', '/sdcard/startup.xml')['stdout']
    (OUT / (name + '.xml')).write_text(xml)


def launch(name):
    adb('logcat', '-c')
    started = adb('shell', 'am', 'start', '-W', '-n', COMPONENT, timeout=60)
    (OUT / (name + '-launch.json')).write_text(json.dumps(started, indent=2))
    initial_pid = adb('shell', 'pidof', GAME)['stdout'].strip()
    samples = []
    for second in (10, 30, 60):
        time.sleep(10 if second == 10 else 20 if second == 30 else 30)
        pid = adb('shell', 'pidof', GAME)['stdout'].strip()
        samples.append({'after_seconds': second, 'pid': pid})
        screen(name + '-' + str(second))
    runtime_text = adb('logcat', '-d', '-t', '8000')['stdout']
    # Keep only engine/crash evidence; third-party SDK payloads can contain keys.
    evidence = '\n'.join(line for line in runtime_text.splitlines() if re.search(r'\s(?:Unity|IL2CPP|CRASH|DEBUG|AndroidRuntime|libc)\s*:', line))
    (OUT / (name + '-runtime.txt')).write_text(evidence)
    (OUT / (name + '-activity.txt')).write_text(adb('shell', 'dumpsys', 'activity', 'activities')['stdout'])
    return {'launch': started, 'initial_pid': initial_pid, 'process_samples': samples,
            'stable_without_crash': healthy_start(initial_pid, samples, runtime_text),
            'survived_60_seconds': bool(samples[-1]['pid']),
            'meaning': 'Process survival is not proof of login, gameplay or feature functionality.'}


report = {'test': 'source-bootstrap-candidate-startup' if os.environ.get('MODIFIED_GAME_APK') else 'existing-release-game-startup', 'gameplay_verified': False}
try:
    adb('shell', 'settings', 'put', 'global', 'http_proxy', '10.0.2.2:8765')
    adb('shell', 'wm', 'size', '720x1280')
    adb('shell', 'wm', 'density', '240')
    report['abi_list'] = adb('shell', 'getprop', 'ro.product.cpu.abilist')['stdout'].strip()
    report['native_bridge'] = adb('shell', 'getprop', 'ro.dalvik.vm.native.bridge')['stdout'].strip()
    if 'arm64-v8a' not in report['abi_list'].split(','):
        report['result'] = 'UNSUPPORTED_EMULATOR_ABI'
        raise RuntimeError('This emulator cannot execute the supplied ARM64 game. No game result inferred.')
    installed = adb('install-multiple', '-r', 'original/com.YoStarEN.StellaSora.apk',
                    'original/config.arm64_v8a.apk', timeout=120)
    report['stock_install'] = installed
    if installed['code'] != 0 or 'Success' not in installed['stdout']:
        report['result'] = 'STOCK_INSTALL_FAILED'
        raise RuntimeError('Stock baseline installation failed')
    report['stock'] = launch('stock')
    adb('shell', 'am', 'force-stop', GAME)
    # Uninstalls ONLY the stock baseline inside this freshly created emulator.
    removed = adb('uninstall', GAME)
    if removed['code'] != 0:
        raise RuntimeError('Could not reset disposable game installation')
    menu = adb('install', '-r', os.environ.get('MENU_APK', 'mobile/dist/SoraVynx-Menu-0.1.0-arm64-preview.apk'), timeout=120)
    modified = adb('install', '-r', os.environ.get('MODIFIED_GAME_APK', 'mobile/dist/StellaSora-SoraVynx-2.0.0-arm64-preview.apk'), timeout=120)
    report['menu_install'], report['modified_install'] = menu, modified
    if menu['code'] != 0 or modified['code'] != 0 or 'Success' not in modified['stdout']:
        report['result'] = 'MODIFIED_INSTALL_FAILED'
        raise RuntimeError('Released APK installation failed')
    report['modified'] = launch('modified')
    adb('shell', 'am', 'start', '-W', '-n', 'com.soravynx.android/.MainActivity')
    time.sleep(8)
    screen('menu-after-game')
    report['result'] = 'STARTUP_OBSERVED' if report['modified']['stable_without_crash'] else 'MODIFIED_CRASHED_OR_RESTARTED'
    if report['result'] == 'STARTUP_OBSERVED' and not report['stock']['stable_without_crash']:
        report['result'] = 'BASELINE_UNSTABLE'
        raise RuntimeError('Modified process survived but stock baseline was unstable; comparison is inconclusive')
    if report['result'] == 'MODIFIED_CRASHED_OR_RESTARTED':
        raise RuntimeError('Modified game process exited during startup; inspect captured evidence')
finally:
    (OUT / 'report.json').write_text(json.dumps(report, indent=2))
    print(json.dumps(report, indent=2))
