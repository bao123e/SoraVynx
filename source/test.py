"""Native mock integration tests; does not execute the Windows DLL or game."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parent
if __name__=='__main__':
 with tempfile.TemporaryDirectory(prefix='soravynx-tests-') as temporary:
  binary=Path(temporary)/'backend-test'
  subprocess.run(['gcc','-O1','-g','-std=c11','-Wall','-Wextra','-Werror',
   '-Wno-unused-function','-Wno-misleading-indentation','-ffunction-sections',
   '-fdata-sections','-fsanitize=address,undefined','-I'+str(ROOT/'tests/include'),
   str(ROOT/'tests/backend_test.c'),'-Wl,--gc-sections','-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True)
  path_binary=Path(temporary)/'launcher-path-test'
  subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(ROOT/'tests/launcher_path_test.c'),'-o',str(path_binary)],check=True)
  subprocess.run([str(path_binary)],check=True)
  print('PASS launcher path normalization: doubled separators, spaces/quotes, UNC and empty path')

  capture_binary=Path(temporary)/'launcher-capture-test'
  subprocess.run(['gcc','-O1','-std=c11','-Wall','-Wextra','-Werror',str(ROOT/'tests/launcher_capture_test.c'),'-o',str(capture_binary)],check=True)
  subprocess.run([str(capture_binary)],check=True)
  print('PASS loader capture x64 ABI, full-width module and immediate error capture with mock API calls')

  layout_binary=Path(temporary)/'ui-layout-test'
  subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(ROOT/'tests/ui_layout_test.c'),'-o',str(layout_binary)],check=True)
  subprocess.run([str(layout_binary)],check=True)
  print('PASS full-window background fit/fill: original aspect ratio, coverage, portrait/wide and invalid source')
  daily_binary=Path(temporary)/'daily-text-test'
  subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(ROOT/'tests/daily_text_test.c'),'-o',str(daily_binary)],check=True)
  subprocess.run([str(daily_binary)],check=True)
  print('PASS daily metadata name filtering and bounded formatting including truncation and zero-size buffers')

  subprocess.run(['python', str(ROOT/'test_lua_inventory.py')],check=True)
