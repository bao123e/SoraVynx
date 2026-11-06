# Kiểm tra và sao lưu dữ liệu Stella Sora qua USB

Chạy trên Windows có PowerShell 5.1 trở lên. Không yêu cầu root hoặc Python. Lần đầu công cụ tải Android Platform Tools từ trang Google chính thức vào thư mục vừa giải nén.

1. Trên điện thoại, bật **Developer options → USB debugging**. Cắm USB vào PC và chấp nhận thông báo **Allow USB debugging** trên điện thoại.
2. Chỉ kết nối một điện thoại. Giải nén ZIP vào thư mục trên ổ PC còn đủ dung lượng.
3. Chạy **Inspect.cmd** trước. Nó chỉ đọc thư mục dữ liệu ngoài/OBB và in dung lượng, không dừng game, không gỡ/cài APK và không sửa dữ liệu điện thoại. Có thể chạy Inspect trong lúc game đang tải; số đo lúc đó chưa phải dung lượng cuối.
4. Nếu có đủ dữ liệu ngoài để sao lưu: chờ game tải xong, đóng Menu, chọn **Force stop** game trong Settings, rồi chạy **Backup.cmd**. PC nên còn ít nhất 15 GB trống. Công cụ đối chiếu SHA-256 nguồn trước/sau và bản copy PC; sao lưu lớn có thể cần nhiều phút.
5. Chỉ thư mục có **manifest.json** và không còn **INCOMPLETE.txt** mới qua kiểm tra tính toàn vẹn. Không xóa bản gốc khi thư mục còn INCOMPLETE.

Android có thể chặn ADB đọc một số thư mục tùy thiết bị. Công cụ sẽ dừng nếu bị chặn. Chỉ sao lưu được dữ liệu bên ngoài dưới `Android/data/com.YoStarEN.StellaSora` và `Android/obb/com.YoStarEN.StellaSora`; không truy cập được dữ liệu riêng `/data/user/0`, mật khẩu hoặc phiên đăng nhập.

**Sao lưu thành công không chứng minh đã có toàn bộ 12 GB hoặc rằng game sẽ chấp nhận cache sau khi đổi APK.** Cần kiểm tra dung lượng/vị trí dữ liệu thực tế trước khi quyết định thay APK. Bản công cụ này chưa tự khôi phục, không tự gỡ/cài game và không chép file ngược về điện thoại.

Backup được lưu trong `backups` bên cạnh công cụ trên PC. Giữ thư mục đó riêng tư, không đưa toàn bộ lên repo hay gửi qua chat.
