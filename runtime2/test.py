"""Native mock integration tests; does not execute the Windows DLL or game."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parent
if __name__=='__main__':
 with tempfile.TemporaryDirectory(prefix='sstool2-tests-') as temporary:
  binary=Path(temporary)/'backend-test'
  subprocess.run(['gcc','-O1','-g','-std=c11','-Wall','-Wextra','-Werror',
   '-Wno-unused-function','-Wno-misleading-indentation','-ffunction-sections',
   '-fdata-sections','-fsanitize=address,undefined','-I'+str(ROOT/'tests/include'),
   str(ROOT/'tests/backend_test.c'),'-Wl,--gc-sections','-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True)
