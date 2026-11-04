<p align="center"><img src="source/assets/brand.png" width="112" alt="SoraVynx logo"></p>

# SoraVynx — Stella Sora Toolkit for Windows

SoraVynx is a native Windows x64 toolkit for Stella Sora with Player/Event controls and customizable backgrounds.

**[Download v1.1.0 preview.6](https://github.com/bao123e/SoraVynx/releases/tag/v1.1.0-preview.6)** · [Source & build](source/README.vi.md)

- **Player:** God Mode, Damage x2–x50, No Cooldown, Unlimited Energy, Mob Vacuum.
- **Event:** cooldown/energy riêng và Score x2–x100.
- **Custom BG:** PNG/JPG/BMP, Fit/Fill, opacity ảnh/bảng/nút riêng.
- **Auto Daily — tích hợp một phần:** **Shop gift / status** gửi một yêu cầu nhận quà Shop miễn phí khi mục Shop đang USE; bấm lại chỉ cập nhật trạng thái. Chưa tự chạy cả 12 nhiệm vụ. **Copy Lua name report** và **Copy API metadata** giữ chức năng chẩn đoán.

Để thử Shop: đóng game, cập nhật cả EXE/DLL, vào màn chính đợi READY rồi chọn Auto Daily → Shop gift / status. Chuyển lại game để nhận callback, sau đó bấm nút lần nữa để xem trạng thái. Mỗi phiên game tối đa một lần gửi; chưa giải mã server success nên cần xác nhận quà trong game. Không mua hàng hoặc tạo log.

Giao diện bo góc, logo SoraVynx và font **Be Vietnam Pro** nhúng sẵn. Không đánh số menu; không tạo log.

Đóng game, chạy **SoraVynx.exe** và chọn **StellaSora.exe**. Giữ **SoraVynx.Runtime.dll** cạnh EXE; không trộn với bản cũ. **F1/Hide** ẩn, giữ tính năng; mở lại EXE để hiện bảng. **X/Alt+F4** tắt tính năng rồi ẩn. **F6/F7/F8** điều khiển event.

Repo chính thức: **[bao123e/SoraVynx](https://github.com/bao123e/SoraVynx)**. Repo chứa đầy đủ EXE, DLL, giấy phép và thư mục source; ZIP chỉ ở Releases.

Build Windows x64; 22 nhóm kiểm thử native/Lua và hai test report Windows/Wine đạt; 13 nhóm UI Wine đạt. Player/Event giữ backend đã được kiểm tra trước đó; Shop có lệnh gửi yêu cầu thử nghiệm; 11 nhiệm vụ còn lại chưa thực hiện. [AGPL-3.0-or-later, MinHook BSD, font SIL OFL](LICENSE.txt).
