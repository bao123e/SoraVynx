# SSTool standalone 2 — chọn đường dẫn và mở game

## Dùng

1. Đặt SSTool.exe và MG_cheat.dll cùng một thư mục. Thay SSTool.exe bằng bản mới; DLL standalone 1 vẫn dùng được.
2. Có thể đặt config.ini hiện có cạnh SSTool.exe. Chương trình đọc GamePath trong mục [Config], hỗ trợ đường dẫn có dấu gạch chéo lặp của file đã gửi. Không sử dụng DLLPath_2 hoặc key trong file cấu hình.
3. Mở SSTool.exe. Dán đường dẫn StellaSora.exe hoặc bấm Browse để chọn file.
4. Bấm Start game + tool. Đường dẫn được lưu vào config.ini cạnh EXE để dùng lần sau.
5. Nếu game chưa chạy, chương trình mở game với thư mục làm việc là thư mục game, chờ GameAssembly.dll tối đa 120 giây rồi nạp DLL. Nếu game đang chạy, chương trình nạp vào phiên đang chạy. Hai phiên cùng tên sẽ bị từ chối.
6. Chờ menu công cụ hiện. F1 ẩn/hiện. Khởi động lại game trước nếu phiên hiện tại đã nạp DLL.

Trình nạp mới không có bước nhập key, không sử dụng Injector.exe cũ. Nếu game cần official launcher để đăng nhập/khởi động, mở bằng launcher chính thức trước rồi bấm Start game + tool. Chương trình không điều khiển màn hình đăng nhập game.

Đường dẫn cấu hình cá nhân không nằm trong bản phát hành. File mới lưu dạng Unicode INI. Ví dụ cấu trúc (chọn đường dẫn thật bằng Browse):

```ini
[Config]
GamePath=C:\Games\StellaSora_EN\StellaSora.exe
```

Đã build Windows GUI x64, kiểm tra bộ chuẩn hóa đường dẫn với sanitizer (gạch chéo lặp, khoảng trắng/dấu nháy, UNC và rỗng), và chạy lại 13 nhóm mock tests backend. Giao diện, CreateProcess và nạp DLL thực chưa được chạy trên Windows tại môi trường build.

## Tính năng và giới hạn

- God Mode: đặt sát thương dương về 0 ở AdventureActor.RealDamage khi đối tượng nhận là PlayerAdventureActor; các đường mất máu khác và trạng thái khống chế không được bảo đảm.
- Damage Multiplier: nhân damage dương lên MonsterAdventureActor, hệ số **2–50**. Không lặp đòn. Tràn int64 được chặn. Khi rời ô nhập, số ngoài giới hạn được đưa về 2–50; nhãn cạnh ô luôn hiện hệ số đang dùng. Mức 50 là giới hạn của công cụ, không phải giới hạn damage đã xác minh của game.
- No Cooldown: gọi ResetAllSkillCD() trước UpdateSkillCD(TrueSync.FP), có chặn gọi lồng; chưa bảo đảm áp dụng mọi loại cooldown.
- Unlimited Energy: thay giá trị setter currentEnergy bằng totalEnergy qua FDP 8 byte. Người dùng đã xác nhận có tác dụng với bản frame trước.
- Mob Vacuum: trước DriveUpdate, gọi getter/setter vị trí qua runtime tối đa mỗi 100 ms; chỉ xử lý quái còn sống.
- Instant Break: tắt theo yêu cầu.

GameAssembly phải khớp SHA-256 6adc5de8d2ed85ee752be80b38aff0e5e85d21e6efb6133c262c9f2632e06dd8. EnterFrame tại RVA 0x64e1c0 còn được kiểm tra prologue. Không sửa hash để ép chạy bản game khác.

## Kiểm chứng

Log người dùng của frame test 2 ghi energy=9656, god_zero_damage=35, damage_scaled=3643, cooldown_resets=18969, monsters_moved=2800. Các số là số lần xử lý trong code, không chứng minh mọi hiệu ứng cuối cùng đều đúng. Người dùng báo gần hết tính năng có tác dụng. Log cũ không ghi hệ số damage; nhập 20000 giữ hệ số hợp lệ trước đó, không chứng minh x20000 hoạt động.

Standalone 2 giữ backend gameplay, chỉnh giao diện giới hạn và bổ sung trình nạp mới. DLL/EXE đã build Windows x64; 13 nhóm mock tests của backend đạt với ASan/UBSan. GUI mới và trình nạp chưa chạy trên Windows trong môi trường build. Xem SSTool2.log cạnh DLL nếu khởi động dừng; log mới có SETTING damage_multiplier và FRAME ACTIONS.

## Build và giấy phép

python runtime2/build.py --zig /path/to/zig-0.14.1
python runtime2/test.py

Mã nguồn đầy đủ trong runtime2 của nhánh runtime2-test hoặc gói ZIP tương ứng; build.py sinh cả SSTool.exe và MG_cheat.dll. Giấy phép AGPL-3.0-or-later (LICENSE-AGPL.txt), MinHook BSD (runtime2/vendor/minhook/LICENSE.txt).

Nguồn tham khảo: TheReVeaLz/StellaSora-Tool commit 30f67811411dca754f1b876b4d4dbf6a001e0a20; MinHook commit 8af6b4acae5a9388fd742b56fa79ece89d96f823. MetadataDecryption của TheReVeaLz được dùng đối chiếu dữ liệu, không link vào DLL.
