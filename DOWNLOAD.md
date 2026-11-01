# Stella Sora Runtime 2 — frame test 1

[Tải ZIP frame test 1](releases/StellaSora-Runtime2-frame-test1.zip)

Bản thử đổi cơ chế sang hook frame của lời gọi managed nội bộ. Triển khai thay đối số energy setter và toughness damage. Bốn tính năng còn lại bị tắt. **Chưa xác nhận hai tính năng này hoạt động trong game.**

Build Windows x64 và 10 nhóm mock tests với sanitizer đã đạt. [Hướng dẫn và giới hạn](runtime2/README.vi.md).

Thoát game, thay DLL trong bản sao công cụ rồi thử hai tính năng lần lượt. Gửi SSTool2.log sau khoảng 30–60 giây trong trận.

SHA-256 ZIP: `975ce8180c117ea7253b06cef10b887635512a5f0e0aba7783f6e1d13c1d39ed`.

Gói chứa DLL, mã nguồn và giấy phép; không chứa game, Injector, log cá nhân hoặc khóa.
