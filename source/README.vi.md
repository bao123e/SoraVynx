## Trekker Details / Training riêng — 1.3.3-trekker-preview.1

Nối đầy đủ luồng Trekker theo các ảnh chủ tool đã gửi: Home -> Trekker List (6 cột) -> Trekker Details -> nút Upgrade bên phải -> Trekker Training -> Upgrade 1 level -> xác nhận một lần -> Power Up -> kiểm tra Training thấy cấp mới và chi phí 0 -> Home. Chọn Upgrade Trekker once USE, các mục khác SKIP để thử riêng, bấm Run selected daily steps. Giữ key hiện tại, giải nén toàn bộ ZIP và khởi động lại game/tool cũ.

Details và Training có profile riêng: header/caption, nút nâng, bảng chỉ số, cấp tránh huy hiệu chân dung, giá nằm thấp/phải hơn Disc. Phần số giá loại biểu tượng tiền, hỗ trợ thẻ hơi nghiêng. Nếu chỉ đọc được cấp >=10 ở Training, bấm Back qua Details rồi về List trước khi chọn ứng viên khác; tối đa 12 ô trong hai hàng, không tự cuộn. Không dùng vị trí tab Upgrade của Disc cho Trekker.

Có Inspect-Trekker.cmd riêng (chỉ đọc, không nhấp/phím/log/ảnh lưu), in nhãn Trekker và cờ bố cục/cấp/giá. Chỉ cần dùng nếu lỗi cần đối chiếu game thật. Script/helper được pin nguyên byte như trước. Kiểm thử mô phỏng kiểm tra cả luồng, tránh đọc nhầm huy hiệu, giá có icon ở độ nghiêng khác nhau, hai bước Back, đúng USE/SKIP và xác nhận nâng tối đa một lần. Chưa xác minh tự động Trekker trong game thật; không xác nhận nhiệm vụ từ server. Các bước Disc/Home/Commission và issuer giữ nguyên.

## Đóng Power Up rồi về Home — 1.3.2-upgrade-preview.7

Ảnh của chủ tool xác nhận nâng Disc thành công lên cấp 2, sau đó dừng vì chưa xử lý bảng Power Up. Bản này nhận bảng kết quả bằng dải cyan toàn chiều ngang và bảng sáng ở giữa, chỉ trong phase VERIFY sau khi lượt hiện tại đã gửi xác nhận nâng. Nhấp vùng trống bên phải bảng; nếu animation còn hiện thì có tối đa một lần thử đóng lại sau 1,8 giây, vẫn nhận diện lại bảng trước mỗi nhấp.

Không xem overlay là bằng chứng hoàn tất. Sau khi đóng phải đọc lại editor, thấy cấp mới >= cấp đã chuẩn bị và chi phí về 0, rồi mới bấm ngôi nhà. Không chọn nguyên liệu hay nâng thêm. Nếu bảng không đóng trong giới hạn thì ERR_UPGRADE_OVERLAY_NOT_CLOSED. Luồng không có overlay vẫn kiểm tra editor như trước. Mã chung hỗ trợ bảng tương tự ở Trekker, nhưng chỉ Disc được chủ tool xác nhận tới bước nâng thực tế.

Khi phiên cũ đã dừng, bảng đang mở có thể đóng bằng một nhấp tay rồi về Home; không cần nâng thêm chỉ để nhận lại phần đã hoàn thành. Bản sửa áp dụng cho các lượt auto tiếp theo. Kiểm thử Windows mô phỏng kiểm tra đầy đủ overlay -> editor -> Home, giới hạn đóng bảng và không lặp chi tiêu; cần xác nhận giao diện thực tế.

## Tách biểu tượng tiền khỏi chi phí Disc — 1.3.2-upgrade-preview.6

Ảnh thực tế xác nhận đã vào editor và đọc được cấp 1, nhưng báo ERR_UPGRADE_COST_UNREADABLE khi chi phí hiển thị 0. Vùng số trước đây bắt đầu ở x=82% và bao gồm biểu tượng tiền cao hơn chữ số, làm bộ phân đoạn chọn nhầm thành phần lớn nhất. Bản này đọc phần số Disc từ x=84,2%, không đổi vùng Trekker; mẫu chữ số chi phí riêng có các góc nghiêng nhỏ để đọc thẻ giá. Bộ đọc cấp vẫn dùng mẫu thẳng như trước.

Kiểm thử mới có biểu tượng cao hơn chữ, giá 0/100/2,860 và thẻ trắng nghiêng -8/0/+8 độ, đồng thời yêu cầu ảnh không có chữ phải trả không đọc được, không mặc định là 0. Cổng editor cấp 1–9, giá trống 0, chuẩn bị 1 level rồi xác nhận tối đa một lần giữ nguyên. Đây là sửa vùng đọc dựa trên ảnh đã gửi, vẫn cần kiểm tra auto trong game.

## Dùng kết quả Inspect thực tế để sửa Details — 1.3.2-upgrade-preview.5

Inspect của chủ tool tại Details: Disc label 1, Header/House True, DetailsActive/UpgradeInactive True, hai cờ Melody/Harmony False, Commit False; editor thì nhận đúng DISC_EDIT và cấp 1. Vì vậy Details được nhận bằng caption Disc ở header cùng header/home icon và trạng thái hai tab đã được đo đúng; không còn bắt buộc cờ màu hai thanh kỹ năng khi caption đã xác nhận. Vẫn yêu cầu ngữ cảnh Disc đã chọn, trạng thái tab không mâu thuẫn và không có nút xác nhận editor. Khi không có caption, phương án hai thanh kỹ năng cũ vẫn có điều kiện đầy đủ.

