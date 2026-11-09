import {test} from 'node:test';
import assert from 'node:assert/strict';
import {generateKeyPairSync} from 'node:crypto';
import {mkdtempSync,writeFileSync,readFileSync,rmSync} from 'node:fs';
import {join} from 'node:path';
import {tmpdir} from 'node:os';
import {readOwnerEnv,saveOwnerEnv,matchingIssuer} from '../owner-config.mjs';
import {publicHeader} from '../issuer.mjs';
test('setup accepts only the existing matching private issuer, without replacing it',()=>{
 const dir=mkdtempSync(join(tmpdir(),'svx-setup-'));
 try{
  const key=generateKeyPairSync('ec',{namedCurve:'prime256v1'}),other=generateKeyPairSync('ec',{namedCurve:'prime256v1'});
  const pem=key.privateKey.export({type:'pkcs8',format:'pem'}),pub=key.publicKey.export({type:'spki',format:'pem'});
  const file=join(dir,'private.pem'),header=join(dir,'public.h');writeFileSync(file,pem);writeFileSync(header,publicHeader(pub));
  assert.equal(matchingIssuer(file,header),pem);assert.equal(readFileSync(file,'utf8'),pem);
  writeFileSync(header,publicHeader(other.publicKey.export({type:'spki',format:'pem'})));assert.throws(()=>matchingIssuer(file,header),/KHONG khop/);
  writeFileSync(file,pub);assert.throws(()=>matchingIssuer(file,header));
 }finally{rmSync(dir,{recursive:true,force:true});}
});
test('local configuration preserves unrelated settings and rejects newline injection before overwriting',()=>{
 const dir=mkdtempSync(join(tmpdir(),'svx-env-')),file=join(dir,'.env');
 try{
  const values={DISCORD_BOT_TOKEN:'local-test-token',PUBLIC_BASE_URL:'https://example.com',LICENSE_PRIVATE_KEY_FILE:'C:\\Sora Vynx\\issuer-private.pem'};
  saveOwnerEnv(values,file);assert.deepEqual(readOwnerEnv(file),{...values,LICENSE_PRIVATE_KEY_FILE:'C:/Sora Vynx/issuer-private.pem'});
  const before=readFileSync(file,'utf8');assert.throws(()=>saveOwnerEnv({...values,DISCORD_BOT_TOKEN:'x\nOTHER=value'},file));assert.equal(readFileSync(file,'utf8'),before);
 }finally{rmSync(dir,{recursive:true,force:true});}
});
