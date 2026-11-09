# SoraVynx — key truy cập, bot Discord và QR BIDV

## Trạng thái của bản đầu

Đã thêm **license gate offline ký số P-256, ràng buộc mã máy** cho source Windows. Launcher yêu cầu key trước khi chọn/chạy game; DLL cũng kiểm tra riêng trước khi tạo UI hoặc khởi tạo tính năng. Key sai chữ ký, bị sửa, hết hạn hoặc cấp cho máy khác bị từ chối. Khi hết hạn trong phiên, UI tắt các lựa chọn và đóng. Activation được Windows DPAPI bảo vệ trong `%LOCALAPPDATA%\SoraVynx\license.dat`; đó là dữ liệu license, không phải log.

Bản này không tự liên hệ ngân hàng, không tự thu tiền/cấp key từ website, không có thu hồi key online. Thời hạn offline dựa trên đồng hồ hệ điều hành, có kiểm tra lùi đồng hồ trong phiên và dùng thời gian monotonic; không thay thế giờ máy chủ đáng tin. Người có khả năng sửa binary/source hoặc thay đổi hệ thống có thể vượt bảo vệ local. Không quảng cáo đây là hệ thống chống crack hoặc license thương mại có thu hồi từ xa.

Repo hiện có mã nguồn mở AGPL; phải tiếp tục tuân thủ giấy phép khi phân phối. Bản EXE/DLL cũ đã phát hành không tự bị khóa; gate chỉ nằm trong bản build mới. Không có private issuer key hoặc key khách hàng trong Git.

## A. Thiết lập người cấp key (làm trên PC riêng của bạn)

1. Cài Node.js bản LTS 22 trở lên từ https://nodejs.org/.
2. Tải/clone source nhánh `feature/license-gate`. Trong thư mục gốc repo, chạy:

```powershell
node licensing/issuer.mjs init
```

Lệnh tạo:

- `licensing/secrets/issuer-private.pem`: **khóa bí mật**, giữ riêng và sao lưu. Không upload GitHub, gửi khách, chụp màn hình nội dung hoặc đưa vào ZIP tool.
- `licensing/secrets/issuer-public.pem`: khóa công khai.
- `source/license_public_key.h`: khóa công khai dành cho tool.

Lệnh không ghi đè bộ khóa đã tồn tại. Nếu mất private key, không thể tạo key mới tương thích bộ tool cũ; nếu đổi bộ khóa, key cũ không còn được bộ tool mới xác nhận.

3. Chỉ đưa **source/license_public_key.h** đã tạo lên nhánh source tương ứng trên GitHub. File này là public, không phải secret. Build lại cả launcher và DLL bằng CI hoặc Zig 0.14.1:

```powershell
python source/build.py --zig zig
```

Trong Actions → Verify Windows license gate, khi source có issuer public key hợp lệ, artifact customer build sẽ chứa EXE/DLL tương ứng; chưa có xác minh gameplay cho bản build này. Build mặc định có SV_LICENSE_CONFIGURED=0 sẽ giữ tool khóa và không tạo artifact khách có thể dùng.

4. Khách mở bản tool mới → Copy code → gửi mã máy 64 ký tự cho bạn qua Discord.
5. Sau khi bạn xác nhận thanh toán thật, tạo key:

```powershell
node licensing/issuer.mjs issue --machine MA_MAY_64_KY_TU --plan week
```

Plan: `week` = 7 ngày, `month` = 30 ngày, `year` = 365 ngày, `lifetime` = không có hạn cuối. **Thời hạn bắt đầu lúc cấp key**, không phải lúc khách nhập key. Key dài do chứa chữ ký, khách sao chép toàn bộ SV1.… vào cửa sổ kích hoạt. Không thể tùy ý sửa hạn trong key.

6. Gửi key riêng cho đúng khách. Khách nhập → Activate. Lần mở sau tool đọc activation DPAPI và kiểm tra lại; không lưu key vào config.ini hoặc log.

Để bán key có kiểm tra trạng thái/thu hồi/thời gian đáng tin, cần giai đoạn backend online thay vì chỉ gate offline này.

## B. Bot Discord cấp key bằng lệnh của chủ tool

Bot source đã có ở `discord-bot.mjs`; bản đầu dùng **xác nhận tiền thủ công**, không coi việc gọi lệnh là bằng chứng ngân hàng.

1. Mở https://discord.com/developers/applications → New Application → Bot → tạo token. Lưu token trong môi trường riêng của bot, không gửi chat và không commit.
2. Lấy Application ID, Server/Guild ID và User ID của bạn (Discord Settings → Advanced → Developer Mode). Mời bot bằng OAuth2 URL Generator với scopes `bot` và `applications.commands`. Không cần Message Content Intent hay quyền Administrator cho bot.
3. Trong thư mục licensing:

```powershell
npm ci
Copy-Item .env.example .env
```

Điền `.env` trên PC/VPS riêng:

```dotenv
DISCORD_BOT_TOKEN=TOKEN_LUU_RIENG
DISCORD_APPLICATION_ID=APPLICATION_ID
DISCORD_GUILD_ID=SERVER_ID
DISCORD_OWNER_ID=USER_ID_CUA_BAN
LICENSE_PRIVATE_KEY_FILE=./secrets/issuer-private.pem
```