Kiểm thử dùng đúng vector cờ người dùng cung cấp và ảnh tổng hợp không có thanh kỹ năng để kiểm tra mở Upgrade mà chưa chi tiêu. Cổng kiểm tra cấp editor 1–9, chi phí 0, chọn nguyên liệu và xác nhận đúng một lần giữ nguyên. Inspect cập nhật có thêm caption header và chi phí editor để đối chiếu nếu cần. Bản sửa cần xác nhận bằng auto trên game thật.

## Nhận diện bố cục Disc và kiểm tra chỉ đọc — 1.3.2-upgrade-preview.4

Chủ tool gửi ảnh ERR_UNKNOWN_SCREEN ở Claw the Claw 1/10; nhánh thử lại mở tab trong preview.3 chưa được chạy vì chưa nhận ra màn hình. Preview.4 thêm nhận diện quang học có ngữ cảnh: chỉ khi lượt hiện tại đã mở thẻ Disc, yêu cầu đồng thời header/home icon, trạng thái thanh tab và hai thanh kỹ năng của Details hoặc các nút vật liệu/xác nhận ở editor. Không tự áp dụng bố cục Disc cho Trekker hay cửa sổ chưa có thẻ được mở. Bố cục thiếu hoặc bị làm tối sẽ bị từ chối.

Nếu cấp ở Details không đọc được, được mở editor để kiểm tra vì chuyển màn không dùng nguyên liệu. Chỉ chọn vật liệu sau khi đọc rõ cấp editor 1–9 và chi phí hiện tại 0. Cấp >=10 quay lại chọn thẻ khác; cấp/chi phí không đọc được báo lỗi riêng. Xác nhận nâng vẫn tối đa một lần. Mẫu thử Windows mới bao gồm không có toàn bộ nhãn OCR, thiếu landmark, sai ngữ cảnh, làm tối giao diện, bộ đếm vàng và không được dùng nguyên liệu khi chưa xác minh editor.

ZIP có Inspect-Screen.cmd: chạy rồi chuyển focus sang game trong 8 giây, chỉ đọc các nhãn cố định và cờ bố cục. In chữ không chứa tên tài khoản, key hay đường dẫn; không gửi click/phím, không lưu/gửi ảnh hoặc log. Dùng kết quả này khi cần đối chiếu game thật để tránh tiếp tục đoán. Bản sửa cần thử lại trên game; không khẳng định đã sửa xong từ ảnh tổng hợp.

## Mở lại tab Upgrade có kiểm tra — 1.3.2-upgrade-preview.3

Chủ tool báo dừng ở Disc 1/10 với ERR_UPGRADE_SCREEN_NOT_CHANGED. Bản này nhận diện lại màn chi tiết sau lần mở Upgrade: nếu vẫn là cùng mục và qua ít nhất 1,8 giây, thử chuyển tab thêm một lần. Nếu vẫn không mở được thì báo ERR_UPGRADE_TAB_NOT_OPENED. Các lỗi mở danh sách/chi tiết/về Home có mã riêng, không dùng chung thông báo không đổi màn hình.

Đây chỉ là thử lại thao tác chuyển màn, không áp dụng cho chọn nguyên liệu hoặc xác nhận nâng. Chi tiêu Upgrade vẫn tối đa một lần. Dùng key cũ, giải nén toàn bộ ZIP, bắt đầu từ màn chính như trước. Khả năng click tab được game nhận vẫn cần thử lại trong game; chưa khẳng định nguyên nhân đầu vào từ ảnh và mã lỗi hiện có.

## Sửa chọn mục đầy cấp và nhận diện Disc — 1.3.2-upgrade-preview.2

Preview.1 đã chọn Daylight Garden 10/10 rồi dừng ERR_UNKNOWN_SCREEN, không đạt yêu cầu chọn cấp thấp. Preview.2 đọc cấp Disc Details ở góc trên phải, tách khỏi vùng cấp trong màn Upgrade; đọc lại header/tab/skill bằng các vùng phóng to nếu OCR toàn khung bỏ sót. Chỉ nâng cấp 1–9. Cấp từ 10 trở lên hoặc không đọc rõ thì quay lại danh sách, kiểm tra thẻ khác; không mở màn chi tiêu cho thẻ bị loại.

Tìm trong hai hàng đang thấy: tối đa 10 Disc hoặc 12 Trekker. Ưu tiên nhãn cấp thấp đọc được; mỗi ô thẻ chỉ kiểm tra một lần trong một lượt. Nếu không tìm thấy mục phù hợp thì dừng trước khi dùng nguyên liệu, không tự cuộn hay Ascension. Các bước Upgrade 1 level, xác nhận một lần, kiểm tra kết quả và về Home giữ nguyên. Dùng key cũ, giải nén đủ ZIP mới. Chưa xác nhận bản sửa trong game thật.

## Disc / Trekker upgrade — 1.3.2-upgrade-preview.1

Mở màn chính, Auto Daily chọn USE/SKIP cho Upgrade Disc once, Upgrade Trekker once, Interact with home Trekker và Dispatch 1 commission rồi **Run selected daily steps**. Giữ key cũ, giải nén đầy đủ ZIP; không cần F9. Thứ tự các mục USE: Home interaction -> Disc -> Trekker -> Commission. Mỗi mục nâng xong sẽ nhấp icon ngôi nhà và đợi màn chính trước khi chuyển bước.

