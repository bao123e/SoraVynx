# Stella Sora Runtime 2 — diagnostics 3

**Bản chẩn đoán, chưa xác nhận gameplay hoạt động.** Sửa tên đầy đủ của tham số RealDamage và theo dõi entry chung của bộ thông dịch tại RVA 0x64e270, chỉ sau khi SHA-256 GameAssembly khớp profile. Entry được kiểm tra prologue trước khi cài hook. Probe chuyển nguyên MethodInfo, argument stack và return storage cho hàm gốc; không tự diễn giải hoặc sửa các đối số.

Log ghi INTERPRETER calls và INTERPRETER TARGET khi phương thức mục tiêu đi qua entry này. Không thấy target ở đây cũng chưa chứng minh phương thức không chạy: các lời gọi nội bộ có thể đi qua frame khác. Không dùng probe này làm bằng chứng tính năng đã hoạt động.

Chạy một trận 30–60 giây, dùng kỹ năng rồi gửi SSTool2.log cạnh DLL. Không cần gửi lại tệp game. Bản Windows x64 đã build; 9 nhóm mock tests với ASan/UBSan đã đạt. Windows/game runtime chưa được kiểm thử tại môi trường build.

## Cách thử trên máy bạn

1. Thoát hoàn toàn game.
2. Sao chép thư mục công cụ cũ sang một thư mục mới để giữ bản gốc. Trong **bản sao**, thay riêng `MG_cheat.dll` bằng DLL trong gói này. Giữ Injector và các thư viện của bạn; gói này không kèm injector.
3. Dùng trình nạp DLL hiện có, chỉ nạp **một** bản MG_cheat.dll mới. Không nạp đồng thời cheat cũ, probe hoặc một bản DLL mới thứ hai. Giữ nguyên khóa và các thiết lập xác thực của trình nạp; không cần gửi chúng.
4. Chờ game tải tới menu chính. Cửa sổ điều khiển riêng sẽ hiện; **F1** ẩn/hiện cửa sổ khi game đang được chọn. Đây là cửa sổ Windows riêng, không phải menu ImGui nằm trong game.
5. Tất cả tính năng ban đầu đều **tắt**. Ô bị mờ nghĩa là backend chưa tìm được đủ hàm/trường dữ liệu tương ứng. Vào màn chơi rồi thử **từng tính năng một**.
6. Nếu không có tác dụng, ô bị mờ, hoặc startup dừng: gửi `SSTool2.log` cạnh DLL. Màn hình ghi cả số tính năng có hook và số phương thức thực sự đã được game gọi. Không cần gửi config chứa khóa.

Đóng cửa sổ bằng nút X hoặc bấm Disable all sẽ tắt mọi tính năng. F1 chỉ ẩn/hiện và không tự tắt chúng. Khởi động lại game trước khi thay hoặc bỏ DLL; bản này không hỗ trợ unload nóng.

## Các tính năng được triển khai

| Tính năng | Backend mới |
|---|---|
| Instant Break | Dùng getter toughness hiện tại và chữ ký damage 64-bit; chỉ áp dụng đòn từ PlayerAdventureActor. |
| God Mode | Lọc lớp player ở RealDamage và các OnHitActor đã đối chiếu. |
| Multi Hit | Lặp OnHitActor với attack ID mới; chỉ áp dụng lên monster, có chặn nhân lặp khi các hook gọi lồng nhau. |
| No Cooldown | Lọc đúng MethodInfo của ReduceSkillSection và ResetUseInterval. |
| Unlimited Energy | Giữ ClearEnergy và sao chép tổng năng lượng TrueSync.FDP qua runtime; không dùng phép dịch bit FP cũ. |
| Mob Vacuum | Chạy sau DriveUpdate; gọi getter/setter và phép toán vector qua runtime, bỏ qua player và quái đã chết. |

Các biến thể tùy chọn Crit Rate, Player Speed và Aggro Only không được port trong bản này. Chưa có tính năng nào trong bảng được xác nhận bằng kiểm thử game thực tế.

