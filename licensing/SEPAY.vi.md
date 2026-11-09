# SoraVynx — bot Discord và webhook SePay trên máy tính

Bạn đã tạo bot và tài khoản SePay. Bộ này dùng Node.js **24 LTS** và khóa ký cũ đã khớp tool. Không tạo lại issuer, không gửi PEM, token hoặc .env cho người hỗ trợ.

## Chuẩn bị một lần

1. Giải nén bộ mới **vào một thư mục mới riêng biệt**. **COPY**, không di chuyển/xóa, thư mục `licensing/secrets/` của bộ cũ sang `licensing/secrets/` của bộ mới. Giữ bộ cũ làm bản sao lưu. Nếu đã có `.env`, copy riêng file này sang thư mục `licensing/` mới. Không chạy `1-Setup-Issuer.cmd`. Bộ v0.3.2 dùng khóa công khai vừa xuất từ PEM bạn đang giữ.
2. Cài Node.js 24 LTS tại https://nodejs.org/ nếu chưa có. Kiểm tra bằng `node --version` trong Terminal.
3. Mở `licensing/3-Setup-Bot.cmd` và làm theo từng dòng. Bot Token lấy ở https://discord.com/developers/applications → chọn ứng dụng → Bot. Không dán Client Secret thay cho Bot Token.
4. Bật Developer Mode trong Discord → sao chép Server ID và User ID của bạn. Chương trình tự lấy Application ID, tạo link mời bot và đăng ký lệnh. Bot không cần quyền Administrator hoặc Message Content Intent; tài khoản chủ dùng lệnh cần quyền Administrator theo mặc định của lệnh.
5. Tạo fine-grained GitHub token chỉ cho `bao123e/soravynx.github.io`, Contents **Read and write**. Tên token, mật khẩu ngân hàng, token SePay User API không thay thế token này. Bộ cài kiểm tra truy cập repo; quyền ghi được xác nhận khi đăng ký key đầu tiên.
6. Nhập số tài khoản BIDV **đã liên kết SePay**, gồm chữ số. Chưa có link backend thì Enter để trống. Bộ cài tự sinh hai khóa riêng cho webhook và quản trị đơn.
7. Mở `4-Start-Bot.cmd`. Chương trình tự cài thư viện lần đầu. Đợi `SoraVynx owner bot ready`. Thử `/keycreate plan:7 ngày` trong server đã chọn: mã chỉ hiện cho bạn; gửi riêng cho khách sau khi tự kiểm tra tiền. Cách này chưa cần webhook.

## Mở webhook khi chưa có VPS

1. Mở `5-Start-Webhook.cmd`, giữ cửa sổ mở. Backend chỉ lắng nghe `127.0.0.1:8787`; không cần mở port router.
2. Tải Cloudflare Tunnel từ link chính thức https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-windows-amd64.exe . Đổi tên thành `cloudflared.exe`, đặt trong `licensing/`, mở `6-Start-Tunnel.cmd`.
3. Cloudflare in ra link dạng `https://...trycloudflare.com`. Mở `7-Set-Public-URL.cmd`, dán link đó. Chương trình hiện **URL webhook** và **API Key webhook** để bạn điền vào SePay. Không gửi ảnh có API Key.
4. Trong SePay, thêm Webhook cho đúng tài khoản BIDV: sự kiện tiền vào, phương thức POST, URL vừa hiện có đuôi `/webhooks/sepay`, xác thực **API Key**, giá trị là `SEPAY_WEBHOOK_API_KEY` vừa hiện. Không chọn Không xác thực. Không dùng API token quản lý SePay ở ô này. Nếu có thiết lập tách mã đơn, dùng tiền tố `SVX`; webhook cũng đối chiếu mã đầy đủ trong nội dung chuyển khoản.
5. Dừng cửa sổ backend bằng Ctrl+C rồi mở lại `5-Start-Webhook.cmd` để nhận URL mới. Giữ bot, webhook và tunnel cùng chạy.
6. Trong Discord dùng `/order plan:7 ngày`. Bot hiện ảnh QR ngay trong Discord, cùng số tiền, nội dung và nút **Mở đơn / nhận key**. Khách có thể quét ảnh QR trực tiếp; gửi **riêng** link trang đơn cho người trả tiền để nhận key sau thanh toán. Link có quyền xem key sau thanh toán, không đăng ở kênh công khai.
7. Trang đơn hiện QR BIDV, số tiền và mã đơn `SVX...`. Sau khi SePay gửi đúng thông báo tiền vào, backend giữ một key duy nhất và đăng ký bản ghi lên website. Khách đợi tại trang đơn để lấy key. Không có nút tự nhận đã trả tiền.

