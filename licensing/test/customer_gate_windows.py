"""Smoke-test the actual owner-configured launcher without starting a game."""
import ctypes as c
from ctypes import wintypes as w
import os,re,subprocess,sys,time,json
from pathlib import Path
if sys.platform!='win32':raise SystemExit('Windows is required')
u=c.WinDLL('user32',use_last_error=True)
u.GetWindowThreadProcessId.argtypes=[w.HWND,c.POINTER(w.DWORD)];u.GetWindowThreadProcessId.restype=w.DWORD
u.GetClassNameW.argtypes=[w.HWND,w.LPWSTR,c.c_int];u.GetClassNameW.restype=c.c_int
u.GetDlgItem.argtypes=[w.HWND,c.c_int];u.GetDlgItem.restype=w.HWND
u.GetWindowTextW.argtypes=[w.HWND,w.LPWSTR,c.c_int];u.GetWindowTextW.restype=c.c_int
u.SetWindowTextW.argtypes=[w.HWND,w.LPCWSTR];u.SetWindowTextW.restype=w.BOOL
u.SendMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM];u.SendMessageW.restype=c.c_ssize_t
u.PostMessageW.argtypes=[w.HWND,w.UINT,w.WPARAM,w.LPARAM];u.PostMessageW.restype=w.BOOL
callback_type=c.WINFUNCTYPE(w.BOOL,w.HWND,w.LPARAM)
u.EnumWindows.argtypes=[callback_type,w.LPARAM];u.EnumWindows.restype=w.BOOL
exe=Path(sys.argv[1]).resolve();wrong_key=Path(sys.argv[2]).read_text().strip()
env=dict(os.environ);data=Path(os.environ['RUNNER_TEMP'])/'owner-launcher-ui';data.mkdir(exist_ok=True)
env['LOCALAPPDATA']=str(data)
process=subprocess.Popen([str(exe)],env=env)
try:
 window=None
 for _ in range(150):
  found=[]
  @callback_type
  def visit(hwnd,_arg):
   pid=w.DWORD();u.GetWindowThreadProcessId(hwnd,c.byref(pid));name=c.create_unicode_buffer(100);u.GetClassNameW(hwnd,name,100)
   if pid.value==process.pid and name.value=='SoraVynxLicenseGate':found.append(hwnd)
   return True
  u.EnumWindows(visit,0)
  if found:window=found[0];break
  if process.poll() is not None:raise AssertionError('Launcher exited before its access-key gate')
  time.sleep(.1)
 assert window,'Access-key window was not created'
 def text(handle):
  value=c.create_unicode_buffer(2048);u.GetWindowTextW(handle,value,2048);return value.value
 assert not u.GetDlgItem(window,10),'Machine-code field must be absent'
 assert not u.GetDlgItem(window,2),'Copy button must be absent'
 assert u.GetDlgItem(window,11) and u.GetDlgItem(window,1),'Key input and submit button required'
 u.SetWindowTextW(u.GetDlgItem(window,11),wrong_key);u.SendMessageW(window,0x111,1,0)
 time.sleep(2)
 assert process.poll() is None,'Wrong-issuer key exited the gate unexpectedly'
 assert u.GetDlgItem(window,11),'Wrong-issuer key bypassed the activation window'
 # No activation is saved for a key signed by the disposable test issuer.
 assert not (data/'SoraVynx/license-short.dat').exists(),'Wrong issuer saved activation'
 u.PostMessageW(window,0x10,0,0);assert process.wait(timeout=10)==0
 print(json.dumps({'customer_launcher_gate':'passed','minimal_key_only_ui':True,'wrong_issuer_blocked':True,'cancel_exits_without_launching_game':True,'gameplay_verified':False}))
finally:
 if process.poll() is None:process.kill();process.wait()
