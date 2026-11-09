// Read an existing private key locally. Never create, replace or publish it.
import {createPrivateKey,createPublicKey} from 'node:crypto';
import {readFileSync,writeFileSync,existsSync} from 'node:fs';
import {dirname,resolve} from 'node:path';
import {fileURLToPath} from 'node:url';
const here=dirname(fileURLToPath(import.meta.url));
export function exportExistingPublic(privateFile,publicFile){
 const key=createPrivateKey(readFileSync(privateFile,'utf8'));
 const jwk=createPublicKey(key).export({format:'jwk'});
 if(jwk.crv!=='P-256')throw new Error('Khoa nay khong phai P-256. Giu nguyen file PEM.');
 const xy=Buffer.concat([Buffer.from(jwk.x,'base64url'),Buffer.from(jwk.y,'base64url')]);
 const header='/* PUBLIC KEY ONLY. Derived from the existing issuer, not a new key. */\n#ifndef SORAVYNX_LICENSE_PUBLIC_KEY_H\n#define SORAVYNX_LICENSE_PUBLIC_KEY_H\n#define SV_LICENSE_CONFIGURED 1\nstatic const unsigned char sv_license_public_xy[64]={'+[...xy].map(b=>'0x'+b.toString(16).padStart(2,'0')).join(',')+'};\n#endif\n';
 if(existsSync(publicFile)){
  if(readFileSync(publicFile,'utf8')!==header)throw new Error('PUBLIC-KEY-TO-SEND.h da ton tai va khac khoa. Doi ten file .h nay roi thu lai; KHONG sua file PEM.');
 }else writeFileSync(publicFile,header,{flag:'wx'});
 return publicFile;
}
if(process.argv[1]&&resolve(process.argv[1])===fileURLToPath(import.meta.url)){
 try{
  const file=exportExistingPublic(resolve(here,'secrets/issuer-private.pem'),resolve(here,'PUBLIC-KEY-TO-SEND.h'));
  console.log('\nDA TAO FILE CONG KHAI:\n'+file+'\n\nChi gui PUBLIC-KEY-TO-SEND.h vao chat de build lai tool.\nKHONG gui issuer-private.pem hoac thu muc secrets.\nKhoa bi mat cua ban khong bi sua, ghi de hoac tao lai.');
 }catch(e){console.error('\nCHUA XUAT DUOC: '+(e.code==='ENOENT'?'Dat ca 2 file Export-Public-Key vao thu muc licensing dang co secrets/issuer-private.pem.':e.message));process.exitCode=1;}
}
