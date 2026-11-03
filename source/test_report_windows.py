"""Mock-only report regression; does not open or execute the real game."""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent


def run(args, output):
    output.mkdir(parents=True, exist_ok=True)
    common = [args.zig, 'cc', '-target', 'x86_64-windows-gnu', '-O2',
              '-UNDEBUG', '-std=c11', '-Wall', '-Wextra', '-Werror']
    subprocess.run(common + ['-shared', str(ROOT / 'tests/daily_report_fixture.c'),
                            '-o', str(output / 'GameAssembly.dll')], check=True)
    executable = output / 'daily-report-test.exe'
    subprocess.run(common + [str(ROOT / 'tests/daily_report_windows_test.c'),
                            '-luser32', '-lkernel32', '-o', str(executable)], check=True)
    lua_executable = output / 'daily-lua-test.exe'
    subprocess.run(common + ['-Wno-unused-function', str(ROOT / 'tests/daily_lua_windows_test.c'),
                            '-luser32', '-lkernel32', '-o', str(lua_executable)], check=True)
    for binary in (executable, lua_executable):
        if args.build_only:
            print('Built mock report regression:', binary)
        else:
            command = [str(binary)] if os.name == 'nt' else [args.wine, str(binary)]
            subprocess.run(command, check=True, cwd=output)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--zig', default='zig')
    parser.add_argument('--wine', help='Wine binary for a non-Windows host')
    parser.add_argument('--out', type=Path)
    parser.add_argument('--build-only', action='store_true')
    args = parser.parse_args()
    if os.name != 'nt' and not args.build_only and not args.wine:
        parser.error('Use --wine on a non-Windows host, or --build-only.')
    if args.build_only and not args.out:
        parser.error('--build-only requires --out so fixtures remain available.')
    if args.out:
        run(args, args.out.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='soravynx-report-test-') as temporary:
            run(args, Path(temporary))
