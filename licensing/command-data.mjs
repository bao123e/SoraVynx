const choices=[{name:'7 ngày',value:'week'},{name:'30 ngày',value:'month'},{name:'365 ngày',value:'year'},{name:'Vĩnh viễn',value:'lifetime'}];
const options=[{type:3,name:'plan',description:'Thời hạn key',required:true,choices}];
export const commandData=[
 {name:'buy',description:'Mua key SoraVynx: quét QR và tự nhận key riêng',default_member_permissions:null,options},
 {name:'keycreate',description:'Chủ tool: cấp key sau khi tự xác nhận thanh toán',default_member_permissions:'8',options},
 {name:'order',description:'Chủ tool: tạo đơn BIDV, tự cấp key khi SePay xác nhận',default_member_permissions:'8',options}
];
