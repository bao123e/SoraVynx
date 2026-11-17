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
readme="""SoraVynx — Auto Home / Commission preview

Giải nén TOÀN BỘ ZIP, giữ thư mục automation/commission.ps1 cạnh EXE/DLL. Không sửa file script; tool kiểm tra nó khớp binary.
Thoát game/tool cũ, mở cặp mới bằng key hiện tại, vào màn chính hoặc Commission và chờ READY.
Auto Daily -> chọn USE/SKIP cho Interact with home Trekker và Dispatch 1 commission -> Auto Home / Commission. Không cần F9.
Nếu Home USE: bắt đầu ở màn chính, tool nhấp nhân vật một lần tại giữa màn hình theo bố cục tham chiếu (không nhận diện hình nhân vật). Nếu Dispatch cũng USE thì chạy Commission tiếp; chỉ Home USE thì dừng sau bước này.
Tool đọc UI tiếng Anh: mở Commission, Claim All, Dispatch Again một lần; Esc Skip và Items Obtained được xử lý nếu xuất hiện ở cả trước lẫn sau Dispatch Again.
Sau Dispatch Again, quan sát ít nhất 8 giây và chờ trang Commission ổn định 2,5 giây rồi kết thúc. Không có overlay thì không gửi Esc/click dư.
Giữ game có focus, không đổi kích thước/chuyển ứng dụng trong lúc chạy. F1 dừng và hiện lại tool; Disable all cũng dừng.
Không có thưởng, không nhận ra màn hình hoặc thiếu English OCR thì dừng, không bấm đoán. Nếu báo thiếu OCR, cần thành phần OCR tiếng Anh của Windows.
Ảnh/chữ OCR chỉ xử lý cục bộ trong bộ nhớ, không lưu/gửi ảnh, không tạo log. Không đổi key/issuer/private PEM hoặc bot/SePay.
Chủ tool xác nhận preview.2 mở Commission và Dispatch Again hoạt động. Chủ tool đánh giá preview.3 có vẻ ổn. Home interaction mới cần thử trong game; kiểm thử Windows OCR dùng ảnh giả lập có chữ. Kết thúc input không đồng nghĩa server/daily đã xác nhận.
Đây mới là hai bước Home interaction và Commission, chưa phải tự chạy toàn bộ daily. Chế độ F9 thủ công vẫn có nếu cần.
"""
checks=''.join(hashlib.sha256((build/name).read_bytes()).hexdigest()+'  '+name+'\n' for name in ['SoraVynx.exe','SoraVynx.Runtime.dll'])
with ZipFile(archive,'w',ZIP_DEFLATED,compresslevel=9) as z:
 for name in ['SoraVynx.exe','SoraVynx.Runtime.dll']:z.write(build/name,name)
 z.write(root/'source/vision/commission.ps1','automation/commission.ps1')
 z.writestr('README.vi.txt',readme);z.writestr('build.json',json.dumps(metadata,indent=2)+'\n');z.writestr('SHA256SUMS.txt',checks)
 z.write(root/'LICENSE.txt','LICENSE.txt');z.write(root/'config.example.ini','config.example.ini')
with ZipFile(archive) as z:
 assert z.testzip() is None
 assert not any(n.endswith(('.pem','.h')) or '/secrets/' in n for n in z.namelist())
Path(str(archive)+'.sha256').write_text(hashlib.sha256(archive.read_bytes()).hexdigest()+'  '+archive.name+'\n')
print(archive)
