# SoraVynx — mã nguồn Windows x64

SoraVynx là tên mới của toolkit Stella Sora. Launcher **SoraVynx.exe**, thư viện **SoraVynx.Runtime.dll** luôn phải cùng một build. Đóng game trước khi chuyển từ bản cũ; không đổi tên hoặc trộn EXE/DLL giữa hai bản. config.ini vẫn giữ các tùy chọn nền đã lưu.

Menu không đánh số: **Player**, **Event**, **Custom BG**, **Auto Daily**. Player/Event giữ backend trước; các hàng đủ chiều ngang và ô Damage/Vacuum/Score nằm cùng hàng. Custom BG có Image/Panel/Button opacity riêng 0–100%, Fit/Fill và ảnh PNG/JPG/BMP. Font mới **Be Vietnam Pro** nhúng Regular/SemiBold/Bold, hỗ trợ tiếng Việt; logo SoraVynx trong app và icon EXE. Không cài font hệ thống, không tạo log.

## Auto Daily: thử Tower reward, bỏ qua Shop

Mười hai lựa chọn theo ảnh người dùng: Tier-up Trial; Ascension; nâng Disc; nâng Trekker; 5 battle stages; tiêu 100 Vigor; tương tác Trekker màn chính; nhận thưởng Tyrant’s Tower; tặng quà Heartlink; dispatch commission; nhận quà ngẫu nhiên Shop; đăng nhập ngày. Dự kiến dùng đội, màn, difficulty, Trekker và Disc đang chọn trong game.

Bản preview.10 dừng phần Shop theo yêu cầu: luôn xóa bit USE của Shop khi đọc cấu hình, lưu lại SKIP và khóa hàng Shop trong UI. Script Shop cũ giữ trong mã nguồn để lưu lịch sử, không còn được nhúng hay chạy trong DLL. Mười nhiệm vụ khác vẫn chưa triển khai; chỉ có lệnh thử Tower reward.

Đóng game, cập nhật cả EXE/DLL, vào game và mở Tyrant’s Tower để dữ liệu thưởng được nạp. Đợi READY, bật USE cho **Claim Tyrant’s Tower rewards**, chọn **Tower reward / status**. Lệnh chạy sau Update gốc trên main thread; tool tự ẩn và trả focus về game. F1 hiện lại tool, bấm lại nút để đọc trạng thái vào clipboard mà không gửi thêm yêu cầu.

Nguồn binding là report v5: PlayerData.InfinityTower có đúng class PlayerInfinityTowerData, ITDailyRewardReq(self) một tham số và không vararg, InfinityTowerRewardsTab là bảng. Khi module cache không có class chuẩn, chỉ chấp nhận source metadata của handler khớp đúng module. Thiếu/bất đồng class, arity hoặc dữ liệu thưởng thì dừng có Code. Không dùng CS/Unity object discovery hay registry PanelManager của phần Shop.

Mỗi phiên game chỉ thử gọi handler một lần, ghi state trước khi gọi để không tự retry sau exception. Các lần sau so sánh snapshot giới hạn của riêng bảng thưởng Tower (32 node, 128 entry/node, sâu hai tầng), chỉ xuất số field thay đổi, không xuất scalar/ID. Thay đổi có thể là cập nhật dữ liệu thông thường; không suy ra nhận thưởng thành công hay task hoàn tất. Chưa giải mã server response. Handler gốc và game chịu trách nhiệm xử lý/hiển thị thưởng, cần xác nhận trong game thật.

Disable all/X hoặc chuyển Tower sang SKIP sẽ hủy lệnh còn chờ; hành động đã chạy không được retry tự động. Nút Shop bị khóa SKIP và không thể gửi action Shop. Player/Event, Custom BG và font/logo giữ nguyên backend/giao diện trước. Không tạo log. Muốn thử lại sau lỗi hoặc ngày mới thì restart game để kết thúc giới hạn một lần thử.

