"""Cross-compile the experimental Windows x64 runtime backend with Zig 0.14.1."""
from pathlib import Path
import argparse,subprocess,json,hashlib
ROOT=Path(__file__).resolve().parent
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--zig',default='zig');a=p.parse_args()
 out=ROOT/'dist';out.mkdir(exist_ok=True)
 vendor=ROOT/'vendor/minhook'
 subprocess.run([a.zig,'rc','/fo',str(out/'ui.res'),'--',str(ROOT/'assets/ui.rc')],check=True)
 subprocess.run([a.zig,'rc','/fo',str(out/'launcher.res'),'--',str(ROOT/'assets/launcher.rc')],check=True)
 sources=[ROOT/'main.c',ROOT/'api.c',ROOT/'backend.c']+[vendor/'src'/n for n in ['hook.c','buffer.c','trampoline.c','hde/hde64.c']]
 cmd=[a.zig,'cc','-target','x86_64-windows-gnu','-O2','-g0','-std=c11','-Wall','-Wextra','-Werror','-Wno-unused-function','-shared','-Wl,--dynamicbase','-Wl,--nxcompat','-I'+str(vendor/'include')]+list(map(str,sources))+[str(out/'ui.res'),'-lgdiplus','-lcomdlg32','-lmsimg32','-luser32','-lgdi32','-lkernel32','-lbcrypt','-o',str(out/'SoraVynx.Runtime.dll')]
 subprocess.run(cmd,check=True)
 subprocess.run([a.zig,'cc','-target','x86_64-windows-gnu','-O2','-std=c11','-Wall','-Wextra','-Werror',str(ROOT/'launcher.c'),str(out/'launcher.res'),'-DEXPECTED_DLL_SHA256="'+hashlib.sha256((out/'SoraVynx.Runtime.dll').read_bytes()).hexdigest()+'"','-lbcrypt','-municode','-Wl,--subsystem,windows','-lcomdlg32','-luser32','-lkernel32','-o',str(out/'SoraVynx.exe')],check=True)
 data=(out/'SoraVynx.Runtime.dll').read_bytes()
 (out/'build.json').write_text(json.dumps({'product':'SoraVynx','version':'1.1.0-preview.2','auto_daily_ready':False,'sha256':hashlib.sha256(data).hexdigest(),'bytes':len(data),'launcher_sha256':hashlib.sha256((out/'SoraVynx.exe').read_bytes()).hexdigest(),'launcher_runtime_tested':False,'tool_upstream_commit':'30f67811411dca754f1b876b4d4dbf6a001e0a20','minhook_commit':'8af6b4acae5a9388fd742b56fa79ece89d96f823','compiler':'Zig 0.14.1 / Clang Windows x64 GNU','gameassembly_sha256':'6adc5de8d2ed85ee752be80b38aff0e5e85d21e6efb6133c262c9f2632e06dd8','windows_runtime_tested':False,'gameplay_verified':False},indent=2)+'\n')
 print('Built',out/'SoraVynx.Runtime.dll');print('Windows/game runtime validation is still required.')
