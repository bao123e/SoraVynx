<p align="center"><img src="source/assets/brand.png" width="112" alt="SoraVynx logo"></p>

# SoraVynx — Stella Sora Toolkit for Windows

SoraVynx is a native Windows x64 toolkit for Stella Sora with Player/Event controls and customizable backgrounds.

**[Download v1.1.0 preview.3](https://github.com/bao123e/SoraVynx/releases/tag/v1.1.0-preview.3)** · [Source & build](source/README.vi.md)

- **Player:** God Mode, Damage x2–x50, No Cooldown, Unlimited Energy, Mob Vacuum.
- **Event:** cooldown/energy riêng và Score x2–x100.
- **Custom BG:** PNG/JPG/BMP, Fit/Fill, opacity ảnh/bảng/nút riêng.
- **Auto Daily — chưa tự chạy:** danh sách 12 nhiệm vụ, giữ lựa chọn hiện tại trong game. **Copy setup report v3** lấy thông tin hàm vào clipboard để hoàn tất tích hợp; chưa có backend làm nhiệm vụ.

Giao diện bo góc, logo SoraVynx và font **Be Vietnam Pro** nhúng sẵn. Không đánh số menu; không tạo log.

Đóng game, chạy **SoraVynx.exe** và chọn **StellaSora.exe**. Giữ **SoraVynx.Runtime.dll** cạnh EXE; không trộn với bản cũ. **F1/Hide** ẩn, giữ tính năng; mở lại EXE để hiện bảng. **X/Alt+F4** tắt tính năng rồi ẩn. **F6/F7/F8** điều khiển event.

Repo chính thức: **[bao123e/SoraVynx](https://github.com/bao123e/SoraVynx)**. Repo chứa đầy đủ EXE, DLL, giấy phép và thư mục source; ZIP chỉ ở Releases.

Build Windows x64; 20 nhóm native và 13 nhóm UI qua Wine đạt. Player/Event giữ backend đã được kiểm tra trước đó; Auto Daily còn chờ nối hàm game thật. [AGPL-3.0-or-later, MinHook BSD, font SIL OFL](LICENSE.txt).
