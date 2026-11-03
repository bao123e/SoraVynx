# SoraVynx — mã nguồn Windows x64

SoraVynx là tên mới của toolkit Stella Sora. Launcher **SoraVynx.exe**, thư viện **SoraVynx.Runtime.dll** luôn phải cùng một build. Đóng game trước khi chuyển từ bản cũ; không đổi tên hoặc trộn EXE/DLL giữa hai bản. config.ini vẫn giữ các tùy chọn nền đã lưu.

Menu không đánh số: **Player**, **Event**, **Custom BG**, **Auto Daily**. Player/Event giữ backend trước; các hàng đủ chiều ngang và ô Damage/Vacuum/Score nằm cùng hàng. Custom BG có Image/Panel/Button opacity riêng 0–100%, Fit/Fill và ảnh PNG/JPG/BMP. Font mới **Be Vietnam Pro** nhúng Regular/SemiBold/Bold, hỗ trợ tiếng Việt; logo SoraVynx trong app và icon EXE. Không cài font hệ thống, không tạo log.

## Auto Daily: setup preview, chưa tự chạy

Mười hai lựa chọn theo ảnh người dùng: Tier-up Trial; Ascension; nâng Disc; nâng Trekker; 5 battle stages; tiêu 100 Vigor; tương tác Trekker màn chính; nhận thưởng Tyrant’s Tower; tặng quà Heartlink; dispatch commission; nhận quà ngẫu nhiên Shop; đăng nhập ngày. Dự kiến dùng đội, màn, difficulty, Trekker và Disc đang chọn trong game.

Bản này chỉ lưu danh sách lựa chọn và tạo thông tin hàm để hoàn tất tích hợp. Không có nút Start giả, không chạy hay đánh dấu hoàn thành nhiệm vụ. Khi game đã vào màn chính và Player/Event báo READY: mở **Auto Daily → Copy setup report v2**, dán nội dung vào cuộc trao đổi phát triển. Report v2 quét toàn bộ danh sách lớp trước, loại lớp Protobuf bằng chữ ký get_Parser (không loại cả namespace), ưu tiên controller/manager và UI, giữ chỉ mục lớp rồi chia dung lượng riêng cho sáu nhóm nhiệm vụ. Kèm tên tham số, tên field và hàm truy cập singleton kế thừa, không đọc giá trị field. Báo cáo chỉ đọc metadata Hotfix vào clipboard, giới hạn 256 KiB, 24 KiB mỗi nhóm và 4 KiB mỗi lớp; các danh sách dài có ghi giới hạn và mức bao phủ. Báo cáo không đọc thông tin tài khoản, không gọi hành động game và không tạo tệp. Cần đối chiếu báo cáo và kiểm tra luồng game trước khi nối các hành động daily thực tế.

## Build / kiểm thử

```sh
python source/build.py --zig /path/to/zig-0.14.1
python source/test.py
# Optional Windows/Wine mock report regression:
python source/test_report_windows.py --zig /path/to/zig-0.14.1
```

Build xuất EXE, DLL và build.json vào source/dist. SHA-256 DLL nhúng trong EXE; source/build.json ghi hash của cặp phát hành. Bitmap nền không nhúng sẵn; resource chỉ có logo/icon và font. Các tệp EXE/DLL phát hành nằm ngay gốc repo; ZIP nằm ở GitHub Releases.

20 nhóm kiểm thử native gồm backend mô phỏng, tiện ích launcher, hình học Fit/Fill và bộ định dạng báo cáo có giới hạn. Đã kiểm tra 13 nhóm UI qua Wine, gồm font/logo, trang Auto Daily và sao chép metadata qua clipboard với API giả; không thay thế kiểm tra game thật. Auto Daily chưa có backend thực hiện nhiệm vụ.

Giữ F1/Hide để ẩn mà không tắt tính năng; X/Alt+F4 tắt tính năng rồi ẩn. F6/F7/F8 điều khiển event. Chỉnh giao diện và danh sách daily không đổi trạng thái Player/Event.

Thiết kế theo UI/UX Pro Max; mã AGPL-3.0-or-later, MinHook BSD, Be Vietnam Pro SIL OFL 1.1. Logo được tạo riêng cho SoraVynx; bản PNG gốc và ICO nằm trong assets. Giấy phép tổng hợp trong LICENSE.txt.

## Sửa report v1 trong preview.2

Report thực tế đầu tiên có 90 lớp, trong đó 85 lớp là schema Protobuf; bộ lọc cũ hết dung lượng trước khi đến các controller. Report v2 bỏ phần này, xuất chỉ mục trước các chữ ký, ưu tiên handler kể cả nằm sau phương thức thứ 32 và bổ sung thông tin lớp cha. Test Windows mock đặt 100 schema trước các lớp thật, kiểm tra đủ sáu nhóm, field/singleton kế thừa và clipboard, đồng thời xác nhận không có lệnh game được gọi. Trên Linux dùng --wine hoặc --build-only --out để biên dịch riêng.
