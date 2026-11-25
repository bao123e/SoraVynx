WUWA WORKBENCH 0.2.0 — BẢN THỬ NHẬN DIỆN TIẾN TRÌNH

Ứng dụng Windows x64 tách phần giao diện, cấu hình và phím tắt từ SoraVynx
v1.3.5-custom-keys-preview.1, commit 80fad7a42f1a8f0ec2a30d26270f9e163b849c7b.
Đây là khung giao diện và trình theo dõi tiến trình theo đường dẫn EXE đã chọn.
Chưa có backend Unreal/Puerts, ESP, sửa gameplay hoặc chức năng injector.

CÁCH DÙNG
Giải nén ZIP vào thư mục bạn chọn, mở WuwaWorkbench.exe trực tiếp.
Không cần mở game. Cửa sổ hiện ngay. Close/X thoát ứng dụng.

Overview: Select game EXE để chọn file thực thi của game, thường là
Client-Win64-Shipping.exe trong thư mục Client/Binaries/Win64 của bản cài.
Chọn file game thực tế trên máy bạn, không chọn launcher. App lưu lựa chọn và
quét lại mỗi 2 giây trên luồng nền. Rescan yêu cầu kiểm tra ngay; Clear selection
xóa lựa chọn. Có thể chọn file trước khi mở game để theo dõi lúc game bắt đầu.

App so khớp đường dẫn đầy đủ, hiển thị PID/số tiến trình trùng đường dẫn.
Chỉ trùng tên file không đủ để báo Running. Nếu Windows không cho truy vấn
đường dẫn, app báo chưa xác minh được, không kết luận game đã kết nối.
EXE file version là metadata Windows của file, có thể khác số phiên bản game
như 3.8; không dùng trường này để xác nhận khả năng tương thích của mod.

Local tools: bộ đếm thời gian cục bộ có Start/Pause, Reset và Mark.
Appearance: chọn ảnh nền, chỉnh độ đậm nền/panel/nút, luôn nổi, hiện giờ/trạng thái.
Keybinds: chọn ô rồi nhấn tổ hợp phím; Save, Cancel, Reset defaults.
Trùng phím hoặc phím đã bị ứng dụng khác chiếm sẽ từ chối Save và giữ cấu hình cũ.
F1 hiện/ẩn cửa sổ theo mặc định. Có biểu tượng khay hệ thống để mở lại hoặc thoát.
Phím phục hồi cửa sổ là global. Các phím còn lại chỉ đăng ký khi app có focus;
không thực hiện tác vụ nền trên cửa sổ game. Nếu F1 bị chiếm, dùng khay để mở app
và đổi phím trong Keybinds.

PHÍM MẶC ĐỊNH
F1: hiện/ẩn cửa sổ.
Ctrl+Alt+1 / 2: Overview / Appearance.
Ctrl+Alt+3: luôn nổi. Ctrl+Alt+4 / 5: hiện giờ / hiện trạng thái.
F6 / F7 / F8: bắt đầu / tạm dừng / đặt lại bộ đếm.
Ctrl+Alt+9: bắt đầu/tạm dừng. F9: ghi dấu thời gian. Ctrl+Alt+0: dừng và đặt lại.

CẤU HÌNH
Lưu tại %LOCALAPPDATA%\WuwaWorkbench\config.ini.
File này riêng với cấu hình SoraVynx; bản thử không sửa bộ cài SoraVynx.
Bộ đếm đang chạy không tự tiếp tục sau khi thoát/mở ứng dụng.
Lựa chọn game nằm ở [Game] Executable trong cùng file cấu hình.
Nhấn Reset defaults trong trình phím chỉ áp dụng sau khi Save.

BUILD TỪ NGUỒN
Cần Python 3 và Zig 0.14.1. Chạy trong thư mục source/ của ZIP:
  python build.py --zig đường-dẫn-tới-zig
Kết quả nằm tại dist/. Lệnh cross-compile dùng được trên Linux hoặc Windows.

KIỂM TRA WINDOWS
  WuwaWorkbench.exe --self-test đường-dẫn-tuyệt-đối-tới-report.txt
Tạo cửa sổ kiểm tra ẩn, dùng thư mục cấu hình tạm, kiểm tra cấu hình/phím/tác vụ,
nhận diện đúng đường dẫn và vòng đời một tiến trình trợ giúp vô hại, ghi báo cáo
PASS/FAIL rồi thoát. Không thay đổi cấu hình người dùng hiện có.

PHẠM VI NHẬN DIỆN
Truy vấn danh sách tiến trình và đường dẫn bằng quyền Windows giới hạn; đọc
metadata phiên bản từ EXE đã chọn. Không đọc/ghi bộ nhớ game, không nạp DLL,
không khởi chạy hoặc dừng game. Running chỉ nghĩa là tìm thấy đúng tiến trình;
không phải xác nhận tích hợp gameplay, hỗ trợ bản 3.8 hay khả năng qua anti-cheat.

MÃ NGUỒN VÀ GIẤY PHÉP
Native C11, Win32/GDI, Windows Common Controls và GDI+.
keybinds.h, ui_layout.h và ui_background.h có nguồn gốc SoraVynx; các điều chỉnh
loại bỏ phụ thuộc game trong thành phần UI được ghi ở mã nguồn hiện tại.
Giấy phép AGPL-3.0-or-later; xem LICENSE.txt. ZIP bao gồm nguồn của bản thử.
