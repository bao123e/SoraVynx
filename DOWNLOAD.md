# SSTool standalone 4

[Tải SSTool.exe mới](releases/SSTool.exe) · [MG_cheat.dll](releases/MG_cheat.dll)

Đặt hai file cùng thư mục. Có thể đặt config.ini đã có cạnh EXE để tự đọc [Config] GamePath.

Mở SSTool.exe, chọn StellaSora.exe bằng Browse rồi bấm Start game + tool. Đường dẫn được lưu cho lần sau; chương trình mở game, chờ runtime rồi nạp DLL. Nếu game đã chạy thì nạp vào phiên đó. Không cần nhập key.

[Hướng dẫn](runtime2/README.vi.md) · [ZIP có mã nguồn](releases/SSTool-standalone4.zip)

DLL gameplay giữ nguyên. Build Windows GUI x64, 13 nhóm mock backend và kiểm tra chuẩn hóa đường dẫn đạt; giao diện/khởi chạy game thực chưa được thử trên Windows trong môi trường build.

Trình nạp mới kiểm tra DLL đúng build và ghi SSTool-launcher.log. Nguyên nhân lỗi loader được người dùng báo chưa xác định; bản này bổ sung chẩn đoán, chưa được kiểm thử trên Windows.

Loader 4 lưu HMODULE 64-bit và GetLastError ngay sau LoadLibraryW trong remote thread, cùng stage trước/sau lời gọi. Adapter qua kiểm thử x64 ABI bằng mock; chưa xác định nguyên nhân hoặc sửa được lỗi nạp trong game.
