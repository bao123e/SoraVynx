<p align="center"><img src="source/assets/brand.png" width="112" alt="SoraVynx logo"></p>

# SoraVynx — Stella Sora Toolkit

SoraVynx is a Windows x64 toolkit for Stella Sora, with an experimental Android ARM64 Player/Event integration. Both interfaces include custom backgrounds.

## Windows

**[Download v1.1.0 preview.10](https://github.com/bao123e/SoraVynx/releases/tag/v1.1.0-preview.10)** · [Source & build](source/README.vi.md)

- **Player:** God Mode, Damage x2–x50, No Cooldown, Unlimited Energy, Mob Vacuum.
- **Event:** cooldown/energy riêng và Score x2–x100.
- **Custom BG:** PNG/JPG/BMP, Fit/Fill, opacity ảnh/bảng/nút riêng.
- **Auto Daily — tích hợp một phần:** Shop được đặt cố định **SKIP** theo yêu cầu. Nút thử nghiệm mới **Tower reward / status** gửi một yêu cầu nhận thưởng ngày Tyrant’s Tower. Chưa tự chạy cả chuỗi nhiệm vụ.

Để thử Tower: đóng game, cập nhật cả EXE/DLL, vào game và mở Tyrant’s Tower để dữ liệu thưởng sẵn sàng. Đợi READY, đặt **Claim Tyrant’s Tower rewards → USE**, rồi chọn **Tower reward / status**. Tool tự ẩn để game xử lý; **F1** hiện lại tool. Bấm lại nút để xem trạng thái đã copy, không gửi thêm yêu cầu trong phiên game đó. Chưa xác minh thành công server; cần kiểm tra quà trong game. Các nút Copy report vẫn dùng được. Không tạo log.

Giao diện bo góc, logo SoraVynx và font **Be Vietnam Pro** nhúng sẵn. Không đánh số menu; không tạo log.

Đóng game, chạy **SoraVynx.exe** và chọn **StellaSora.exe**. Giữ **SoraVynx.Runtime.dll** cạnh EXE; không trộn với bản cũ. **F1/Hide** ẩn, giữ tính năng; mở lại EXE để hiện bảng. **X/Alt+F4** tắt tính năng rồi ẩn. **F6/F7/F8** điều khiển event.

Repo chính thức: **[bao123e/SoraVynx](https://github.com/bao123e/SoraVynx)**. Repo chứa đầy đủ EXE, DLL, giấy phép và thư mục source; ZIP chỉ ở Releases.

Build Windows x64; 22 nhóm kiểm thử native/Lua và hai test report Windows/Wine đạt; 13 nhóm UI Wine đạt. Player/Event giữ backend đã được kiểm tra trước đó; Tower có lệnh gửi yêu cầu thử nghiệm; Shop bị bỏ qua và 10 nhiệm vụ khác chưa thực hiện. [AGPL-3.0-or-later, MinHook BSD, font SIL OFL](LICENSE.txt).

## Android Player/Event preview

Menu React Native dùng [`@react-native-menu/menu`](https://github.com/react-native-menu/menu), font Be Vietnam Pro và logo SoraVynx. Có Player, Event và Custom BG; **không có Auto Daily**. Backend nằm trong APK game đã tích hợp, không yêu cầu root; app menu riêng không điều khiển được game gốc.

**[Tải bộ APK Android thử nghiệm](https://github.com/bao123e/SoraVynx/actions/runs/42197498869/artifacts/13493142410)** (đăng nhập GitHub; artifact giữ 14 ngày).

[Source Android](mobile/) · [Cài đặt và giới hạn](mobile/INSTALL.vi.md) · [Build APK bằng Actions](https://github.com/bao123e/SoraVynx/actions/workflows/android-preview.yml).

Chỉ dành cho Stella Sora **2.0.0 / 26093014 / ARM64**, Android 8 trở lên. Hai APK trong bộ build cần được cài cùng nhau. Backend kiểm tra hash, chữ ký hàm và ABI trước khi cho bật từng chức năng. Build/kiểm tra cấu trúc và menu không chứng minh hiệu quả Player/Event; khởi động, đăng nhập và gameplay trên điện thoại thật **chưa được xác minh**. APK dùng chữ ký mới; việc gỡ game có thể làm mất dữ liệu và phải tải lại 12 GB. Đọc hướng dẫn trước khi thay game.

Đã qua build release, kiểm tra chữ ký/hash và 12 nhóm kiểm thử host; menu đã chạy trên giả lập Android 15 ở chế độ dọc/ngang, tablet và chữ lớn. Các kiểm tra này không xác minh khởi động/đăng nhập game hoặc hiệu quả Player/Event trên máy thật.
