# Auto Daily — tình trạng tích hợp

Report Lua v4 đã chạy được trên game. Các tên dưới đây được quan sát trong report, chưa xác nhận cách gọi hay backend thực thi. Report metadata v3 không chứa chữ ký của các hàm Lua này.

| Nhiệm vụ | Thông tin đã thấy | Cần xác minh trước khi chạy |
| --- | --- | --- |
| Tier-up Trial | PlayerDailyInstanceData, EnterDailyInstance, SendDailyInstanceRaidReq | Đúng chế độ, màn/đội hiện tại, Vigor và kết quả trận |
| Ascension | PlayerStarTowerData, PlayerTrialData | Xác định đúng chế độ và giữ lựa chọn hiện tại |
| Nâng Disc | PlayerDiscData.SendDiscStrengthenReq | Instance, Disc hiện tại, nguyên liệu và giới hạn một lần |
| Nâng Trekker | Module PlayerCharData đã nạp; chi tiết bị cắt | Hàm nâng cấp, Trekker hiện tại, nguyên liệu và giới hạn một lần |
| Thắng 5 battle stages | Các module dữ liệu battle đã nạp | Đọc tiến độ và chỉ chạy số trận còn thiếu |
| Tiêu 100 Vigor | Chưa xác nhận cách đọc tiến độ daily | Tính phần đã tiêu, tận dụng các trận đã chạy |
| Tương tác Trekker màn chính | MainViewCtrl đã thấy | Handler tương tác và xác nhận task tăng tiến độ |
| Thưởng Tyrant’s Tower | PlayerInfinityTowerData.ITDailyRewardReq | Điều kiện nhận, đối số và callback kết quả |
| Quà Heartlink | PlayerDatingData/PlayerDormData đã nạp; thiếu chi tiết | Handler tặng, mục tiêu/vật phẩm đang chọn, giới hạn một quà |
| Dispatch commission | DispatchData.ReqApplyAgent | Commission/đội hiện tại, slot trống, callback kết quả |
| Quà miễn phí Shop | PlayerShopData.SendDailyShopRewardReceiveReq | Instance, điều kiện chưa nhận, đối số và callback |
| Daily login | PlayerDailyData.CheckDailyCheckIn/ProcessDailyCheckIn | Phân biệt login quest với bảng check-in; không đăng nhập lại tài khoản |

PlayerQuestData có ReceiveDailyReward và ReceiveDailyActiveReward, nhưng chưa xác nhận đối số hay trạng thái có thể nhận. Không gọi hàm chỉ dựa vào tên; không gọi LevelEnd hoặc sửa trạng thái task để đánh dấu hoàn tất.

Report v5 chỉ kiểm tra tên hàm, metadata tham số nếu có và cấu trúc đối tượng. Không gọi các hàm trong bảng, không xuất scalar, không tạo log. Dữ liệu report người dùng không được commit vào repo. Các module chưa nạp có thể cần mở trang UI tương ứng rồi chụp lại; không phải làm lại nhiệm vụ.