Webhook trả HTTP **201** và JSON `success:true` khi tiếp nhận, kể cả thông báo không khớp đơn. Chỉ cấp key khi đúng khóa xác thực, đúng BIDV/tài khoản, đúng số tiền, một mã đơn duy nhất và giao dịch trong thời hạn 30 phút của đơn. Gửi lặp cùng transaction ID không tạo thêm key. Tiền ra, chuyển thiếu/thừa, sai nội dung, đơn quá hạn hoặc tài khoản phụ không được cấu hình sẽ không tự cấp key; cần chủ xử lý thủ công.

## Trước khi nhận tiền thật

- Code đã có test mô phỏng riêng, **chưa xác minh với tài khoản SePay thật của bạn**. Đầu tiên kiểm tra webhook trong giao diện SePay; sau đó kiểm tra một đơn hợp lệ và việc key vào tool được. Không dùng payload giả hoặc chế độ Test của SePay trỏ vào backend Live đang bán thật. Chế độ mô phỏng có thể gửi payload giống Live.
- Quick Tunnel phù hợp bước thiết lập/thử nghiệm: URL có thể đổi khi mở lại; phải cập nhật bằng file 7, khởi động lại backend và sửa URL trong SePay. Các link đơn cũ trên tên miền cũ cũng không còn truy cập được. Mất mạng, tắt máy, đóng tunnel sẽ ngừng nhận webhook; kiểm tra retry ở SePay trước khi xử lý đơn cũ. Để bán thường xuyên cần host luôn bật và hostname HTTPS cố định.
- Sao lưu **cả thư mục `licensing/secrets/`** khi bot và backend đã dừng. Trong đó có khóa ký, database giao dịch/key và bản sao key cấp thủ công. Không xóa database giữa chừng, không đưa thư mục này lên GitHub. Thử nghiệm không biến đổi hay hoàn tiền giao dịch ngân hàng.
- Token GitHub hết hạn hoặc mất quyền: đơn đã nhận tiền giữ trạng thái “đang đăng ký”; bộ xử lý thử lại cùng key mỗi 15 giây và sau khi khởi động lại. Không cấp lại bằng `/keycreate` cho cùng đơn khi chưa kiểm tra, tránh tạo hai key.
- Website `soravynx.online` hiện vẫn nhận yêu cầu qua Discord. Trang QR thật nằm tại backend do bot `/order` tạo; chưa có URL cố định để nối nút mua tự động trên cửa hàng.

## Các lỗi thường gặp

| Thông báo | Cách xử lý |
|---|---|
| Khóa ký không khớp | Chọn đúng PEM cũ đã tạo key vào được tool, không sinh lại issuer |
| Discord HTTP 401 | Bot Token sai hoặc đã Reset; chạy file 3 để thay token |
| Discord HTTP 403/404 | Bot chưa vào đúng server hoặc sai Server/User ID |
| Không thấy lệnh | Dùng đúng server, tài khoản chủ có quyền Administrator, rồi chạy file 3 hoặc `npm run register` |
| GitHub HTTP 403/404 | Chọn đúng repo website và Contents Read and write; kiểm tra token hết hạn |
| `/order` chưa tạo được đơn | Mở backend, đặt URL HTTPS bằng file 7 rồi khởi động lại backend |
| SePay HTTP 401 | Chọn xác thực API Key; giá trị phải khớp khóa webhook bộ cài sinh |
| SePay thành công nhưng chưa có key | Kiểm tra đúng số tiền, nội dung mã đơn, BIDV/tài khoản và thời hạn; nếu trang đang đăng ký, kiểm tra GitHub token |

Tài liệu chính thức: https://developer.sepay.vn/vi/sepay-webhooks/tich-hop-webhook và https://developer.sepay.vn/vi/sepay-webhooks/tao-qr-va-form-thanh-toan . Hỗ trợ xác thực API Key trong bộ này; chưa triển khai xác thực HMAC/OAuth hoặc hoàn tiền tự động.