**Copy Lua name report** vẫn đọc metadata, không gọi handler nhiệm vụ. Vào màn chính hoặc mở trang cần chẩn đoán, chờ READY rồi bấm nút và chuyển lại game. Không cần làm lại daily. Nếu không có callback Update sau 15 giây, yêu cầu chờ bị hủy.

**Lua report v5:** dùng cầu nối main-thread đã xác nhận hoạt động từ report v4. Một hook riêng gọi Update gốc trước, kiểm tra Unity main thread, đọc singleton LuaManager/LuaEnv hiện có và chạy script kiểm tra cố định nhúng trong resource. Không nhận script tùy ý, không require module mới, không gọi hàm nhiệm vụ tìm được.

Script ưu tiên 18 lớp dữ liệu liên quan daily cùng các controller UI đã nạp, bỏ duyệt đệ quy super/__index. Thu thập hàm khai báo trực tiếp, tên/số tham số qua debug API nếu runtime hỗ trợ và liên kết cấu trúc PlayerData/PanelManager tới lớp đã nạp. Khi debug API không hỗ trợ hoặc lỗi, ghi unknown; không suy ra hàm không có đối số. Không xuất scalar, địa chỉ, thân hàm, giá trị tài khoản hay thông báo lỗi debug/Lua chi tiết.

Giới hạn 30000 entry tổng, 2048 entry mỗi lần liệt kê bảng, 2400 dòng và 192 KiB nội dung; mỗi module tối đa 96 hàm/12 KiB, chia lượt giữa các module. Có mức bao phủ và NOT_LOADED cho lớp chưa thấy trong package.loaded. Nếu cần lấy thêm controller, mở trang tương ứng trong game rồi chụp lại; không phải nâng cấp, tặng quà, tiêu Vigor hay làm lại daily. Không cần gửi lại Copy API metadata v3. Script có cấp phát tạm trong Lua; kiểm thử mô phỏng không thay thế xác minh game thật.

**Copy API metadata:** giữ report v3 để chẩn đoán C#/IL2CPP: danh sách assembly, quét Game/Hotfix và assembly ứng dụng, ưu tiên Lua/UI cùng sáu nhóm nhiệm vụ. Giới hạn 512 KiB tổng, 192 KiB chỉ mục; nhóm Lua/UI 64 KiB với 16 KiB/lớp, nhóm nhiệm vụ 24 KiB với 4 KiB/lớp. Chỉ metadata, không đọc giá trị field hay gọi hành động game.

Hai báo cáo chẩn đoán chỉ sao chép vào clipboard, không tạo tệp log. Lệnh Tower riêng có thể gọi handler nhận thưởng game thật như mô tả trên; chưa có chuỗi tự động 12 nhiệm vụ.

## Build / kiểm thử

```sh
python source/build.py --zig /path/to/zig-0.14.1
python source/test.py  # GCC + liblua5.4 (or liblua5.3)
# Optional Windows/Wine mock report regression:
python source/test_report_windows.py --zig /path/to/zig-0.14.1
```

Build xuất EXE, DLL và build.json vào source/dist. SHA-256 DLL nhúng trong EXE; source/build.json ghi hash của cặp phát hành. Bitmap nền không nhúng sẵn; resource chỉ có logo/icon và font. Các tệp EXE/DLL phát hành nằm ngay gốc repo; ZIP nằm ở GitHub Releases.

22 nhóm kiểm thử native/Lua gồm backend mô phỏng, tiện ích launcher, hình học Fit/Fill và bộ định dạng báo cáo có giới hạn và script inventory chạy trên Lua VM thật. Bộ test Windows/Wine mới kiểm tra callback Update gốc, idle/shared/off-main guards, hàng đợi một yêu cầu, reentrancy, GC handle, exception, clipboard và timeout. Đã kiểm tra 13 nhóm UI qua Wine, gồm font/logo, trang Auto Daily và sao chép metadata qua clipboard với API giả; không thay thế kiểm tra game thật. Tower có handler request thử nghiệm; Shop bị SKIP và 10 nhiệm vụ khác chưa thực hiện.

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

