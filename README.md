<p align="center"><img src="source/assets/brand.png" width="112" alt="SoraVynx logo"></p>

# SoraVynx — Stella Sora Toolkit for Windows

SoraVynx is a native Windows x64 toolkit for Stella Sora with Player/Event controls and customizable backgrounds.

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
