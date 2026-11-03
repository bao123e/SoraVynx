# SoraVynx — mã nguồn Windows x64

SoraVynx là tên mới của toolkit Stella Sora. Launcher **SoraVynx.exe**, thư viện **SoraVynx.Runtime.dll** luôn phải cùng một build. Đóng game trước khi chuyển từ bản cũ; không đổi tên hoặc trộn EXE/DLL giữa hai bản. config.ini vẫn giữ các tùy chọn nền đã lưu.

Menu không đánh số: **Player**, **Event**, **Custom BG**, **Auto Daily**. Player/Event giữ backend trước; các hàng đủ chiều ngang và ô Damage/Vacuum/Score nằm cùng hàng. Custom BG có Image/Panel/Button opacity riêng 0–100%, Fit/Fill và ảnh PNG/JPG/BMP. Font mới **Be Vietnam Pro** nhúng Regular/SemiBold/Bold, hỗ trợ tiếng Việt; logo SoraVynx trong app và icon EXE. Không cài font hệ thống, không tạo log.

## Auto Daily: setup preview, chưa tự chạy

Mười hai lựa chọn theo ảnh người dùng: Tier-up Trial; Ascension; nâng Disc; nâng Trekker; 5 battle stages; tiêu 100 Vigor; tương tác Trekker màn chính; nhận thưởng Tyrant’s Tower; tặng quà Heartlink; dispatch commission; nhận quà ngẫu nhiên Shop; đăng nhập ngày. Dự kiến dùng đội, màn, difficulty, Trekker và Disc đang chọn trong game.

Bản này lưu danh sách lựa chọn và cung cấp hai báo cáo để hoàn tất tích hợp. Không có nút Start giả, không chạy hay đánh dấu hoàn thành nhiệm vụ. Dự kiến giữ Trekker, Disc, đội và màn đang chọn trong game.

Đóng game và cập nhật cả EXE/DLL cùng bản. Vào màn chính, đợi Player/Event READY, chọn **Auto Daily → Copy Lua name report**, chờ thông báo đã sao chép rồi dán nội dung vào cuộc trao đổi phát triển. Không cần làm lại nhiệm vụ. Nếu không có callback Update sau 15 giây, yêu cầu bị hủy; trở lại màn chính rồi thử lại. Nếu cầu nối không khả dụng, tool báo rõ và Player/Event vẫn hoạt động.

**Lua report v5:** dùng cầu nối main-thread đã xác nhận hoạt động từ report v4. Một hook riêng gọi Update gốc trước, kiểm tra Unity main thread, đọc singleton LuaManager/LuaEnv hiện có và chạy script kiểm tra cố định nhúng trong resource. Không nhận script tùy ý, không require module mới, không gọi hàm nhiệm vụ tìm được.

Script ưu tiên 18 lớp dữ liệu liên quan daily cùng các controller UI đã nạp, bỏ duyệt đệ quy super/__index. Thu thập hàm khai báo trực tiếp, tên/số tham số qua debug API nếu runtime hỗ trợ và liên kết cấu trúc PlayerData/PanelManager tới lớp đã nạp. Khi debug API không hỗ trợ hoặc lỗi, ghi unknown; không suy ra hàm không có đối số. Không xuất scalar, địa chỉ, thân hàm, giá trị tài khoản hay thông báo lỗi debug/Lua chi tiết.

Giới hạn 30000 entry tổng, 2048 entry mỗi lần liệt kê bảng, 2400 dòng và 192 KiB nội dung; mỗi module tối đa 96 hàm/12 KiB, chia lượt giữa các module. Có mức bao phủ và NOT_LOADED cho lớp chưa thấy trong package.loaded. Nếu cần lấy thêm controller, mở trang tương ứng trong game rồi chụp lại; không phải nâng cấp, tặng quà, tiêu Vigor hay làm lại daily. Không cần gửi lại Copy API metadata v3. Script có cấp phát tạm trong Lua; kiểm thử mô phỏng không thay thế xác minh game thật.

**Copy API metadata:** giữ report v3 để chẩn đoán C#/IL2CPP: danh sách assembly, quét Game/Hotfix và assembly ứng dụng, ưu tiên Lua/UI cùng sáu nhóm nhiệm vụ. Giới hạn 512 KiB tổng, 192 KiB chỉ mục; nhóm Lua/UI 64 KiB với 16 KiB/lớp, nhóm nhiệm vụ 24 KiB với 4 KiB/lớp. Chỉ metadata, không đọc giá trị field hay gọi hành động game.

Cả hai báo cáo chỉ sao chép vào clipboard, không tạo tệp log. Auto Daily vẫn chưa thực hiện nhiệm vụ; cần xác minh module/hàm và luồng game trước khi nối hành động thực tế.

## Build / kiểm thử

