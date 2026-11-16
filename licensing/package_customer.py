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
readme="""SoraVynx — Commission UI steps preview.2

Game hiện chặn bắt đầu Dispatch vì Limit 4/4. Luồng cần thử: Claim All -> xem UI tự xếp đội/làm tiếp -> mới quyết định bước tiếp theo.
Thoát game/tool cũ, giải nén cặp EXE/DLL mới, giữ nguyên key hiện tại.
1. Mở Commission với phần thưởng Complete còn chưa nhận. Không bấm nhận tay.
2. Auto Daily -> bấm Target đến Claim All; để Dispatch 1 commission ở USE.
3. F1 ẩn tool -> rê lên CLAIM ALL trong game -> F9 chỉ đánh dấu -> F1 hiện tool.
4. Bấm Claim All / status một lần. Gửi ảnh màn hình sau đó, gồm popup thưởng/xếp đội và Limit.
Chưa tự chọn đội, bấm Auto team hoặc Start Dispatch trước khi xác định đúng UI kế tiếp.
Các target Tower / Claim All / Auto team / Start Dispatch có tọa độ và một lượt thử riêng. Không tự chuyển bước hoặc dùng tọa độ của nhau.
Không khẳng định Claim All giải phóng giới hạn 4/4; cần xem trạng thái thực tế sau nhận thưởng.
Không đổi trang/kích thước cửa sổ sau đánh dấu. Disable all hủy input chờ, nhả chuột nếu đã nhấn.
Không tạo log, không đổi issuer/private PEM/key/bot. Chưa xác minh các bước Commission mới trong game thật.
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
