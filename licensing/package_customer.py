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
readme="""SoraVynx — Dorm Claim Gifts preview

GIỮ QUÀ CHƯA NHẬN để thử bằng tool. Không bấm Claim Gifts trong game bằng tay.
Thoát game/tool cũ, giải nén vào thư mục mới, dùng đúng cặp EXE/DLL. Key và bộ khóa ký cũ giữ nguyên.
Vào Tyrant's Tower / Hall, để nút Claim Gifts trong game hiện rõ, chờ tool READY.
Auto Daily -> Claim Tyrant's Tower rewards: USE -> Claim Gifts / status.
Tool gọi một lần đúng handler UI Dorm nếu tìm được một controller đang hoạt động; nếu không, chỉ báo lỗi và không gửi hành động.
Tool tự ẩn; F1 hiện lại. Kiểm tra quà và nhiệm vụ daily có chuyển 0/1 sang 1/1 không.
Bấm lại nút tool sau một lần gọi chỉ xem trạng thái. Không tự gửi lại khi handler lỗi.
Báo cáo chỉ ở clipboard, không có thông tin tài khoản/key và không tạo log.
Bản mới bỏ luồng InfinityTower gọi nhầm. Chưa xác minh nhận quà Dorm trong game thật; không khẳng định thành công từ việc hàm trả về.
Không cần đổi private PEM, tạo lại key hoặc cấu hình lại bot/SePay.
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