Theo ảnh hướng dẫn của chủ tool, đọc nhãn cấp trên các thẻ đang nhìn thấy và chọn cấp thấp nhất trong khoảng 1–10. Nếu nhãn cấp nhỏ không đọc được, mở thẻ đầu tiên để xem cấp lớn ở màn chi tiết; chỉ tiếp tục khi đọc rõ cấp 1–10. Không tự cuộn, không nâng mục cấp cao hoặc không đọc được cấp. Mở màn nâng, bấm Upgrade 1 level để game chọn nguyên liệu, đợi mức xem trước tăng và chi phí dương rồi bấm Upgrade đúng một lần. Sau đó yêu cầu mức mới vẫn hiện và phần chọn nguyên liệu/chi phí đã về 0 trước khi về Home. Các ô cấp/chi phí bị OCR bỏ qua có bộ đối chiếu chữ số bằng mẫu font Windows tạo cục bộ trong bộ nhớ; kết quả không rõ sẽ dừng. Chi tiêu Upgrade không được thử lại trong cùng lượt. Đã chọn sẵn nguyên liệu, thiếu nguyên liệu, không đọc được UI hoặc không thấy thay đổi thì dừng với mã lỗi. Không dùng Upgrade to Max, Crescendo hoặc Ascension.

Hai thao tác này dùng nguyên liệu và tiền game theo nút Upgrade 1 level. Chỉ USE những mục bạn muốn chạy. Kiểm thử Windows dùng màn hình tổng hợp, không chạy trên tài khoản game; Disc/Trekker mới cần chủ tool xác nhận trong game. Kết thúc luồng không chứng minh nhiệm vụ đã Complete; chưa phải toàn bộ Auto Daily. Player/Event, issuer và cấu hình bot/thanh toán không đổi; không tạo log sản phẩm.

## Home interaction + Commission — 1.3.1-home-commission-preview.1

Chủ tool cho biết nhiệm vụ Interact with home Trekker chỉ cần nhấp nhân vật ở màn chính. Bản này thêm một lần nhấp tại giữa vùng client (50%, 50%) theo ảnh giao diện đã cung cấp, chỉ khi OCR nhận ra màn hình HOME. Đây là điểm theo bố cục tham chiếu, không phải nhận diện hình nhân vật. Không nhấp lại trong cùng lượt chạy; không tự kiểm tra nhiệm vụ qua server.

Ở Auto Daily, chọn USE/SKIP độc lập cho Interact with home Trekker và Dispatch 1 commission; nút mới là **Auto Home / Commission**. Nếu cả hai USE: bắt đầu màn chính -> nhấp nhân vật -> chờ -> Commission và luồng nhận thưởng/Dispatch Again như trước. Chỉ Home USE thì dừng sau thao tác và chờ; chỉ Dispatch USE thì không nhấp nhân vật. Nếu Home USE mà đang ở trang Commission, tool yêu cầu về màn chính. Không cần F9. F1 hoặc thay lựa chọn trong lúc chạy sẽ dừng helper.

Commission preview.3 được chủ tool đánh giá có vẻ ổn và yêu cầu chuyển bước; chưa có xác nhận riêng cho lần gặp overlay tùy chọn. Home interaction mới cần thử trong game. Giữ key và issuer hiện tại, giải nén đầy đủ ZIP.

## Xử lý overlay sau Dispatch Again — 1.3.0-commission-ocr-preview.3

Chủ tool xác nhận preview.2 đã tự chạy được, nhưng hội thoại Esc Skip và Items Obtained đôi khi xuất hiện sau Dispatch Again. Preview.3 tiếp tục đọc màn hình sau nút này, xử lý hội thoại/vật phẩm được nhận diện ở cả trước lẫn sau Dispatch Again. Chỉ kết thúc khi thấy lại trang Commission ổn định ít nhất 2,5 giây và đã quan sát tối thiểu 8 giây sau Dispatch Again. Không bấm Dispatch Again hoặc Claim All thêm trong giai đoạn này; modal không đổi hoặc màn hình không rõ sẽ dừng có lỗi. Không xác minh tiến độ nhiệm vụ qua server.

Giải nén toàn bộ ZIP mới, giữ key cũ. Chạy từ màn chính hoặc Commission như preview.2. F1 vẫn dừng. Hội thoại không xuất hiện thì tool chỉ chờ quan sát rồi kết thúc; không gửi Esc/click dư. Bản sửa thứ tự này chưa được chủ tool thử trong game.

## Sửa nhận diện màn chính — 1.3.0-commission-ocr-preview.2

Chủ tool báo preview.1 dừng `ERR_UNKNOWN_SCREEN` ngay màn chính, chưa mở Commission. Preview.2 đọc vị trí từng từ thay cho so khớp nguyên dòng (OCR có thể gộp Purchase và Commission), thêm đọc phóng to vùng biểu tượng góc phải với hai kiểu màu. Vẫn chỉ bấm khi nhận diện duy nhất chữ Commission trong vùng màn chính; không đoán điểm bấm khi thiếu chữ. Dùng key cũ và giải nén toàn bộ ZIP mới, không chỉ thay script. Bản sửa chưa được xác nhận trong game thật.

## Auto Commission OCR — 1.3.0-commission-ocr-preview.1

Chủ tool đã xác nhận Tower nhận quà/daily Complete và Dispatch Again dẫn tới nhiệm vụ Dispatch 1/1. Bản này ghép riêng Commission thành một lượt chạy dựa trên chữ/UI tiếng Anh, không cần F9 cho module này: màn chính -> Commission -> Claim All -> hội thoại nếu có (Esc Skip + Space Next) -> Items Obtained (nhấp dòng tiếp tục) -> Commission Complete / Dispatch Again.

