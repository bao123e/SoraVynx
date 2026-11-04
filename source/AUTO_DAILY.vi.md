# Auto Daily — tình trạng tích hợp

Report Lua v5 đã xác nhận chữ ký và liên kết PlayerData cho nhiều lớp. Preview.8 nối riêng handler UI Daily Gift trên Shop; chưa có scheduler hay 11 nhiệm vụ còn lại. Report metadata v3 không chứa chữ ký của các hàm Lua này.

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
| Quà miễn phí Shop | PlayerData.Shop; SendDailyShopRewardReceiveReq(self, callback); GetDailyShopReward(self) | Preview.8 gọi ShopCtrl.OnBtnClick_DailyGift(self) trên instance UI đang hoạt động; cần xác minh UI/quà trong game |
| Daily login | PlayerDailyData.CheckDailyCheckIn/ProcessDailyCheckIn | Phân biệt login quest với bảng check-in; không đăng nhập lại tài khoản |

PlayerQuestData có ReceiveDailyReward và ReceiveDailyActiveReward, nhưng chưa xác nhận đối số hay trạng thái có thể nhận. Không gọi hàm chỉ dựa vào tên; không gọi LevelEnd hoặc sửa trạng thái task để đánh dấu hoàn tất.

Report v5 chỉ kiểm tra tên hàm, metadata tham số nếu có và cấu trúc đối tượng. Không gọi các hàm trong bảng, không xuất scalar, không tạo log. Dữ liệu report người dùng không được commit vào repo. Các module chưa nạp có thể cần mở trang UI tương ứng rồi chụp lại; không phải làm lại nhiệm vụ.

## Shop preview.6

Shop gift / status là lệnh riêng có thể gửi một yêu cầu game thật khi mục Shop đang USE. Cầu nối vẫn chạy sau Update gốc trên main thread. Kiểm tra chữ ký và class identity trước khi gửi, lưu trạng thái riêng trong Lua VM để không tự gửi lại, và giữ payload callback ngoài báo cáo. Tool chỉ xác nhận delivery của callback và cờ quà có đổi hay không; chưa giải mã server success. Callback không đồng nghĩa nhiệm vụ đã xong.

Report v5 có 149 module mục tiêu và vẫn chạm dung lượng 192 KiB; không cần thêm một bản report tổng quát để thử riêng Shop. Các task còn lại cần xác minh tiến độ/lựa chọn/callback và loại gọi: ví dụ ReqApplyAgent(agentList, agentData, callback) là hàm không có self, còn CharUpgrade(self, nCharId, tbMat, mapTargetLevel, callback) cần đúng nguyên liệu hiện tại. Không suy ra các tham số này chỉ từ tên hàm.

## UI preview.7 thay direct request

Người dùng xác nhận preview.6 không bấm/mở Daily Gift. Phiên bản mới dùng active LuaBehaviourBase.luaClassName/luaInstance để xác định đúng ShopCtrl và gọi handler nút một lần. Không gọi lại SendDailyShopRewardReceiveReq riêng hay dùng tọa độ click. Tool tự ẩn để không che UI, F1 hiện lại, các lần bấm sau chỉ xem trạng thái. Chưa có xác minh game thật về việc handler mở đúng panel và nhận quà.

## Tra handler preview.8

Thông báo .7 trong Shop bị gộp giữa thiếu module, thiếu raw method và thiếu chữ ký, nên không đủ kết luận nguyên nhân. Phiên bản .8 dùng active instance làm nguồn chính, class/super/metatable dispatch và alias module làm bằng chứng phụ. ShopCtrl pure Lua dưới ShopPanel cũng được xét theo namespace/function identity. Chỉ biết số tham số bằng một và không vararg mới gọi; tên local bị strip không tự làm binding thất bại. Mỗi lần dừng có Code cụ thể, không yêu cầu người dùng gửi lại toàn bộ metadata chỉ để biết bước nào lỗi.
