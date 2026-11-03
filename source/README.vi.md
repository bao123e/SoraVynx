# SSTool — Bouncing Bumper Bash TEST

**Bản thử, chưa xác nhận hoạt động trong game hoặc điểm được máy chủ chấp nhận.** Bản chính trên nhánh runtime2-test giữ nguyên. Chưa chia menu Player/Event theo yêu cầu: kiểm chứng tính năng trước, chia menu sau.

[Tải gói thử](https://github.com/bao123e/sstool/raw/refs/heads/event-bumper-test/SSTool-event-test.zip)

Thoát game, giải nén vào thư mục riêng, dùng cả EXE và Mastercheat.dll trong gói thử. Mở SSTool.exe và chọn StellaSora.exe. Không dùng EXE bản chính kèm DLL thử vì kiểm tra hash khác nhau.

Vào Bouncing Bumper Bash rồi thử lần lượt:

- **F6**: bật/tắt No Cooldown của event.
- **F7**: bật/tắt Unlimited Energy của event.
- **F8**: bật/tắt nhân điểm event **x2**, hệ số cố định cho bước thử đầu.
- **F1**: ẩn/hiện bảng như cũ. Disable all hoặc X tắt cả tùy chọn Player lẫn event.

Mọi tùy chọn event bắt đầu OFF, độc lập với các nút Player. Dòng Event TEST ở cuối bảng cho biết ON/OFF hoặc N/A khi không tìm đủ tài nguyên. Applied ghi số lần sửa cooldown, năng lượng và điểm tương ứng; không tự chứng minh kết quả server. Phím event là phím tắt toàn cục trong khi game đang mở. Bản thử vẫn không ghi log.

Cách kiểm tra: bật F6 và F7 riêng rồi cùng nhau, dùng R và quan sát thanh năng lượng/cooldown. Thử F8 riêng trong một màn, so sánh điểm mỗi lần tăng và kết quả cuối trận. Nhân điểm có thể làm màn kết thúc sớm do đạt mục tiêu. Gửi ảnh phần Event TEST và mô tả R/điểm cuối trận. Nếu hiện N/A, đừng thay hash để ép chạy.

## Triển khai

Hai tệp đã gửi khớp nhau và trùng bản Hotfix trước. Đã giải mã các bảng managed và thân CIL để kiểm tra:

- BrickSkillInfo.get_IsFull yêu cầu curSkillEnergy >= maxSkillEnergy và curTime <= FP zero.
- BrickSkillInfo.Clear xóa năng lượng, đưa curTime về skillCd.
- BrickActorSkillCom.ResetEnergy gọi Clear rồi SyncLua.
- Bộ thử sửa field bằng il2cpp_field_set_value tại get_IsFull, SyncLua và lượt cập nhật. Lượt cập nhật sao chép Values của dictionary vào array runtime rồi đồng bộ UI, tối đa 10 lượt/giây. FP zero lấy từ op_Implicit, không giả định biểu diễn bit.
- BrickBreakerManager.UpdateScrole chuyển tiếp sang BrickBreakerController.UpdateScrole. Chỉ sửa delta Int32 dương tại controller, tránh nhân hai lần. Có giới hạn chống tràn Int32; không sửa tổng điểm hay gói mạng.

Dùng cùng hook EnterFrame/hash GameAssembly với bản chính. Trường và phương thức event chỉ bind khi khớp tên, kiểu và chữ ký. Player giữ luồng xử lý trước.

Đã build Windows x64. 15 nhóm backend mock với ASan/UBSan và 2 bài kiểm tra tiện ích đạt. Chưa chạy bản thử trên Windows/game tại môi trường build. Chưa thể khẳng định điểm sửa được lưu, thanh năng lượng và R đồng bộ đúng trong game.

[Mã nguồn tương ứng](source) · AGPL-3.0-or-later và MinHook BSD, bản giấy phép trong ZIP. Build: python source/build.py --zig /path/to/zig-0.14.1. Tests: python source/test.py.