Giải nén đủ thư mục automation cùng EXE/DLL, giữ key hiện tại. Vào màn chính hoặc Commission, chờ READY, Auto Daily để Dispatch 1 commission ở USE rồi **Auto Commission**. Giữ game foreground, không đổi kích thước/chuyển ứng dụng. **F1 dừng luồng và hiện tool**; Disable all cũng hủy helper và nhả input thuộc luồng. Chế độ F9 thủ công vẫn giữ nguyên. Không yêu cầu chọn lại commission/đội; dùng đúng nút Dispatch Again đã được kiểm chứng ở bước thủ công.

Dùng OCR tiếng Anh tích hợp Windows, xử lý ảnh trong bộ nhớ ở helper cục bộ; không lưu/gửi screenshot hoặc xuất chữ tài khoản. Chỉ các mã trạng thái/input định sẵn đi qua pipe. Native kiểm tra script trùng resource của DLL và giữ file read-only trong cả phiên; helper ở job riêng kill-on-close, chỉ kế thừa ba handle stdio cần thiết. Dừng nếu focus/window thay đổi, không hiểu màn hình, vượt giới hạn bước/thời gian hoặc thiếu OCR. Không gửi Esc khi không nhận diện cặp dấu hiệu hội thoại. Claim All chỉ tối đa hai lần có nhận diện lại, Dispatch Again một lần; không bấm mù theo tọa độ cũ.

Kiểm thử Windows dùng OCR thật trên bitmap tổng hợp cho các trạng thái và kiểm tra máy trạng thái/lỗi/protocol/script pinning. Chưa kiểm tra nhận diện luồng mới trong game thật. Dòng hoàn tất chỉ xác nhận gửi input, chưa truy vấn server/quest. Đây chưa phải toàn chuỗi Auto Daily. Player/Event, issuer và cấu hình bot/thanh toán không đổi; không tạo log sản phẩm.

## Claim All double-click — 1.2.3-dispatch-preview.4

Chủ tool báo con trỏ đến đúng điểm F9 nhưng Claim All không nhận click và yêu cầu thử double click. Thêm **Clicks: Single / Double** theo từng target; Claim All mặc định Double, các target khác Single. Một lần bấm nút tool tạo một batch, không tự lặp batch sau khi đã thử. Chưa kết luận nguyên nhân cú nhấp cũ không được game nhận.

Mở bản mới bằng key hiện tại, chọn Target: Claim All, Dispatch 1 commission ở USE và Clicks: Double. F1 → rê Claim All + F9 (không click) → F1 → Claim All / status một lần. Vì đổi DLL cần khởi động lại game, đánh dấu F9 lại trong phiên mới. Không rê chuột trong lúc batch chạy; gửi ảnh sau kết quả, chưa bấm tiếp Auto team/Start.

Chờ focus 350 ms, giữ DOWN 80 ms, nhả UP, chờ 120 ms, rồi DOWN/UP thứ hai tại cùng điểm. Chờ 100 ms sau UP cuối rồi mới restore cursor; single-click cũng có khoảng chờ cuối để không dời con trỏ trước khi game xử lý UP. Cú thứ hai được kiểm tra lại focus/kích thước/vị trí cửa sổ/điểm bị che/cursor/mouse state và bị hủy nếu thay đổi. Disable all hủy bước chờ và nhả nút đã nhấn. Input count không chứng minh nhận thưởng hay daily Complete. Windows regression kiểm tra bốn event đúng thứ tự, không restore cursor trong gap, hủy/cancel trong mỗi nửa batch, chặn batch thứ hai và giữ single-click behavior cũ.

## Commission steps — 1.2.3-dispatch-preview.2

Ảnh thực tế cho thấy Limit 4/4 và Max commission limit reached. Chủ tool cho biết sau Claim All có UI game tự xếp đội/làm tiếp, không cần chọn từng đội lại. Không kết luận quota theo ngày hay slot đang chiếm trước khi xem kết quả nhận thưởng.

Thêm target riêng: **Tower → Claim All → Auto team → Start Dispatch**. Mỗi bước có điểm F9 và lượt thử riêng. Không tự chuyển bước, không dùng tọa độ của bước khác hoặc đoán agentList/agentData. Những target xếp đội/bắt đầu chỉ sẵn sàng để thử sau khi có ảnh đúng màn hình tiếp theo, chưa phải chuỗi tự chạy.

Trước mắt chỉ thử Claim All bằng tool: mở Commission còn thưởng Complete, Auto Daily chọn Target: Claim All và để Dispatch 1 commission ở USE. F1 → rê lên Claim All + F9 (không bấm tay) → F1 → Claim All / status. Gửi ảnh sau khi tool bấm, gồm popup và bộ đếm Limit. Chưa yêu cầu chọn lại đội hoặc tự bấm tiếp. Tower đã Complete không cần thử lại.

Windows regression kiểm tra trạng thái/điểm riêng cả bốn target, không tự chuyển bước, cấm đổi target khi input chờ/đang giữ chuột, và Disable all nhả chuột của target cuối. Các bước Commission mới chưa xác minh trong game thật. Player/Event và issuer không đổi; không tạo log.

## Dispatch preview — 1.2.3-dispatch-preview.1

Tower nhận quà và daily Complete đã được chủ tool xác nhận ở preview.2. Tiếp theo thử nút START DISPATCH bằng cách đánh dấu F9, giữ commission/đội đang chọn. Thêm **Target: Tower / Dispatch** trong Auto Daily; mỗi target có điểm, trạng thái và giới hạn một lượt riêng. Không chuyển target giữa lúc input chờ/chuột đang giữ. Disable all hủy/nhả chuột của cả hai target.

