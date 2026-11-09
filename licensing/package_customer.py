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
readme='''SoraVynx — key 15 ký tự

Giải nén cả thư mục và mở SoraVynx.exe. Nhập key đã được đăng ký rồi bấm Vào tool.
Cửa sổ chỉ có ô nhập key và nút vào tool; Enter xác nhận, Esc đóng.
Máy cần có Internet khi mở tool để tra bản ghi ký số tại soravynx.online.
Chủ tool dùng Owner Key Tools v0.2, giữ nguyên private PEM cũ, chọn thời hạn để sinh key ngẫu nhiên 15 ký tự. Bản ghi .lic phải được đưa vào thư mục licenses/ của repo website trước khi dùng.
Key mới là mã truy cập có thể chia sẻ, không còn ràng buộc mã máy. Hạn tính từ lúc cấp. Không tự thu tiền hoặc xác nhận QR.
Đã kiểm tra giao thức và UI Windows; chưa xác minh gameplay của bản mới. Không tạo log sản phẩm.
''' 
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