## Giới hạn quan trọng

- Chỉ khởi động backend khi SHA-256 GameAssembly.dll khớp bộ tệp đã phân tích: `6adc5de8d2ed85ee752be80b38aff0e5e85d21e6efb6133c262c9f2632e06dd8`. Nhãn game 2.0.0 do người dùng cung cấp; hash mới là định danh chính xác của profile này. Không đổi hash để ép chạy một binary khác.
- Mỗi phương thức được chọn bằng tên, chữ ký đầy đủ và loại instance/static. Các trường được đọc bằng API reflection, không dùng offset đối tượng cũ.
- MethodInfo v31 được đọc để lấy entry candidate. Các entry dùng chung được nhóm theo ABI; nhóm có xung đột ABI sẽ không bị hook. Các callback không khớp MethodInfo luôn gọi trampoline gốc.
- HybridCLR có thể gọi nội bộ trực tiếp trong bộ thông dịch, không đi qua native entry đã hook. Nếu số target calls vẫn là 0 khi chơi, hook đó chưa được chứng minh hữu hiệu; cần log để điều chỉnh cơ chế dispatch. Việc checkbox bật được không phải bằng chứng tính năng đã hoạt động.
- Gói không sửa khóa/HWID, dịch vụ cấp phép, anti-cheat hoặc tệp game. Không có kết nối mạng trong backend mới.
- `lua.arcx` vượt giới hạn tải 32 MiB và chưa được phân tích; nó không được sử dụng trong build này.

## Bằng chứng kiểm tra

- DLL Windows PE32+ x64 được build bằng Zig 0.14.1 với cảnh báo của mã mới được coi là lỗi.
- 8 nhóm kiểm thử callback thực bằng mock đã đạt với AddressSanitizer và UndefinedBehaviorSanitizer: mặc định tắt/passthrough, cooldown, FDP và fallback size, toughness 64-bit, god mode lọc lớp, multihit lồng nhau, tính năng không khả dụng, và luồng vacuum qua runtime API.
- Công cụ MetadataDecryption upstream được kiểm tra với dữ liệu đã gửi: đầu ra trùng SHA-256 `7fcb4180a8cc0fa42302bacfb9690235e4abc7a40e0733dce43407a3fd28c58b` với bản đã giải mã trước đó.
- Windows loader, GUI, hook/trampoline thật và kết quả gameplay: **chưa được chạy/kiểm chứng trong môi trường này**.

## Build và kiểm thử lại

Cần Python 3 và Zig 0.14.1 cho Windows build; GCC hỗ trợ sanitizer cho native mock tests:

```sh
python3 runtime2/build.py --zig /path/to/zig
python3 runtime2/test.py
```

Không cần mã nguồn game, thư viện VC runtime riêng, Visual Studio hoặc các tệp game để biên dịch DLL này. Game và hotfix chỉ cần thiết cho kiểm thử runtime.

## Nguồn và giấy phép

- [TheReVeaLz/StellaSora-Tool](https://github.com/TheReVeaLz/StellaSora-Tool), commit `30f67811411dca754f1b876b4d4dbf6a001e0a20`, AGPL-3.0. Các tính năng được viết lại cho runtime thay vì giữ địa chỉ và layout cũ.
- [TheReVeaLz/StellaSora-MetadataDecryption](https://github.com/TheReVeaLz/StellaSora-MetadataDecryption), commit `8250d1bb9fa472b8d8ea0da1513649f812ca2d5b`, GPL-3.0. Được dùng để đối chiếu dữ liệu; không link vào DLL.
- [TsudaKageyu/MinHook](https://github.com/TsudaKageyu/minhook), commit `8af6b4acae5a9388fd742b56fa79ece89d96f823`, giấy phép BSD trong `vendor/minhook/LICENSE.txt`.

Mã nguồn backend và build scripts đi kèm gói nhị phân, theo AGPL-3.0-or-later. Gói chẩn đoán và mã nguồn được cung cấp trên nhánh runtime2-test.