Mở Dispatch, chọn commission/đội, chưa bấm bắt đầu. Trong tool chọn Target: Dispatch và để Dispatch 1 commission ở USE. F1 -> rê lên START DISPATCH + F9 (không bấm tay) -> F1 -> DISPATCH POINT MARKED -> Dispatch / status. Kiểm tra commission bắt đầu chạy và daily Complete. Nếu có hộp xác nhận mới, giữ nguyên và gửi ảnh; không tự nối thêm click chưa được kiểm chứng. Không cần làm lại Tower.

Đây vẫn là kiểm thử từng hành động; chưa tự điều hướng, chọn commission/đội, truy vấn tiến độ daily hoặc chạy toàn chuỗi. Report v5 cho thấy ReqApplyAgent(agentList, agentData, callback) là hàm không có self; không suy ra dữ liệu gửi từ tên hàm, nên dùng nút UI game với lựa chọn hiện tại. Player/Event backend và issuer giữ nguyên; Dispatch chưa được kiểm chứng trong game thật.

## Claim Gifts đánh dấu bằng F9 — 1.2.2-dorm-preview.2

Ảnh thực tế của preview.1 trả `DORM_DISCOVERY_LIMIT` trước khi gọi handler. Preview.2 bỏ hẳn luồng Lua đó cho hành động nhận quà; giữ script cũ ở trạng thái chỉ trả hướng dẫn, không quét hoặc gửi request. Bộ test Lua cũ cho discovery được thay bằng kiểm thử helper click native Windows.

**Giữ quà chưa nhận.** Mở cặp EXE/DLL mới với key hiện tại, vào Hall để Claim Gifts hiện rõ, chờ READY. Auto Daily -> Claim Tyrant's Tower rewards -> USE. F1 ẩn tool, rê chuột lên Claim Gifts và nhấn **F9 để đánh dấu, không bấm nhận tay**. F1 hiện lại tool; thấy POINT MARKED rồi bấm **Claim Gifts / status** một lần. Tool tự nhấn điểm đã chọn. Kiểm tra vật phẩm và daily lên 1/1; F1 hiện tool, lần bấm sau chỉ xem trạng thái.

Không đổi trang game hoặc kích thước cửa sổ giữa đánh dấu và thực hiện. F9 kiểm tra đúng process/cửa sổ Unity và điểm trong client; không gửi input. Trước click kiểm tra lại kích thước/focus/điểm không bị cửa sổ khác che/chuột không bị giữ. Chuột được nhả ở tick sau kể cả khi mất focus; Disable all hủy thao tác chờ và nhả chuột đã nhấn. Không gửi lần thứ hai tự động, kể cả khi Windows chặn input. Tọa độ chỉ ở bộ nhớ; không ghi file log. Player/Event backend và public issuer giữ nguyên.

Windows test dùng input API giả để kiểm tra chính helper thực thi, không nhấn chuột thật trên runner. Bản mới chưa được kiểm tra nhận quà trong game thật và chưa phải chuỗi Auto Daily đầy đủ. Các phần dưới là lịch sử thử nghiệm, không phải hướng dẫn dùng preview.2.

## Dorm Claim Gifts preview — 1.2.2-dorm-preview.1

Report Lua v5 mới có `Game.UI.Dorm.DormInteractive.DormInteractiveCtrl.OnBtnClick_ReceiveDormReward(self)` và `PlayerDormData.DormRewardReceive(self,rewardIds,callback)`. Nút Claim Gifts trong Hall thuộc Dorm; lệnh `InfinityTower.ITDailyRewardReq` trước đây không chứng minh được nhiệm vụ Tyrant's Tower. Bản mới bỏ hẳn lệnh InfinityTower và dùng handler UI Dorm để game tự chọn reward IDs/callback, không đoán đối số.

GIỮ QUÀ CHƯA NHẬN. Đóng game/tool cũ, mở cặp EXE/DLL mới với key hiện tại. Vào Hall để Claim Gifts hiện rõ, chờ READY, bật Claim Tyrant's Tower rewards -> USE rồi bấm **Claim Gifts / status** trên tool. Không nhận quà thủ công trước. F1 hiện lại tool sau khi ẩn; kiểm tra vật phẩm và daily từ 0/1 sang 1/1. Nếu không gọi được, thông báo có mã lỗi và bộ đếm discovery. Không cần làm lại report Lua rộng.

Script duyệt bảng registry/capture của PanelManager theo giới hạn, chỉ nhận đúng class có chữ ký đã biết và userdata Unity thuộc scene loaded/active. Class definition, instance không chứng minh được hoạt động, nhiều ứng viên hoặc discovery bị cắt đều không được gọi. Sau một lần gọi (kể cả lỗi), lần bấm sau chỉ xem trạng thái. Không có fallback InfinityTower/Shop. Không tạo log hay xuất giá trị tài khoản. Player/Event backend không đổi; action Dorm mới chưa xác minh trong game thật.

## Bản Windows hiện tại — xác nhận Player trên game 2.1.0

Bản `1.2.1-startup-preview.1` đã được chủ tool chạy: READY, Player 5/5, Event 3/3; **cả 5 chức năng Player hoạt động**. Event chưa được thử vì sự kiện đã kết thúc; chữ ký khớp không có nghĩa gameplay Event đã được xác minh. Auto Daily chưa được xác nhận.

Cặp EXE/DLL ở gốc repo được lấy nguyên từ ZIP đã qua Windows CI và đã được người dùng thử; không build lại hoặc thay hành vi sau xác nhận. `source/build.json` ghi metadata của lần build; `source/verification.json` ghi kết quả thử bổ sung theo người dùng và hash cặp file. ZIP ở Releases.

Key 15 ký tự và public key hiện tại được giữ nguyên. Không chạy lại Setup-Issuer, không đưa private PEM/token/.env lên repo. Nếu gặp lỗi khởi tạo, dùng Copy startup report; không tạo file log.

