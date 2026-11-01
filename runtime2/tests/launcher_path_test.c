#include <assert.h>
#include "../launcher_path.h"
int main(void) {
 wchar_t p0[]=L"D:\\\\YostarGames\\\\StellaSora_EN\\\\StellaSora.exe";normalize_path(p0);assert(!wcscmp(p0,L"D:\\YostarGames\\StellaSora_EN\\StellaSora.exe"));
 wchar_t p1[]=L"  \"D:/My Games/StellaSora.exe\"  ";normalize_path(p1);assert(!wcscmp(p1,L"D:\\My Games\\StellaSora.exe"));
 wchar_t p2[]=L"\\\\server\\share\\StellaSora.exe";normalize_path(p2);assert(!wcscmp(p2,L"\\\\server\\share\\StellaSora.exe"));
 wchar_t p3[]=L"";normalize_path(p3);assert(!wcscmp(p3,L""));
 return 0;
}
