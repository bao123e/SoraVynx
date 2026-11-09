# SoraVynx — bộ cấu hình bot và key 15 ký tự (v0.3)

**Bắt đầu tại [SEPAY.vi.md](SEPAY.vi.md).** Đã có bot Discord và SePay: mở `3-Setup-Bot.cmd`, điền theo từng dòng rồi mở `4-Start-Bot.cmd`. Bộ cài kiểm tra đúng khóa ký cũ trước khi lưu cấu hình. Không chạy lại Setup-Issuer.

Có thêm `/order` tạo trang QR BIDV riêng, endpoint `/webhooks/sepay` xác thực API Key, database giữ đơn và chống cấp trùng. Chưa kết nối SePay thật hoặc triển khai backend công khai; cần làm các bước tunnel/host trong hướng dẫn trước khi nhận tiền tự động. Website cửa hàng hiện vẫn dùng liên hệ Discord.

Các ghi chú bên dưới mô tả giao thức key; hướng dẫn cài bot, webhook và giới hạn bản mới nằm trong SEPAY.vi.md.

## Thay đổi

Tool Windows v1.2.0-license-preview.2 chỉ có ô nhập key và nút **Vào tool** ở cửa sổ khóa. Không có mã máy, Copy code hoặc các dòng hướng dẫn. Enter xác nhận, Esc đóng. Cửa sổ tối, viền bo và nút mint cùng phong cách tool.

Key là 15 ký tự ngẫu nhiên, không phân biệt chữ hoa/thường. Bỏ các ký tự I/O/0/1 để dễ đọc. Không nhúng chữ ký dài trong mã người dùng nhập. Tool tra SHA-256 của key qua HTTPS `soravynx.online/licenses/<hash>.lic`, rồi xác minh bản ghi bằng khóa công khai P-256 của chủ tool. Không có private key trong EXE/DLL hoặc JavaScript website.

**Máy cần Internet khi mở tool.** Một mã chưa đăng ký lên website sẽ chưa dùng được, dù đã được tạo ở máy chủ tool. Key được nhớ bằng Windows DPAPI, nhưng mỗi lần mở vẫn tra bản ghi lại. Nếu DNS/mạng/website lỗi, giữ khóa và cho thử lại; không bỏ qua xác thực.

Key mới không ràng buộc máy. Người có mã có thể chia sẻ mã; cần backend có database để giới hạn số thiết bị/kích hoạt. Hạn 7/30/365 ngày hoặc vĩnh viễn bắt đầu từ lúc cấp. Đồng hồ máy và các can thiệp vào binary vẫn là giới hạn; đây không phải hệ thống chống crack.

## Chủ tool tạo key

1. Giải nén Owner Key Tools v0.2 vào thư mục bộ tạo key cũ, thay các tệp mã chương trình; giữ nguyên thư mục `licensing/secrets/`.
2. **Không chạy lại 1-Setup-Issuer.cmd nếu đã tạo khóa và gửi public key.** Giữ/sao lưu `issuer-private.pem`, không gửi nó vào chat hoặc upload GitHub.
3. Mở `licensing/2-Create-Key.cmd`. Chỉ chọn `7`, `30`, `365` hoặc `lifetime`; không cần mã máy khách.
4. Chương trình tạo một key 15 ký tự và một tệp `licensing/public-records/<hash>.lic`. Bản sao key gần nhất nằm trong secrets/last-short-key.txt, không đưa lên repo.
5. Đăng ký tệp .lic theo một trong hai cách dưới đây. Đợi GitHub Pages cập nhật xong rồi đưa key cho khách.

### Đăng ký thủ công (không cần token)

Vào repo https://github.com/bao123e/soravynx.github.io → thư mục `licenses/` → Add file → Upload files → chọn đúng tệp `.lic` vừa tạo → Commit changes. Có thể gửi tệp `.lic` này cho người hỗ trợ triển khai; nó là bản ghi ký số công khai, không phải private key. Chỉ gửi mã 15 ký tự riêng cho khách; không ghi mã đó vào repo.

### Đăng ký tự động bằng bộ tạo key hoặc bot

Tạo fine-grained GitHub token tại https://github.com/settings/personal-access-tokens/new, chỉ chọn repo `bao123e/soravynx.github.io`, quyền Contents: Read and write. Lưu token riêng trong `licensing/.env`:

```dotenv
GITHUB_TOKEN=TOKEN_LUU_RIENG_TREN_PC_HOAC_HOST
```

Không gửi token vào chat. CMD sẽ đọc .env nếu có; sau khi tạo key, chương trình tự đưa bản ghi ký số lên website. Chờ triển khai Pages trước khi dùng. Nếu đăng ký lỗi, bản ghi vẫn ở public-records/ để upload tay. GitHub Pages có độ trễ và giới hạn build; cách này phù hợp giai đoạn cấp thủ công ít đơn, không thay thế API/database thanh toán cho cửa hàng đông khách.

## Bot Discord

Bot có lệnh `/keycreate plan:...`, không cần machine code. Chỉ đúng owner ID ở server cấu hình được cấp key. Bot ký và đăng ký bản ghi rồi trả mã dạng ephemeral cho chủ tool. Chưa tự xác nhận thanh toán hoặc tự DM người mua.

1. Tạo ứng dụng tại https://discord.com/developers/applications, lấy Bot Token/Application ID. Bật Developer Mode trong Discord để lấy Guild ID/User ID.
2. Mời bot với scopes bot + applications.commands; không cần quyền Administrator cho bot hay Message Content Intent.
3. Điền licensing/.env theo .env.example, gồm token GitHub giới hạn repo website và đường dẫn private PEM cũ.
4. Trong thư mục licensing: `npm ci`, `npm run register`, `npm run bot`.
5. Sau khi xác nhận tiền thật, chủ tool dùng `/keycreate plan:...`, chờ website cập nhật rồi gửi mã riêng cho khách.

## QR BIDV thật

Bộ v0.3 có backend mẫu đã kiểm thử mô phỏng, xem SEPAY.vi.md. Kết nối thật chưa được bật. GitHub Pages không nhận webhook hoặc giữ secret được. Luồng tự động cần backend HTTPS + database: tạo đơn và giá ở server → QR BIDV đúng số tiền/mã đơn → SePay/payOS gửi webhook đã xác thực → kiểm tra đúng tài khoản/tiền vào/số tiền/mã đơn và chống giao dịch lặp → cấp một key cho đúng đơn → giao riêng cho người mua.

BIDV/SePay: https://sepay.vn/bidv.html và https://docs.sepay.vn/tich-hop-webhooks.html. Kiểm tra loại tài khoản/gói hỗ trợ với nhà cung cấp. Không cấp key dựa trên ảnh bill hoặc nút “đã thanh toán” của người mua. Khi triển khai thanh toán nhiều đơn, thay việc publish từng bản ghi bằng API có database; cần thêm kiểm soát số máy, thu hồi và giờ server nếu bán key thương mại.

## Key dài phiên bản cũ

SV1 gắn máy vẫn chỉ dùng cho bản v1.2.0-license-preview.1. Bản preview.2 dùng mã ngắn và giao thức SV2; không nhập key dài vào cửa sổ mới. Private/public key ký vẫn giữ nguyên.