## Chẩn đoán bản game 2.1.0 — startup-preview.1

GameAssembly SHA256 người dùng cung cấp vẫn khớp profile cũ (`6adc5de8…32e06dd8`); global-metadata.dat cũng không đổi. hotfix.arch mới khớp MD5 trong file_versions.dat, chứa Hotfix theo định dạng CDPH. Chưa xác định được bước khởi tạo bị dừng trên máy thật; chưa có cơ sở thay offset hay bỏ kiểm tra phiên bản.

Bản mới hiển thị mã lỗi riêng ở thanh trạng thái và có **Copy startup report** ở chân cửa sổ, dùng được sau khi khởi tạo thất bại. Mở tool như trước, vào màn chính game, chờ kết thúc kết nối (WAIT_HOTFIX tối đa 180 giây), bấm nút và gửi nội dung. Báo cáo không chứa key/tài khoản/đường dẫn, không tạo tệp log. Giữ nguyên issuer/private PEM; đây là bản chẩn đoán, chưa xác nhận gameplay sau cập nhật.

Các mã chính: HOTFIX_TIMEOUT (chưa thấy image), PLAYER_CLASS_MISSING/MONSTER_CLASS_MISSING (thiếu lớp), FRAME_PROFILE_MISMATCH (entry trong RAM không khớp), MINHOOK_INIT_FAILED/FRAME_CREATE_FAILED/FRAME_ENABLE_FAILED (thư viện hook, kèm mã trạng thái). Không ghi đè bộ tool cũ đang mở; EXE và DLL mới phải cùng một cặp.

# SoraVynx — mã nguồn Windows x64

SoraVynx là tên mới của toolkit Stella Sora. Launcher **SoraVynx.exe**, thư viện **SoraVynx.Runtime.dll** luôn phải cùng một build. Đóng game trước khi chuyển từ bản cũ; không đổi tên hoặc trộn EXE/DLL giữa hai bản. config.ini vẫn giữ các tùy chọn nền đã lưu.

Menu không đánh số: **Player**, **Event**, **Custom BG**, **Auto Daily**. Player/Event giữ backend trước; các hàng đủ chiều ngang và ô Damage/Vacuum/Score nằm cùng hàng. Custom BG có Image/Panel/Button opacity riêng 0–100%, Fit/Fill và ảnh PNG/JPG/BMP. Font mới **Be Vietnam Pro** nhúng Regular/SemiBold/Bold, hỗ trợ tiếng Việt; logo SoraVynx trong app và icon EXE. Không cài font hệ thống, không tạo log.

## Auto Daily: thử Tower reward, bỏ qua Shop

Mười hai lựa chọn theo ảnh người dùng: Tier-up Trial; Ascension; nâng Disc; nâng Trekker; 5 battle stages; tiêu 100 Vigor; tương tác Trekker màn chính; nhận thưởng Tyrant’s Tower; tặng quà Heartlink; dispatch commission; nhận quà ngẫu nhiên Shop; đăng nhập ngày. Dự kiến dùng đội, màn, difficulty, Trekker và Disc đang chọn trong game.

Bản preview.10 dừng phần Shop theo yêu cầu: luôn xóa bit USE của Shop khi đọc cấu hình, lưu lại SKIP và khóa hàng Shop trong UI. Script Shop cũ giữ trong mã nguồn để lưu lịch sử, không còn được nhúng hay chạy trong DLL. Mười nhiệm vụ khác vẫn chưa triển khai; chỉ có lệnh thử Tower reward.

Đóng game, cập nhật cả EXE/DLL, vào game và mở Tyrant’s Tower để dữ liệu thưởng được nạp. Đợi READY, bật USE cho **Claim Tyrant’s Tower rewards**, chọn **Tower reward / status**. Lệnh chạy sau Update gốc trên main thread; tool tự ẩn và trả focus về game. F1 hiện lại tool, bấm lại nút để đọc trạng thái vào clipboard mà không gửi thêm yêu cầu.

Nguồn binding là report v5: PlayerData.InfinityTower có đúng class PlayerInfinityTowerData, ITDailyRewardReq(self) một tham số và không vararg, InfinityTowerRewardsTab là bảng. Khi module cache không có class chuẩn, chỉ chấp nhận source metadata của handler khớp đúng module. Thiếu/bất đồng class, arity hoặc dữ liệu thưởng thì dừng có Code. Không dùng CS/Unity object discovery hay registry PanelManager của phần Shop.

Mỗi phiên game chỉ thử gọi handler một lần, ghi state trước khi gọi để không tự retry sau exception. Các lần sau so sánh snapshot giới hạn của riêng bảng thưởng Tower (32 node, 128 entry/node, sâu hai tầng), chỉ xuất số field thay đổi, không xuất scalar/ID. Thay đổi có thể là cập nhật dữ liệu thông thường; không suy ra nhận thưởng thành công hay task hoàn tất. Chưa giải mã server response. Handler gốc và game chịu trách nhiệm xử lý/hiển thị thưởng, cần xác nhận trong game thật.

Disable all/X hoặc chuyển Tower sang SKIP sẽ hủy lệnh còn chờ; hành động đã chạy không được retry tự động. Nút Shop bị khóa SKIP và không thể gửi action Shop. Player/Event, Custom BG và font/logo giữ nguyên backend/giao diện trước. Không tạo log. Muốn thử lại sau lỗi hoặc ngày mới thì restart game để kết thúc giới hạn một lần thử.

