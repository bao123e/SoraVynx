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
readme='''SoraVynx Windows — bản kiểm thử có khóa truy cập\n\n1. Giải nén cả thư mục, giữ SoraVynx.exe và SoraVynx.Runtime.dll cạnh nhau.\n2. Mở SoraVynx.exe. Nhấn Copy code để lấy mã máy.\n3. Chủ tool dùng licensing/2-Create-Key.cmd trong bộ Owner Key Tools đã thiết lập để cấp key đúng mã máy.\n4. Dán toàn bộ key SV1.… vào ô rồi Activate. Tool sẽ chuyển sang bước chọn StellaSora.exe.\n5. Lần sau kích hoạt được đọc lại từ LocalAppData/SoraVynx/license.dat, bảo vệ bằng Windows DPAPI.\n\nKhông chạy lại Setup-Issuer để tạo bộ khóa khác: bộ EXE/DLL này chỉ nhận key từ private key đi cùng public key bạn đã gửi.\n\nĐã kiểm thử native Windows cho giao thức key và cửa sổ khóa; chưa kiểm thử gameplay của bản mới. Đây là bản key offline, không có thu hồi trực tuyến hay thanh toán QR tự động. Thời hạn bắt đầu khi cấp key. Không tạo log sản phẩm; license.dat là dữ liệu kích hoạt.\n'''
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