```sh
python source/build.py --zig /path/to/zig-0.14.1
python source/test.py  # GCC + liblua5.4 (or liblua5.3)
# Optional Windows/Wine mock report regression:
python source/test_report_windows.py --zig /path/to/zig-0.14.1
```

Build xuất EXE, DLL và build.json vào source/dist. SHA-256 DLL nhúng trong EXE; source/build.json ghi hash của cặp phát hành. Bitmap nền không nhúng sẵn; resource chỉ có logo/icon và font. Các tệp EXE/DLL phát hành nằm ngay gốc repo; ZIP nằm ở GitHub Releases.

21 nhóm kiểm thử native/Lua gồm backend mô phỏng, tiện ích launcher, hình học Fit/Fill và bộ định dạng báo cáo có giới hạn và script inventory chạy trên Lua VM thật. Bộ test Windows/Wine mới kiểm tra callback Update gốc, idle/shared/off-main guards, hàng đợi một yêu cầu, reentrancy, GC handle, exception, clipboard và timeout. Đã kiểm tra 13 nhóm UI qua Wine, gồm font/logo, trang Auto Daily và sao chép metadata qua clipboard với API giả; không thay thế kiểm tra game thật. Auto Daily chưa có backend thực hiện nhiệm vụ.

Giữ F1/Hide để ẩn mà không tắt tính năng; X/Alt+F4 tắt tính năng rồi ẩn. F6/F7/F8 điều khiển event. Chỉnh giao diện và danh sách daily không đổi trạng thái Player/Event.

Thiết kế theo UI/UX Pro Max; mã AGPL-3.0-or-later, MinHook BSD, Be Vietnam Pro SIL OFL 1.1. Logo được tạo riêng cho SoraVynx; bản PNG gốc và ICO nằm trong assets. Giấy phép tổng hợp trong LICENSE.txt.

## Sửa report v1 trong preview.2

Report thực tế đầu tiên có 90 lớp, trong đó 85 lớp là schema Protobuf; bộ lọc cũ hết dung lượng trước khi đến các controller. Report v2 bỏ phần này, xuất chỉ mục trước các chữ ký, ưu tiên handler kể cả nằm sau phương thức thứ 32 và bổ sung thông tin lớp cha. Test Windows mock đặt 100 schema trước các lớp thật, kiểm tra đủ sáu nhóm, field/singleton kế thừa và clipboard, đồng thời xác nhận không có lệnh game được gọi. Trên Linux dùng --wine hoặc --build-only --out để biên dịch riêng.

## Mở rộng report v3 trong preview.3

Report v2 thực tế đã liệt kê đủ 637 ứng viên Hotfix nhưng không có luồng daily/UI cần dùng: dispatch chỉ là animation dispatcher, shop chủ yếu enum. Phân tích GameAssembly tìm thấy tham chiếu Game:XLua.LuaEnv/GCAction, nên report v3 mở rộng sang assembly Game và metadata cầu nối Lua/UI. Đây là bước tìm API, chưa xác nhận module Lua thực hiện nhiệm vụ. Test Windows/Wine mô phỏng ba assembly, kiểm tra thu được LuaEnv/LuaTable/LuaManager trong Game, bỏ qua framework và không gọi runtime_invoke hay hành động game.

## Lua report v4 trong preview.4

Report v3 thực tế liệt kê đủ 1005 ứng viên, trong đó có Game.LuaManager, LuaUIController và XLua.LuaEnv/LuaTable. Đây là cầu nối C#, chưa phải tên module nghiệp vụ Lua. Bản preview.4 bổ sung inventory tên module/hàm đã nạp thay vì tiếp tục tăng dung lượng metadata. Test trên Lua VM thật kiểm tra thứ tự ổn định, chu trình, độ sâu, dung lượng và không gọi các hàm game/metamethod trong fixture hoặc xuất scalar bí mật. Không cam kết mọi hàm daily đã xuất hiện hoặc Auto Daily sẽ hoạt động chỉ từ tên hàm.

## Kết quả report v4 và sửa trong preview.5

Report v4 thực tế đã đọc Lua thành công: 4090 entry, 300 bảng, đủ 2400 dòng và Limited=yes. Có ReceiveDailyReward, ReceiveDailyActiveReward, SendDailyShopRewardReceiveReq, SendDiscStrengthenReq, ReqApplyAgent và ITDailyRewardReq, nhưng tên hàm không xác nhận được đối số, điều kiện gọi, tài nguyên hoặc callback hoàn tất. Nhiều dòng thuộc super của Login/CheckIn controller; các module PlayerCharData/PlayerDatingData chỉ còn tên, thiếu chi tiết.

Report v5 sửa đúng phần này, bổ sung metadata tham số và liên kết instance. Auto Daily vẫn setup-only; danh sách hàm chưa đủ để chạy một chuỗi nhiệm vụ thật. [Bản đồ thông tin đã tìm được](AUTO_DAILY.vi.md) ghi rõ những phần còn thiếu.
