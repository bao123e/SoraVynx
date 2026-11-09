import {createInterface} from 'node:readline/promises';
import {readOwnerEnv,saveOwnerEnv} from './owner-config.mjs';
import {bidvVA} from './bidv-account.mjs';
const io=createInterface({input:process.stdin,output:process.stdout});
try{
 const values=readOwnerEnv();if(!values.SEPAY_ACCOUNT_NUMBER)throw new Error('Can .env da setup. Khong tao lai issuer.');
 console.log('BIDV ca nhan chi gui thong bao SePay khi chuyen vao VA.\nMo SePay > Tai khoan ngan hang > BIDV > Tai khoan ao, copy dung VA dang hoat dong cua ban.');
 const input=(await io.question('VA bat dau 96247'+(values.SEPAY_VIRTUAL_ACCOUNT?' ['+values.SEPAY_VIRTUAL_ACCOUNT+']':'')+': ')).trim();
 values.SEPAY_VIRTUAL_ACCOUNT=bidvVA(input||values.SEPAY_VIRTUAL_ACCOUNT);saveOwnerEnv(values);
 console.log('Da luu VA. Khoa ky, token, URL va database duoc giu nguyen.\nDung cua so webhook cu bang Ctrl+C, mo lai 5-Start-Webhook.cmd dung mot lan.\nQR moi se dung VA. Khoan da chuyen vao tai khoan goc khong tu xuat hien tren SePay; khong chuyen them de tra lai don cu.');
}catch(e){console.error(e.message);process.exitCode=1;}finally{io.close();}