**Copy Lua name report** vẫn đọc metadata, không gọi handler nhiệm vụ. Vào màn chính hoặc mở trang cần chẩn đoán, chờ READY rồi bấm nút và chuyển lại game. Không cần làm lại daily. Nếu không có callback Update sau 15 giây, yêu cầu chờ bị hủy.

**Lua report v5:** dùng cầu nối main-thread đã xác nhận hoạt động từ report v4. Một hook riêng gọi Update gốc trước, kiểm tra Unity main thread, đọc singleton LuaManager/LuaEnv hiện có và chạy script kiểm tra cố định nhúng trong resource. Không nhận script tùy ý, không require module mới, không gọi hàm nhiệm vụ tìm được.

Script ưu tiên 18 lớp dữ liệu liên quan daily cùng các controller UI đã nạp, bỏ duyệt đệ quy super/__index. Thu thập hàm khai báo trực tiếp, tên/số tham số qua debug API nếu runtime hỗ trợ và liên kết cấu trúc PlayerData/PanelManager tới lớp đã nạp. Khi debug API không hỗ trợ hoặc lỗi, ghi unknown; không suy ra hàm không có đối số. Không xuất scalar, địa chỉ, thân hàm, giá trị tài khoản hay thông báo lỗi debug/Lua chi tiết.

Giới hạn 30000 entry tổng, 2048 entry mỗi lần liệt kê bảng, 2400 dòng và 192 KiB nội dung; mỗi module tối đa 96 hàm/12 KiB, chia lượt giữa các module. Có mức bao phủ và NOT_LOADED cho lớp chưa thấy trong package.loaded. Nếu cần lấy thêm controller, mở trang tương ứng trong game rồi chụp lại; không phải nâng cấp, tặng quà, tiêu Vigor hay làm lại daily. Không cần gửi lại Copy API metadata v3. Script có cấp phát tạm trong Lua; kiểm thử mô phỏng không thay thế xác minh game thật.

**Copy API metadata:** giữ report v3 để chẩn đoán C#/IL2CPP: danh sách assembly, quét Game/Hotfix và assembly ứng dụng, ưu tiên Lua/UI cùng sáu nhóm nhiệm vụ. Giới hạn 512 KiB tổng, 192 KiB chỉ mục; nhóm Lua/UI 64 KiB với 16 KiB/lớp, nhóm nhiệm vụ 24 KiB với 4 KiB/lớp. Chỉ metadata, không đọc giá trị field hay gọi hành động game.

Hai báo cáo chẩn đoán chỉ sao chép vào clipboard, không tạo tệp log. Lệnh Tower riêng có thể gọi handler nhận thưởng game thật như mô tả trên; chưa có chuỗi tự động 12 nhiệm vụ.

## Build / kiểm thử

```sh
python source/build.py --zig /path/to/zig-0.14.1
python source/test.py  # GCC + liblua5.4 (or liblua5.3)
# Optional Windows/Wine mock report regression:
python source/test_report_windows.py --zig /path/to/zig-0.14.1
```

Build xuất EXE, DLL và build.json vào source/dist. SHA-256 DLL nhúng trong EXE; source/build.json ghi hash của cặp phát hành. Bitmap nền không nhúng sẵn; resource chỉ có logo/icon và font. Các tệp EXE/DLL phát hành nằm ngay gốc repo; ZIP nằm ở GitHub Releases.

22 nhóm kiểm thử native/Lua gồm backend mô phỏng, tiện ích launcher, hình học Fit/Fill và bộ định dạng báo cáo có giới hạn và script inventory chạy trên Lua VM thật. Bộ test Windows/Wine mới kiểm tra callback Update gốc, idle/shared/off-main guards, hàng đợi một yêu cầu, reentrancy, GC handle, exception, clipboard và timeout. Đã kiểm tra 13 nhóm UI qua Wine, gồm font/logo, trang Auto Daily và sao chép metadata qua clipboard với API giả; không thay thế kiểm tra game thật. Tower có handler request thử nghiệm; Shop bị SKIP và 10 nhiệm vụ khác chưa thực hiện.

Giữ F1/Hide để ẩn mà không tắt tính năng; X/Alt+F4 tắt tính năng rồi ẩn. F6/F7/F8 điều khiển event. Chỉnh giao diện và danh sách daily không đổi trạng thái Player/Event.

Thiết kế theo UI/UX Pro Max; mã AGPL-3.0-or-later, MinHook BSD, Be Vietnam Pro SIL OFL 1.1. Logo được tạo riêng cho SoraVynx; bản PNG gốc và ICO nằm trong assets. Giấy phép tổng hợp trong LICENSE.txt.

## Sửa report v1 trong preview.2

Report thực tế đầu tiên có 90 lớp, trong đó 85 lớp là schema Protobuf; bộ lọc cũ hết dung lượng trước khi đến các controller. Report v2 bỏ phần này, xuất chỉ mục trước các chữ ký, ưu tiên handler kể cả nằm sau phương thức thứ 32 và bổ sung thông tin lớp cha. Test Windows mock đặt 100 schema trước các lớp thật, kiểm tra đủ sáu nhóm, field/singleton kế thừa và clipboard, đồng thời xác nhận không có lệnh game được gọi. Trên Linux dùng --wine hoặc --build-only --out để biên dịch riêng.

## Mở rộng report v3 trong preview.3

Report v2 thực tế đã liệt kê đủ 637 ứng viên Hotfix nhưng không có luồng daily/UI cần dùng: dispatch chỉ là animation dispatcher, shop chủ yếu enum. Phân tích GameAssembly tìm thấy tham chiếu Game:XLua.LuaEnv/GCAction, nên report v3 mở rộng sang assembly Game và metadata cầu nối Lua/UI. Đây là bước tìm API, chưa xác nhận module Lua thực hiện nhiệm vụ. Test Windows/Wine mô phỏng ba assembly, kiểm tra thu được LuaEnv/LuaTable/LuaManager trong Game, bỏ qua framework và không gọi runtime_invoke hay hành động game.