Report v5 sửa phần này, bổ sung metadata tham số và liên kết instance. Ở preview.5 Auto Daily vẫn setup-only; danh sách hàm chưa đủ để chạy một chuỗi nhiệm vụ thật. [Bản đồ thông tin đã tìm được](AUTO_DAILY.vi.md) ghi rõ những phần còn thiếu.

## Hành động Shop đầu tiên trong preview.6

Report v5 thực tế xác nhận PlayerData.Shop, GetDailyShopReward(self) và SendDailyShopRewardReceiveReq(self, callback); đồng thời cho thấy DispatchData.ReqApplyAgent không có self, nên không thể dùng một kiểu gọi chung cho mọi task. Shop được nối riêng với kiểm tra chữ ký/instance và trạng thái một lần thử. Test trên Lua VM thật xác nhận handler mua hàng không bị gọi, callback không làm lộ payload, không có lần gửi thứ hai khi bấm lại hoặc sau exception. Build mới chưa được kiểm tra trong game thật.

## Sửa luồng UI trong preview.7

Ảnh preview.6 của người dùng cho thấy callback request đã về và cờ Shop đã đổi, nhưng tool không mở nút Daily Gift. Preview.7 thay direct request bằng đúng OnBtnClick_DailyGift của ShopCtrl đang hoạt động, dựa trên chữ ký report v5 và public getters LuaBehaviourBase từ report v3. Script không gọi tên hàm trên một class hoặc instance tùy ý. Tool ẩn trước khi trả focus về game; kết quả handler-invoked được copy mà không bật popup che UI. Chỉ kết quả lỗi hoặc lần hỏi trạng thái sau đó mới hiện hộp thông tin. Chưa xác minh thao tác này trong game thật.

## Sửa tra handler trong preview.8

Ảnh .7 cho thấy game đã ở Shop nhưng tool trả thông báo gộp về module/handler/chữ ký trước khi duyệt live UI. Chưa thể phân biệt nguyên nhân cụ thể chỉ từ thông báo cũ. Bản .8 chuyển lookup sang instance đang hiển thị, hỗ trợ class cache không có khóa chuẩn và các kiểu dispatch được dùng trong Lua, đồng thời tách Code chẩn đoán. Test dùng class không có trong package.loaded, handler kế thừa/__index function, controller pure Lua trong panel và missing local names; không gọi RPC dự phòng. Hành vi game thật vẫn cần kiểm tra.

## Sửa trường hợp zero behaviours trong preview.9

Ảnh thực tế trả SHOP_CONTROLLER_NOT_FOUND với normal search bằng 0. Chưa thể kết luận chỉ từ ảnh rằng object có DontSave hay toàn bộ UI là pure Lua, nên .9 xét cả loaded runtime objects và registry UI đã được xác minh current Shop. Báo cáo có normal/all/UIControllers/registry/currentShop theo từng đường và tên hằng enum Shop nếu cần chẩn đoán; không sao chép giá trị ID. Test dựng đúng trường hợp normal finder rỗng nhưng Resources hoặc private PanelManager cache chứa Shop đang dùng. Getter current panel là hàm nghiệp vụ duy nhất được gọi để kiểm tra context; các hàm OpenPanelWithContext/CheckPanelOpen/helper chỉ bị đọc metadata/upvalue, không được thực thi. Còn cần xác minh trong game thật.

## Chuyển mục tiêu preview.10

Người dùng yêu cầu bỏ qua Shop để thử nhiệm vụ khác. Bản này thay resource command 405 bằng daily_tower.lua, không tiếp tục đường dò UI Shop. Trạng thái auto_daily_ready vẫn false; daily_supported_actions chỉ ghi tower_daily_reward_request. Bộ test active dùng mock Tower model trên Lua VM thật, kiểm tra đúng self/arity/class, một lần thử, status refresh, không gọi UI/Shop/battle và không xuất giá trị reward state. Mã/test Shop cũ không chạy trong test mặc định và không nằm trong runtime resource.
