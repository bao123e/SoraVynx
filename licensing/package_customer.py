"""Package the verified customer binary pair; never include signing material."""
from pathlib import Path
from zipfile import ZipFile,ZIP_DEFLATED
import os,json,hashlib
root=Path(__file__).resolve().parent.parent
build=root/'source/dist';metadata=json.loads((build/'build.json').read_text())
if not metadata.get('license_issuer_configured'):raise SystemExit('Cannot package a customer build without an owner public key')
for name,field in [('SoraVynx.exe','launcher_sha256'),('SoraVynx.Runtime.dll','sha256')]:
 if hashlib.sha256((build/name).read_bytes()).hexdigest()!=metadata[field]:raise SystemExit('Binary pair hash mismatch')
if metadata['sha256'].encode() not in (build/'SoraVynx.exe').read_bytes():raise SystemExit('Launcher does not pin this DLL')
if os.environ.get('GITHUB_ACTIONS')=='true':
 metadata['license_gate_windows_tested']=True
 metadata['verification_run']='https://github.com/'+os.environ['GITHUB_REPOSITORY']+'/actions/runs/'+os.environ['GITHUB_RUN_ID']
metadata['gameplay_verified']=False
out=Path(os.environ.get('RUNNER_TEMP',build));archive=out/('SoraVynx-'+metadata['version']+'-win64.zip')
readme="""SoraVynx — selected daily steps preview

Giải nén TOÀN BỘ ZIP, giữ thư mục automation/commission.ps1 cạnh EXE/DLL. Không sửa file script; tool kiểm tra nó khớp binary.
Thoát game/tool cũ, mở cặp mới bằng key hiện tại. Menu ẩn khi khởi động; dùng phím menu đã chọn (mặc định F1) mở menu cố định giữa cửa sổ game. Vào màn chính và chờ READY.
Custom Keybinds: mở menu -> Keybinds -> bấm ô của chức năng -> nhấn phím hoặc tổ hợp Ctrl/Alt/Shift -> Save. Cancel bỏ thay đổi; Reset defaults chỉ đặt lại các ô, cần Save mới áp dụng. Backspace xóa một phím (không được xóa phím mở menu). Lưu vào [Hotkeys] Bindings trong config.ini cạnh tool, dùng lại khi mở tool lần sau. Phím trùng trong tool hoặc ứng dụng khác đang dùng sẽ báo lỗi và giữ nguyên bộ cũ. Giữ config.ini khi chuyển sang thư mục bản mới nếu muốn giữ lựa chọn.
Mặc định lần đầu: F1 menu/dừng Auto, F6/F7/F8 Event, F9 đánh dấu điểm; Ctrl+Alt+1..5 Player; Ctrl+Alt+9 Daily; Ctrl+Alt+0 tắt tất cả. Đây là giá trị mặc định, mỗi chức năng đều đổi được trong Keybinds.
Auto chờ hai lần nhận diện khớp nhau trước khi bấm, chờ chuyển màn tối đa 30 giây và giới hạn toàn lượt 6 phút. Không tự bấm lại xác nhận Upgrade khi lag. Nếu game vẫn không đáp ứng thì dừng và báo lỗi.
Trước khi Inspect: đóng OK của hộp lỗi, ẩn menu bằng phím đã chọn, đưa game lên trước; hộp lỗi có thể che số cấp dù vẫn đọc được giá. Không tiếp tục lượt nâng cũ có nguyên liệu đang chọn: về Home rồi chạy lượt mới.
Auto Daily -> chọn USE/SKIP cho Upgrade Disc once, Upgrade Trekker once, Interact with home Trekker, Dispatch 1 commission -> Run selected daily steps. Không cần F9.
Bắt đầu màn chính. Các mục USE chạy theo thứ tự Home interaction -> Disc -> Trekker -> Commission.
Disc/Trekker: ưu tiên cấp thấp nhất đọc được trên các thẻ đang thấy; nếu nhãn nhỏ thiếu thì kiểm tra cấp lớn ở màn chi tiết từng thẻ, chỉ chọn cấp 1–89. Bỏ qua cấp >=90/không đọc rõ, xem tối đa 10 Disc hoặc 12 Trekker trên hai hàng đang thấy, dùng Upgrade 1 level, xác nhận Upgrade một lần rồi về Home. Có dùng nguyên liệu/tiền game. Không dùng Max hoặc thử lại chi tiêu khi không rõ kết quả.
Nếu Home USE: bắt đầu ở màn chính, tool nhấp nhân vật một lần tại giữa màn hình theo bố cục tham chiếu (không nhận diện hình nhân vật). Nếu Dispatch cũng USE thì chạy Commission tiếp; chỉ Home USE thì dừng sau bước này.
Tool đọc UI tiếng Anh: mở Commission, Claim All, Dispatch Again một lần; Esc Skip và Items Obtained được xử lý nếu xuất hiện ở cả trước lẫn sau Dispatch Again.
Sau Dispatch Again, quan sát ít nhất 8 giây và chờ trang Commission ổn định 2,5 giây rồi kết thúc. Không có overlay thì không gửi Esc/click dư.
Nếu vẫn ở cùng màn chi tiết sau khi mở Upgrade, thử lại chuyển màn đúng một lần; ERR_UPGRADE_TAB_NOT_OPENED nghĩa là cả hai lần không mở được. Không thử lại nút xác nhận chi tiêu.
Trekker dùng Trekker Details -> nút Upgrade bên phải -> Trekker Training -> Upgrade 1 level -> xác nhận một lần -> đóng Power Up -> kiểm tra kết quả -> Home. Chỉ USE Upgrade Trekker once để thử riêng.
Nếu ERR_UPGRADE_LEVEL_UNREADABLE: bộ đọc mới thử thêm vùng số cấp vàng/xanh riêng, loại /giới hạn cấp, XP và biểu tượng hình thoi bên cạnh. Đây là thay đổi cần thử lại trong game, không tự bỏ qua kiểm tra cấp.
Nếu cần kiểm tra Trekker: dùng Inspect-Trekker.cmd (chỉ đọc như Inspect-Screen.cmd).
Nếu cần kiểm tra nhận diện Disc: chạy Inspect-Screen.cmd rồi đưa game lên trước trong 8 giây. Chỉ đọc nhãn/bố cục, không nhấp hay dùng nguyên liệu, không lưu ảnh/log. Có thể gửi phần kết quả chữ.
Sau khi xác nhận nâng: đóng Power Up nếu xuất hiện, kiểm tra editor thấy cấp mới và chi phí 0, rồi về Home. Không gửi thêm xác nhận nâng.
Giữ game có focus, không đổi kích thước/chuyển ứng dụng trong lúc chạy. Phím menu đã chọn dừng và hiện lại tool; Disable all cũng dừng.
Không có thưởng, không nhận ra màn hình hoặc thiếu English OCR thì dừng, không bấm đoán. Nếu báo thiếu OCR, cần thành phần OCR tiếng Anh của Windows.
Ảnh/chữ OCR chỉ xử lý cục bộ trong bộ nhớ, không lưu/gửi ảnh, không tạo log. Không đổi key/issuer/private PEM hoặc bot/SePay.
Chủ tool xác nhận preview.2 mở Commission và Dispatch Again hoạt động. Chủ tool đánh giá preview.3 có vẻ ổn. Chủ tool đã nhận được Power Up của Disc và Trekker. Các thay đổi chờ lag/menu/phím tắt của bản này cần kiểm tra trong game; kiểm thử Windows OCR dùng ảnh giả lập có chữ. Kết thúc input không đồng nghĩa server/daily đã xác nhận.
Home/Disc/Trekker/Commission là bốn bước được hỗ trợ, chưa phải tự chạy toàn bộ daily. Chế độ F9 thủ công vẫn có nếu cần.
"""
checks=''.join(hashlib.sha256((build/name).read_bytes()).hexdigest()+'  '+name+'\n' for name in ['SoraVynx.exe','SoraVynx.Runtime.dll'])
with ZipFile(archive,'w',ZIP_DEFLATED,compresslevel=9) as z:
 for name in ['SoraVynx.exe','SoraVynx.Runtime.dll']:z.write(build/name,name)
 z.write(root/'source/vision/commission.ps1','automation/commission.ps1')
 z.write(root/'source/vision/Inspect-Screen.cmd','Inspect-Screen.cmd')
 z.write(root/'source/vision/Inspect-Trekker.cmd','Inspect-Trekker.cmd')
 z.writestr('README.vi.txt',readme);z.writestr('build.json',json.dumps(metadata,indent=2)+'\n');z.writestr('SHA256SUMS.txt',checks)
 z.write(root/'LICENSE.txt','LICENSE.txt');z.write(root/'config.example.ini','config.example.ini')
with ZipFile(archive) as z:
 assert z.testzip() is None
 assert not any(n.endswith(('.pem','.h')) or '/secrets/' in n for n in z.namelist())
Path(str(archive)+'.sha256').write_text(hashlib.sha256(archive.read_bytes()).hexdigest()+'  '+archive.name+'\n')
print(archive)
