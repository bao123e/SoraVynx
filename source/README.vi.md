# SSTool — mã nguồn Windows x64

Hai trang **Player / Event** độc lập. Ảnh nhúng hiển thị trọn khung, giữ tỷ lệ và không bị chữ/nút che; logo SSTool lớn, chữ sáng trên nền tối. Tab/Shift+Tab di chuyển giữa các nút và ô nhập, Space bật/tắt nút đang chọn.

Giữ backend Player, Bouncing Bumper Bash và các phím F1/F6/F7/F8. Damage x2–x50, khoảng cách Vacuum 1–10, điểm event x2–x100 (mặc định x2). Không tạo file log; launcher chỉ lưu đường dẫn game vào config.ini.

```sh
python source/build.py --zig /path/to/zig-0.14.1
python source/test.py
```

Build tạo DLL nhúng ảnh và EXE xác minh đúng hash DLL, xuất vào source/dist. Luôn phân phối hai tệp cùng một build. Kiểm thử gồm 16 nhóm backend mô phỏng, 2 tiện ích launcher và 1 nhóm kiểm tra tỷ lệ/khung ảnh. Cross-build không thay thế kiểm tra Windows/game thực tế; event x2 đã được người dùng xác nhận trước đó, mức cao hơn chưa được xác nhận.

Thiết kế áp dụng [UI/UX Pro Max](https://github.com/nextlevelbuilder/ui-ux-pro-max-skill): tương phản chữ tối thiểu 4.5:1, focus bàn phím rõ, trạng thái ON/OFF/N/A. Giao diện dùng Win32/GDI và Segoe UI, không cần tải font hay ảnh ngoài. Ảnh Stella Sora từ tài nguyên người dùng cung cấp, quyền thuộc chủ sở hữu tương ứng. Mã nguồn AGPL-3.0-or-later; MinHook dùng BSD, giấy phép kèm gói.
