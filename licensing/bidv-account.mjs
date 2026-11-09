export function bidvVA(value){
 const va=String(value||'').trim().toUpperCase();
 if(!/^96247[A-Z0-9]{1,14}$/.test(va))throw new Error('BIDV ca nhan can tai khoan ao VA bat dau 96247. Mo 8-Set-BIDV-VA.cmd; khong dung so tai khoan goc de tao QR.');
 return va;
}
