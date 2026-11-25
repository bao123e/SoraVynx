WUWA WORKBENCH 0.1.0 — BẢN THỬ GIAO DIỆN ĐỘC LẬP

Ứng dụng Windows x64 tách phần giao diện, cấu hình và phím tắt từ SoraVynx
v1.3.5-custom-keys-preview.1, commit 80fad7a42f1a8f0ec2a30d26270f9e163b849c7b.
Đây là khung giao diện chuẩn bị cho việc phát triển riêng. Chưa tích hợp WuWa,
chưa có backend Unreal/Puerts, ESP, sửa gameplay hoặc chức năng injector.

CÁCH DÙNG
Giải nén ZIP vào thư mục bạn chọn, mở WuwaWorkbench.exe trực tiếp.
Không cần mở game. Cửa sổ hiện ngay. Close/X thoát ứng dụng.

Overview: bộ đếm thời gian cục bộ có Start/Pause, Reset và Mark.
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
Nhấn Reset defaults trong trình phím chỉ áp dụng sau khi Save.

BUILD TỪ NGUỒN
Cần Python 3 và Zig 0.14.1. Chạy trong thư mục source/ của ZIP:
  python build.py --zig đường-dẫn-tới-zig
Kết quả nằm tại dist/. Lệnh cross-compile dùng được trên Linux hoặc Windows.

KIỂM TRA WINDOWS
  WuwaWorkbench.exe --self-test đường-dẫn-tuyệt-đối-tới-report.txt
Tạo cửa sổ kiểm tra ẩn, dùng thư mục cấu hình tạm, kiểm tra cấu hình/phím/tác vụ,
ghi báo cáo PASS/FAIL rồi thoát. Không thay đổi cấu hình người dùng hiện có.

MÃ NGUỒN VÀ GIẤY PHÉP
Native C11, Win32/GDI, Windows Common Controls và GDI+.
keybinds.h, ui_layout.h và ui_background.h có nguồn gốc SoraVynx; các điều chỉnh
loại bỏ phụ thuộc game trong thành phần UI được ghi ở mã nguồn hiện tại.
Giấy phép AGPL-3.0-or-later; xem LICENSE.txt. ZIP bao gồm nguồn của bản thử.
