# Stella Sora Runtime 2 — chẩn đoán lần 2

[Tải ZIP diagnostics 2](releases/StellaSora-Runtime2-diagnostics2.zip)

**Chưa sửa được lỗi tính năng không tác dụng.** Gói này bổ sung số lượt vào entry, số phương thức khớp và chữ ký hàm thực tế để xác định nguyên nhân tiếp theo.

Thoát game, thay DLL trong bản sao thư mục công cụ, chạy một trận 30–60 giây rồi gửi SSTool2.log cạnh DLL. Xem [hướng dẫn](runtime2/README.vi.md).

Build Windows x64 và 8 nhóm mock tests với sanitizer đã đạt. Chưa kiểm thử bản mới trong game.

SHA-256 ZIP: `1f3f9eb96d242b6fb9a858e7de5622878473f7054cda7868fd83c76c1f4cf8b2`.

Gói có DLL, mã nguồn và giấy phép; không chứa tệp game, log cá nhân, khóa hoặc Injector.
