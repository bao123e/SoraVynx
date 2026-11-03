# SSTool — mã nguồn Windows x64

Menu **Player**, **Event**, **Custom BG** không có số thứ tự. Player và Event độc lập; mọi chỉnh giao diện nằm riêng trong Custom BG. Hàng chức năng dùng toàn bộ chiều ngang, các ô Damage/Vacuum/Score nằm ngay trên hàng tương ứng; không giữ khoảng trống của banner cũ. Cửa sổ không có thanh tiêu đề Windows, bo góc; kéo vùng đầu cửa sổ để di chuyển. **Hide/F1** ẩn bảng và giữ chức năng hoạt động; **X/Alt+F4** tắt các chức năng rồi ẩn bảng. Mở lại EXE hoặc F1 để hiện lại.

**Custom BG:** Choose image chọn PNG/JPG/BMP, Remove bỏ ảnh, Image fit đổi Full image/Fill window. Ba điều chỉnh 0–100% độc lập:

- **Image opacity:** độ hiện của ảnh nền.
- **Panel opacity:** nền thanh bên, vùng đầu, bảng chức năng và trạng thái (mặc định 35%).
- **Button opacity:** nền nút, ON/OFF và ô nhập (mặc định 55%).

Nền bảng/nút được giữ lại; 0% cho trong suốt, 100% cho nền đặc. Chữ Inter và bóng viền không bị giảm opacity. Chuyển trang hoặc chỉnh opacity không đổi trạng thái tính năng. Mặc định không có ảnh và không có banner/ảnh nhúng. Fill window phủ kín, có thể cắt mép; Full image giữ trọn ảnh, có thể có dải trống.

Đường dẫn ảnh, ba mức opacity và chế độ Fit/Fill lưu vào mục Appearance trong config.ini cạnh EXE/DLL; giữ nguyên mục Config của launcher. Cần giữ ảnh ở đường dẫn đã chọn cho lần mở sau. Ảnh thiếu/hỏng sẽ dùng nền tối và có thể chọn lại. GDI+ dùng codec có sẵn của Windows; ảnh tối đa 8192 pixel mỗi cạnh và 32 megapixel. Không tạo file log.

Giữ backend và phím F1/F6/F7/F8. Damage x2–x50, khoảng cách Vacuum 1–10, điểm event x2–x100. Tab/Shift+Tab di chuyển giữa nút/ô nhập, Space bật/tắt nút đang chọn.

```sh
python source/build.py --zig /path/to/zig-0.14.1
python source/test.py
```

Build xuất DLL và EXE xác minh đúng hash DLL vào source/dist; luôn phân phối cả hai từ cùng một build. 19 nhóm kiểm thử gồm 16 backend mô phỏng, 2 tiện ích launcher và 1 nhóm hình học Fit/Fill. Đã kiểm tra thêm 11 nhóm UI qua Wine: cửa sổ bo góc/kéo thả, font nhúng/độ đậm, menu không số và trang Custom BG riêng, hàng chức năng đủ rộng/ô nhập cùng hàng và vị trí bấm, opacity ảnh/bảng/nút độc lập ở 0–100%, giải mã ảnh, lưu tùy chọn, đổi trang/ẩn hiện, opacity/Fit/Fill và xóa nền/đóng bảng. Các chức năng của bản trước đã được người dùng xác nhận hoạt động; giao diện/nền tùy chỉnh mới cần kiểm tra lại trên Windows/game.

Thiết kế áp dụng [UI/UX Pro Max](https://github.com/nextlevelbuilder/ui-ux-pro-max-skill): bóng viền dưới chữ, focus bàn phím và trạng thái ON/OFF/N/A rõ ràng. Font **Inter** nhúng ba độ đậm Regular/SemiBold/Bold, hỗ trợ tiếng Việt, nạp riêng trong tiến trình; không cần cài font hoặc tải font từ mạng. Segoe UI dự phòng nếu Windows không nạp được font nhúng. Font có giấy phép SIL OFL 1.1 trong assets/fonts và LICENSE.txt. Mã nguồn AGPL-3.0-or-later; giấy phép MinHook BSD kèm gói.
