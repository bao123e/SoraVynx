# SoraVynx Android · Player/Event preview

**Bản thử nghiệm ARM64 cho Stella Sora 2.0.0 (26093014), Android 8 trở lên.** Build APK và kiểm tra cấu trúc không chứng minh chức năng đã hoạt động trong game. Chưa thử đăng nhập, tải dữ liệu hay chiến đấu trên điện thoại thật. Không có Auto Daily.

## Cài và dùng

1. Trước khi thay game, liên kết tài khoản và kiểm tra bạn có thể đăng nhập lại. Bộ APK dùng chữ ký mới, không cài đè được game gốc. Nếu phải gỡ game gốc, có thể mất dữ liệu cục bộ và phải tải lại khoảng 12 GB. Không có cam kết giữ dữ liệu.
2. Cài `SoraVynx-Menu-0.1.0-arm64-preview.apk` trước, sau đó `StellaSora-SoraVynx-2.0.0-arm64-preview.apk` trong **cùng bộ build**. APK game đã gồm thư viện ARM64, không cần cài thêm split.
3. Mở game và hoàn tất tải dữ liệu. Sau lần tải đầu tiên, đóng hẳn game rồi mở lại, vào màn chính; sau đó mở SoraVynx từ danh sách ứng dụng gần đây và chờ trạng thái backend. Bản preview này chờ runtime/lớp game tối đa 10 phút mỗi phiên; nếu quá thời gian chờ, khởi động lại game sẽ khởi tạo lại backend, không cần gỡ/cài APK hay tải lại dữ liệu.
4. Trong Player hoặc Event, chỉ các chức năng backend nối được mới cho bật. Trở lại game bằng **Về game**. Kiểm tra riêng từng chức năng; dòng CONNECTED chỉ xác nhận đã cài hook, không xác nhận hiệu quả trong trận.
5. **Tắt tất cả** tắt Player và Event. Đóng hẳn game cũng xóa các cờ bật; không lưu chúng cho lần mở sau. Mục Custom BG có chọn ảnh, giữ đủ ảnh và opacity riêng cho ảnh/khung/nút.

Nếu game không mở/không đăng nhập được sau khi đổi chữ ký, bản tích hợp chưa tương thích. Không có bước bỏ qua kiểm tra toàn vẹn hoặc đăng nhập. Có thể cài lại game gốc; việc gỡ bản tích hợp cũng có thể xóa dữ liệu tải về.

## Giới hạn của bản port

- Menu React Native dùng `@react-native-menu/menu` 2.0.0; menu chạy ở ứng dụng riêng, không phải cửa sổ nổi trên game. Chuyển qua Recent Apps để đổi chức năng.
- Backend được nạp **trong APK game đã tích hợp**, không yêu cầu root hoặc Accessibility. App riêng không điều khiển được bản game gốc.
- Kiểm tra SHA-256 đúng thư viện và metadata, chữ ký hàm và vùng code trước khi hook. Các assembly đang nạp được quét để loại entry dùng chung với chữ ký không tương thích; struct truyền theo giá trị không xác minh được cũng bị bỏ qua. Không dùng RVA hay layout interpreter của bản Windows.
- Game Android có bảo vệ export/metadata và tải lớp động. Nếu API không được công khai khi chạy, chữ ký lớp thay đổi hoặc chỉ có interpreter entry không tương thích, một phần hoặc toàn bộ chức năng sẽ chưa khả dụng.
- Chưa xác minh chức năng, độ ổn định, chống can thiệp, nhận điểm/thưởng trên máy thật. Đây là bản port để kiểm tra tương thích, không phải bản Android hoàn thiện.
- SoraVynx không tạo log file/telemetry; menu release không có quyền Internet. SDK và hành vi của game gốc được giữ lại, có thể có ghi log riêng của game.
- Khóa ký mới được tạo riêng cho mỗi lần build; hai APK trong cùng bộ có chung chữ ký. Khóa không lưu trong repo/artifact. Build khác có thể cần gỡ cả hai ứng dụng trước khi cài, dẫn tới mất dữ liệu game. Bản phát hành ổn định cần khóa ký riêng được giữ lâu dài.
