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
readme="""SoraVynx — Claim Gifts đánh dấu bằng F9 (preview.2)

Giữ quà chưa nhận. Không bấm Claim Gifts bằng tay.
Thoát game/tool cũ. Giải nén cặp EXE/DLL mới vào thư mục mới; dùng key hiện tại.
1. Vào Hall, để Claim Gifts hiện rõ; chờ tool READY.
2. Auto Daily -> Claim Tyrant's Tower rewards: USE.
3. F1 ẩn tool. Để game có focus, rê chuột lên chính nút Claim Gifts và nhấn F9 (CHỈ ĐÁNH DẤU, KHÔNG CLICK).
4. F1 hiện tool. Thấy POINT MARKED rồi bấm Claim Gifts / status một lần. Tool tự nhấn điểm đã đánh dấu.
5. Kiểm tra bảng nhận vật phẩm và daily có lên 1/1 không. F1 hiện lại tool; bấm lại chỉ xem trạng thái.
Không đổi trang game hoặc kích thước cửa sổ sau khi đánh dấu. Nếu cần, đánh dấu lại trước khi gửi thao tác đầu tiên.
Mỗi phiên game chỉ một lần gửi thử; Disable all hủy thao tác chờ và nhả chuột nếu đã gửi nhấn.
Điểm chỉ lưu trong bộ nhớ, không ghi file. Không quét Lua hay gọi InfinityTower/Shop cho thao tác này.
Nếu F9 bị ứng dụng khác giữ, đóng ứng dụng đó rồi khởi động lại tool.
Không tạo log, không đổi key/private PEM hay bot/SePay. Chưa xác minh click này trong game thật.
"""
checks=''.join(hashlib.sha256((build/name).read_bytes()).hexdigest()+'  '+name+'\n' for name in ['SoraVynx.exe','SoraVynx.Runtime.dll'])
with ZipFile(archive,'w',ZIP_DEFLATED,compresslevel=9) as z:
 for name in ['SoraVynx.exe','SoraVynx.Runtime.dll']:z.write(build/name,name)
 z.writestr('README.vi.txt',readme);z.writestr('build.json',json.dumps(metadata,indent=2)+'\n');z.writestr('SHA256SUMS.txt',checks)
 z.write(root/'LICENSE.txt','LICENSE.txt');z.write(root/'config.example.ini','config.example.ini')
with ZipFile(archive) as z:
 assert z.testzip() is None
 assert not any(n.endswith(('.pem','.h')) or '/secrets/' in n for n in z.namelist())
Path(str(archive)+'.sha256').write_text(hashlib.sha256(archive.read_bytes()).hexdigest()+'  '+archive.name+'\n')
print(archive)
