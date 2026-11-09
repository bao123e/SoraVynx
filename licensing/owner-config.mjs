import {createPrivateKey,createPublicKey,timingSafeEqual,randomBytes} from 'node:crypto';
import {readFileSync,writeFileSync,renameSync,existsSync} from 'node:fs';
import {resolve,dirname} from 'node:path';
import {parseEnv} from 'node:util';
import {fileURLToPath} from 'node:url';
export const directory=dirname(fileURLToPath(import.meta.url));
export const envPath=resolve(directory,'.env');
export function readOwnerEnv(file=envPath){return existsSync(file)?parseEnv(readFileSync(file,'utf8')):{};}
export function saveOwnerEnv(values,file=envPath){
 const lines=Object.entries(values).map(([name,value])=>{
  if(!/^[A-Z_]+$/.test(name)||/[\r\n"\x00]/.test(String(value)))throw new Error('Gia tri cau hinh khong hop le.');
  return name+'="'+String(value).replaceAll('\\','/')+'"';
 });
 const temporary=file+'.'+randomBytes(8).toString('hex')+'.tmp';
 writeFileSync(temporary,lines.join('\n')+'\n',{mode:0o600,flag:'wx'});renameSync(temporary,file);
}
export function matchingIssuer(privatePath,headerPath=resolve(directory,'../source/license_public_key.h')){
 const pem=readFileSync(resolve(directory,privatePath),'utf8');
 const jwk=createPublicKey(createPrivateKey(pem)).export({format:'jwk'});
 if(jwk.crv!=='P-256')throw new Error('Can dung khoa ky P-256 cu, khong tao lai khoa.');
 const actual=Buffer.concat([Buffer.from(jwk.x,'base64url'),Buffer.from(jwk.y,'base64url')]);
 const header=readFileSync(headerPath,'utf8');
 const bytes=[...header.matchAll(/0x([a-f\d]{2})/gi)].map(m=>parseInt(m[1],16));
 if(bytes.length!==64||!timingSafeEqual(actual,Buffer.from(bytes)))throw new Error('Khoa ky nay KHONG khop tool hien tai. Chon issuer-private.pem cu da dung de tao key vao duoc tool; KHONG chay Setup-Issuer lai.');
 return pem;
}
export async function serviceJSON(url,token,{method='GET',body,scheme='Bot'}={}){
 let response;
 try{response=await fetch(url,{method,redirect:'error',signal:AbortSignal.timeout(15000),headers:{Authorization:scheme+' '+token,Accept:'application/json','Content-Type':'application/json','User-Agent':'SoraVynx-Owner-Setup'},...(body?{body:JSON.stringify(body)}:{})});}
 catch{throw new Error('Khong ket noi duoc '+new URL(url).hostname+'. Kiem tra mang roi thu lai.');}
 if(!response.ok)throw new Error(new URL(url).hostname+' tra HTTP '+response.status+'. Kiem tra token, quyen truy cap va ID.');
 return response.status===204?null:response.json();
}
