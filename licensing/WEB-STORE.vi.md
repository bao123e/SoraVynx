# Web, ví và bot mua key — v0.4.0

## Chủ shop cập nhật bộ đã chạy

1. Ctrl+C ở cửa sổ bot **file 4** và backend **file 5**. Giữ tunnel file 6 đang mở để URL không đổi.
2. Chép nội dung thư mục `licensing/` trong gói Web-Store-Update vào `licensing/` đang dùng; chỉ thay các tệp chương trình và thư mục `web/`. Gói không có `.env`, PEM, `secrets/`, database hoặc header khóa ký. **Không xóa thư mục đang dùng, không tạo lại issuer.**
3. Mở **file 5** một lần. Khi nâng cấp, bộ xử lý tạo bản sao SQLite nhất quán trong `secrets/backups/payments-before-accounts-…sqlite` trước khi thay cấu trúc. Đơn cũ và key được giữ nguyên; ví, tài khoản và sổ giao dịch được thêm mới.
4. Mở lại **file 4**.
5. Mở **`9-Connect-Website.cmd`**. Nó kiểm tra cổng HTTPS đã có phiên bản mới, cập nhật **chỉ** `store-config.json` công khai lên repo website bằng token sẵn có, và đăng ký `/buy` cho khách. Không đưa token, PEM hay database lên web. Đợi GitHub Pages cập nhật.
6. Vào https://soravynx.online/account.html để mở tài khoản. Nếu còn thông báo chờ kết nối, đợi Pages hoặc chạy lại file 9 sau khi kiểm tra file 5 và tunnel.

Khi URL Quick Tunnel đổi: cập nhật bằng file 7, khởi động lại backend file 5 và chạy lại **file 9**. Cần đăng nhập lại trên hostname mới; tài khoản, số dư và đơn vẫn nằm trong cùng database. Để vận hành thường xuyên cần URL HTTPS cố định và máy chủ luôn bật; bản PC/tunnel ngừng phục vụ khi tắt máy hoặc mất mạng. Không có backend thật được chạy trong GitHub Pages.

## Khách mua trên web

- **QR từng đơn:** đăng ký/đăng nhập → chọn gói → QR BIDV → giữ đúng tiền và mã đơn → key hiện ở Đơn hàng & key sau xác nhận và đăng ký.
- **Ví:** tạo QR nạp từ 10.000đ đến 10.000.000đ → ví cộng khi SePay xác nhận → chọn gói và Số dư ví → trừ đúng giá một lần → giao key khi đăng ký xong. Không có nút tự nhận đã nạp hoặc giả lập số dư.
- Lịch sử ví và key gắn với tài khoản, không dựa vào localStorage. Xóa dữ liệu trình duyệt không xóa đơn. Tên đăng nhập 3–32 ký tự ASCII, mật khẩu ít nhất 8 ký tự. Mật khẩu băm scrypt + salt, phiên opaque lưu hash ở máy chủ, cookie Secure/HttpOnly/SameSite. Chưa có xác minh email hay tự khôi phục mật khẩu; hỗ trợ cần đối chiếu chủ tài khoản, không xin mật khẩu.
- Cổng tài khoản/QR mở ở hostname HTTPS của backend để cookie đăng nhập là cookie bên thứ nhất; trang chính và danh mục vẫn tại soravynx.online. Không đặt secret vào JavaScript GitHub Pages, không lưu password/token phiên trong localStorage.

## Khách mua qua Discord

Server: https://discord.gg/J3BevcpAxF . Khách dùng **`/buy plan:…`**, QR và key hiện riêng cho chính người dùng lệnh. Khi tin quá lâu hoặc bot khởi động lại, bot gửi DM đúng người mua, không gửi về tài khoản chủ shop. Bật cho phép DM. `/keycreate` và `/order` vẫn dành cho chủ shop trong đúng server đã cấu hình.

Đơn Discord và tài khoản web chưa tự liên kết; không dùng số dư ví web để mua qua bot. Đây là hai cách mua riêng, cùng danh mục giá và bộ xác nhận SePay. Không hướng dẫn khách dùng lệnh quản trị hoặc nhập token bot.

## Giữ dữ liệu và đối soát

- Không xóa `.env`, `issuer-private.pem`, `payments.sqlite` hay `discord-delivery.sqlite`. Sao lưu **cả** thư mục `secrets/` khi dừng backend/bot; bản SQLite tự tạo lúc nâng cấp không thay thế bản sao khóa ký.
- Webhook trùng ID không cộng ví/cấp key hai lần. Yêu cầu mua bằng ví có mã idempotency; khi lỗi mạng, gửi lại cùng mã sẽ nhận cùng đơn. Nếu đăng ký GitHub lỗi, tiền/đơn giữ trạng thái chờ đăng ký và thử lại cùng key. Không cấp thêm bằng tay trước khi đối soát.
- QR luôn dùng VA BIDV cá nhân đã cấu hình, đối chiếu cả accountNumber gốc và subAccount. Ảnh biên lai không được tự coi là xác nhận. Khoản cũ chuyển thẳng vào tài khoản gốc không tự đồng bộ ngược; chỉ xử lý thủ công sau đối chiếu ngân hàng nhận.
- Hoàn tiền và đổi số dư không tự động. Liên hệ hỗ trợ để xử lý; không gửi payload webhook giả vào cổng đang chạy Live. Các test trong repo dùng issuer/database riêng và publisher mô phỏng, không chuyển tiền thật.

Chưa xác minh giao dịch SePay thật cho bản web mới trong môi trường của chủ shop. Sau cập nhật, kiểm tra đăng ký/đăng nhập, quyền xem đơn và QR đúng VA trước khi cho khách sử dụng. Không phải bản sửa chống ban hoặc cập nhật gameplay.
