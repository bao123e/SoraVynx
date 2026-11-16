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
readme="""SoraVynx — Claim All double click preview.3

Theo yêu cầu thử double click: Claim All mặc định Double, các target khác vẫn Single.
Thoát game/tool cũ, giải nén cặp EXE/DLL mới, dùng key hiện tại.
Mở Commission -> tool Auto Daily -> Target: Claim All, Dispatch 1 commission ở USE, Clicks: Double.
F1 ẩn tool -> rê lên CLAIM ALL -> F9 chỉ đánh dấu -> F1 hiện tool -> Claim All / status một lần.
Không rê chuột trong lúc tool thực hiện batch hai lần nhấp; đánh dấu lại F9 vì đây là phiên game mới.
Chờ focus 350 ms; giữ mỗi lần nhấn 80 ms; chờ giữa hai lần 120 ms. Chỉ trả cursor sau khi nhả lần cuối.
Nếu mất focus, bị che, đổi kích thước/vị trí cửa sổ hoặc chuột bị rê, cú thứ hai bị hủy. Disable all hủy input chờ/nhả chuột.
Mỗi target chỉ một batch thử trong phiên; bấm lại chỉ xem trạng thái, không lặp gửi.
Gửi ảnh sau kết quả. Chưa tự chuyển sang Auto team hay Start Dispatch. Chưa xác minh nhận quà bằng bản mới trong game thật.
Không tạo log, không đổi key/issuer/private PEM/bot/SePay.
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