## Lua report v4 trong preview.4

Report v3 thực tế liệt kê đủ 1005 ứng viên, trong đó có Game.LuaManager, LuaUIController và XLua.LuaEnv/LuaTable. Đây là cầu nối C#, chưa phải tên module nghiệp vụ Lua. Bản preview.4 bổ sung inventory tên module/hàm đã nạp thay vì tiếp tục tăng dung lượng metadata. Test trên Lua VM thật kiểm tra thứ tự ổn định, chu trình, độ sâu, dung lượng và không gọi các hàm game/metamethod trong fixture hoặc xuất scalar bí mật. Không cam kết mọi hàm daily đã xuất hiện hoặc Auto Daily sẽ hoạt động chỉ từ tên hàm.

## Kết quả report v4 và sửa trong preview.5

Report v4 thực tế đã đọc Lua thành công: 4090 entry, 300 bảng, đủ 2400 dòng và Limited=yes. Có ReceiveDailyReward, ReceiveDailyActiveReward, SendDailyShopRewardReceiveReq, SendDiscStrengthenReq, ReqApplyAgent và ITDailyRewardReq, nhưng tên hàm không xác nhận được đối số, điều kiện gọi, tài nguyên hoặc callback hoàn tất. Nhiều dòng thuộc super của Login/CheckIn controller; các module PlayerCharData/PlayerDatingData chỉ còn tên, thiếu chi tiết.

Report v5 sửa phần này, bổ sung metadata tham số và liên kết instance. Ở preview.5 Auto Daily vẫn setup-only; danh sách hàm chưa đủ để chạy một chuỗi nhiệm vụ thật. [Bản đồ thông tin đã tìm được](AUTO_DAILY.vi.md) ghi rõ những phần còn thiếu.

## Hành động Shop đầu tiên trong preview.6

Report v5 thực tế xác nhận PlayerData.Shop, GetDailyShopReward(self) và SendDailyShopRewardReceiveReq(self, callback); đồng thời cho thấy DispatchData.ReqApplyAgent không có self, nên không thể dùng một kiểu gọi chung cho mọi task. Shop được nối riêng với kiểm tra chữ ký/instance và trạng thái một lần thử. Test trên Lua VM thật xác nhận handler mua hàng không bị gọi, callback không làm lộ payload, không có lần gửi thứ hai khi bấm lại hoặc sau exception. Build mới chưa được kiểm tra trong game thật.

## Sửa luồng UI trong preview.7

Ảnh preview.6 của người dùng cho thấy callback request đã về và cờ Shop đã đổi, nhưng tool không mở nút Daily Gift. Preview.7 thay direct request bằng đúng OnBtnClick_DailyGift của ShopCtrl đang hoạt động, dựa trên chữ ký report v5 và public getters LuaBehaviourBase từ report v3. Script không gọi tên hàm trên một class hoặc instance tùy ý. Tool ẩn trước khi trả focus về game; kết quả handler-invoked được copy mà không bật popup che UI. Chỉ kết quả lỗi hoặc lần hỏi trạng thái sau đó mới hiện hộp thông tin. Chưa xác minh thao tác này trong game thật.

## Sửa tra handler trong preview.8

Ảnh .7 cho thấy game đã ở Shop nhưng tool trả thông báo gộp về module/handler/chữ ký trước khi duyệt live UI. Chưa thể phân biệt nguyên nhân cụ thể chỉ từ thông báo cũ. Bản .8 chuyển lookup sang instance đang hiển thị, hỗ trợ class cache không có khóa chuẩn và các kiểu dispatch được dùng trong Lua, đồng thời tách Code chẩn đoán. Test dùng class không có trong package.loaded, handler kế thừa/__index function, controller pure Lua trong panel và missing local names; không gọi RPC dự phòng. Hành vi game thật vẫn cần kiểm tra.

## Sửa trường hợp zero behaviours trong preview.9

Ảnh thực tế trả SHOP_CONTROLLER_NOT_FOUND với normal search bằng 0. Chưa thể kết luận chỉ từ ảnh rằng object có DontSave hay toàn bộ UI là pure Lua, nên .9 xét cả loaded runtime objects và registry UI đã được xác minh current Shop. Báo cáo có normal/all/UIControllers/registry/currentShop theo từng đường và tên hằng enum Shop nếu cần chẩn đoán; không sao chép giá trị ID. Test dựng đúng trường hợp normal finder rỗng nhưng Resources hoặc private PanelManager cache chứa Shop đang dùng. Getter current panel là hàm nghiệp vụ duy nhất được gọi để kiểm tra context; các hàm OpenPanelWithContext/CheckPanelOpen/helper chỉ bị đọc metadata/upvalue, không được thực thi. Còn cần xác minh trong game thật.

## Chuyển mục tiêu preview.10

Người dùng yêu cầu bỏ qua Shop để thử nhiệm vụ khác. Bản này thay resource command 405 bằng daily_tower.lua, không tiếp tục đường dò UI Shop. Trạng thái auto_daily_ready vẫn false; daily_supported_actions chỉ ghi tower_daily_reward_request. Bộ test active dùng mock Tower model trên Lua VM thật, kiểm tra đúng self/arity/class, một lần thử, status refresh, không gọi UI/Shop/battle và không xuất giá trị reward state. Mã/test Shop cũ không chạy trong test mặc định và không nằm trong runtime resource.