4. Đăng ký slash command và chạy bot:

```powershell
npm run register
npm run bot
```

5. Sau khi tự kiểm tra tiền vào BIDV, bạn gọi `/keycreate machine:... plan:...`. Chỉ đúng DISCORD_OWNER_ID ở đúng server cấu hình mới dùng được, kể cả người khác có quyền admin server. Key trả về dạng ephemeral chỉ người gọi nhìn thấy; tự gửi riêng cho khách. Bot cần chạy liên tục trên PC hoặc VPS; khóa riêng lưu ở host, không có trên website.

Chưa có chức năng tự duyệt bill, DM khách, nhận webhook thanh toán hoặc cấp key khi người dùng tự bấm “đã chuyển”. Các bước đó thuộc phần C.

## C. QR BIDV có xác nhận tiền thật và tự cấp key

Số tài khoản và tên người nhận chưa được cấu hình. Không có QR thu tiền thật, cổng thanh toán hoặc bot live được bật trong lần thay đổi source này.

### Luồng đúng

Khách chọn gói + nhập mã máy → backend tạo đơn và chốt giá → hiển thị QR BIDV chứa đúng số tiền/mã đơn → khách chuyển tiền → ngân hàng/dịch vụ gửi webhook đã xác thực → backend đối chiếu → chỉ khi đúng mới tạo một key → hiển thị cho chủ đơn hoặc bot gửi riêng cho khách.

GitHub Pages chỉ phục vụ HTML/JS, không chạy được private signing code, nhận webhook hay giữ secret an toàn. Giữ website soravynx.online, thêm backend HTTPS riêng, ví dụ api.soravynx.online trên VPS hoặc nền tảng có database. Không nhét khóa ký/token SePay vào js/app.js.

### Thiết lập BIDV qua SePay

1. Đăng ký tại https://my.sepay.vn/ và liên kết tài khoản BIDV theo hướng dẫn chính thức https://sepay.vn/bidv.html. Kiểm tra đúng loại tài khoản, gói dịch vụ và điều kiện hỗ trợ của SePay/BIDV trước khi bật thu tiền. Không cần cung cấp thông tin đăng nhập BIDV cho người viết tool.
2. Tạo webhook theo https://docs.sepay.vn/tich-hop-webhooks.html, chọn đúng tài khoản BIDV, chỉ giao dịch tiền vào, endpoint HTTPS của backend và phương thức xác thực có cấu hình secret riêng.
3. Backend tạo đơn từ bảng giá phía server: 29.000 / 79.000 / 499.000 / 999.000 VNĐ. Không nhận giá/hạn license do JavaScript của khách gửi làm giá chuẩn.
4. Sinh mã đơn khó đoán, có hạn thanh toán. Với API VA theo đơn hàng BIDV, dùng tài khoản định danh riêng cho đơn theo https://docs.sepay.vn/api-va-theo-don-hang-bidv.html nếu tài khoản/gói của bạn được hỗ trợ. Nếu dùng nội dung chuyển khoản, đối chiếu mã đơn chính xác.
5. Ở webhook: kiểm tra xác thực; đúng BIDV/tài khoản nhận; tiền vào; đúng VND/số tiền/mã đơn; đơn chưa hết hạn/chưa xử lý. Dữ liệu JSON/ảnh bill từ trình duyệt không phải bằng chứng thanh toán.
6. Dùng transaction database và unique transaction ID: webhook bị gửi lặp vẫn chỉ đánh dấu thanh toán và cấp **một key** cho đơn. Không để một giao dịch thanh toán nhiều đơn, không cấp lại thời hạn mới mỗi lần nhận webhook.
7. Cất key đã cấp ở backend có kiểm soát quyền đọc; chỉ người sở hữu đơn xem được. Nếu gửi Discord, xác thực tài khoản khách bằng OAuth2 hoặc một quy trình liên kết một lần; không tin một Discord ID do trình duyệt tự khai.
8. Kiểm thử giao dịch thật giá nhỏ trong tài khoản của bạn, bao gồm thiếu tiền, sai nội dung, giao dịch trùng, đơn hết hạn và webhook giả. Chỉ bật tự cấp khi các trường hợp này được xử lý.

SePay có tài liệu hỗ trợ BIDV tại https://developer.sepay.vn/vi/sepay-webhooks/tai-khoan-ngan-hang. payOS cũng có tài liệu kết nối BIDV tại https://payos.vn/docs/huong-dan-su-dung/kenh-thu/ket-noi-tai-khoan/ngan-hang-bidv/; có thể cần mã VA Billing và thủ tục theo loại tài khoản. Không khẳng định mọi tài khoản BIDV cá nhân đều dùng được mọi gói của hai dịch vụ.

### Điều cần chốt tiếp

- Tài khoản BIDV đã liên kết thành công với SePay hay payOS chưa.
- Nơi chạy backend + database + bot liên tục.
- Chính sách đổi máy, hoàn tiền và bắt đầu tính hạn.
- Nếu cần thu hồi key/kiểm tra online, nâng giao thức tool trước khi mở bán tự động.

Các secret đặt trong bảng điều khiển/biến môi trường của host, không gửi vào chat hoặc repo.
