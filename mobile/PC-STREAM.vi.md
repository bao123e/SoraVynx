# Dùng điện thoại với bản game đã có trên PC

Nếu PC đã cài đủ dữ liệu Stella Sora, bạn có thể chạy **game và SoraVynx Windows trên PC**, rồi xem/điều khiển từ điện thoại qua mạng nội bộ. Điện thoại không cần tải dữ liệu game 12 GB cho cách này. Đây là bản Windows truyền hình ảnh sang điện thoại, không phải kiểm chứng backend Android.

## Phần mềm chính chủ

- Android chưa root: [Moonlight 12.2, APK khoảng 11 MB](https://github.com/moonlight-stream/moonlight-android/releases/download/v12.2/app-nonRoot-release.apk).
- PC Windows x64: [Sunshine, bộ cài khoảng 34 MB](https://github.com/LizardByte/Sunshine/releases/download/v2026.914.233613/Sunshine-Windows-AMD64-installer.msi).
- Trang dự án: [Moonlight](https://moonlight-stream.org/) và [Sunshine](https://github.com/LizardByte/Sunshine).

## Thiết lập

1. Kết nối PC và điện thoại vào cùng mạng nội bộ. PC nối dây mạng tới router thường ổn định hơn. Luồng hình ảnh chạy trong mạng nội bộ, không tải lại dữ liệu game qua Internet lên điện thoại.
2. Cài Sunshine trên PC. Mở trang quản trị cục bộ do Sunshine hiển thị và đặt tài khoản/mật khẩu riêng cho máy chủ.
3. Cài Moonlight trên điện thoại, chọn PC được tìm thấy. Nếu không thấy, thêm địa chỉ IPv4 nội bộ của PC (xem bằng `ipconfig`). Nhập PIN ghép đôi mà Moonlight hiển thị vào trang Sunshine trên PC.
4. Chọn **Desktop** trong Moonlight. Trên PC, mở game và SoraVynx Windows theo cách bạn vẫn sử dụng. Điện thoại điều khiển phiên Desktop đó, gồm các nút của tool Windows.
5. Bắt đầu với **720p, 30 FPS, bitrate 3 Mbps** trong Moonlight. Điều chỉnh theo chất lượng kết nối Wi-Fi nội bộ. Internet chậm và Wi-Fi nội bộ chậm là hai giới hạn khác nhau; game vẫn cần kết nối máy chủ để đăng nhập/chơi.

PC phải bật trong lúc sử dụng. Chỉ áp dụng nếu PC có đủ chỗ và đã có/đã tải được dữ liệu game. Cách này không làm bản Android preview.1 thành bản đã sửa, không dùng chung cache Windows/Android và không yêu cầu thay APK game đang cài trên điện thoại.
