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
readme="""SoraVynx — Auto Daily Dispatch preview

Tower đã được chủ tool xác nhận nhận quà và daily Complete ở bản trước. Không cần làm lại Tower.
Thoát game/tool cũ, giải nén cặp EXE/DLL mới vào thư mục mới, dùng key hiện tại.
1. Mở Dispatch trong game, chọn commission và đội muốn dùng; CHƯA bấm nút bắt đầu.
2. Auto Daily -> bấm Target để chọn Dispatch. Để Dispatch 1 commission ở USE.
3. F1 ẩn tool. Rê chuột lên nút START DISPATCH trong game, nhấn F9 (chỉ đánh dấu, không click).
4. F1 hiện tool, thấy DISPATCH POINT MARKED -> bấm Dispatch / status một lần.
5. Kiểm tra commission bắt đầu chạy và daily có Complete không. Nếu hiện hộp xác nhận khác, giữ nguyên và gửi ảnh trước khi thao tác tiếp.
Tower/Dispatch có điểm đánh dấu và một lượt thử riêng; không dùng lại tọa độ Tower cho Dispatch.
Không đổi trang/kích thước cửa sổ sau khi đánh dấu. Disable all hủy input chờ và nhả chuột đang giữ.
Chưa tự mở các trang, chọn commission/đội hoặc chạy toàn chuỗi Auto Daily. Bản Dispatch mới chưa thử trong game thật.
Không tạo log, không đổi public/private issuer, key hoặc cấu hình bot/SePay.
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
