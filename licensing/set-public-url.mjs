import {createInterface} from 'node:readline/promises';
import {readOwnerEnv,saveOwnerEnv} from './owner-config.mjs';
const io=createInterface({input:process.stdin,output:process.stdout});
try{
 const values=readOwnerEnv();if(!values.SEPAY_WEBHOOK_API_KEY)throw new Error('Mo 3-Setup-Bot.cmd truoc.');
 const url=(await io.question('Dan link https://...trycloudflare.com (khong co / cuoi): ')).trim();
 if(!/^https:\/\/[a-z\d.-]+(?::\d+)?$/i.test(url))throw new Error('Can link HTTPS, khong co duong dan hoac dau / cuoi.');
 values.PUBLIC_BASE_URL=url;saveOwnerEnv(values);
 console.log('\nTrong SePay > Webhooks > Them webhook:\nURL: '+url+'/webhooks/sepay\nPhuong thuc: POST\nSu kien: tien vao\nXac thuc: API Key\nAPI Key: '+values.SEPAY_WEBHOOK_API_KEY);
 console.log('\nKhoa tren chi dan vao SePay, KHONG gui vao chat.\nDung va mo lai 5-Start-Webhook.cmd de ap dung URL moi.\nGiu cua so Tunnel va Bot mo. Sau do tao /order trong Discord.');
}catch(e){console.error(e.message);process.exitCode=1;}finally{io.close();}
