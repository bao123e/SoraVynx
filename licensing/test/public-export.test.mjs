import {test} from 'node:test';
import assert from 'node:assert/strict';
import {generateKeyPairSync,createPublicKey} from 'node:crypto';
import {mkdtempSync,writeFileSync,readFileSync,rmSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {exportExistingPublic} from '../Export-Public-Key.mjs';
test('recovery derives only the public point and leaves existing private bytes unchanged',()=>{
 const dir=mkdtempSync(join(tmpdir(),'svx-export-'));try{
  const key=generateKeyPairSync('ec',{namedCurve:'prime256v1'}).privateKey;
  const pem=key.export({format:'pem',type:'pkcs8'}),privateFile=join(dir,'private.pem'),publicFile=join(dir,'public.h');
  writeFileSync(privateFile,pem);exportExistingPublic(privateFile,publicFile);
  const h=readFileSync(publicFile,'utf8'),jwk=createPublicKey(key).export({format:'jwk'});
  const xy=Buffer.from([...h.matchAll(/0x([a-f0-9]{2})/g)].map(m=>parseInt(m[1],16)));
  assert.deepEqual(xy,Buffer.concat([Buffer.from(jwk.x,'base64url'),Buffer.from(jwk.y,'base64url')]));
  assert.ok(!h.includes('PRIVATE KEY'));assert.equal(readFileSync(privateFile,'utf8'),pem);
  exportExistingPublic(privateFile,publicFile);assert.equal(readFileSync(publicFile,'utf8'),h);
  writeFileSync(publicFile,'existing public data');assert.throws(()=>exportExistingPublic(privateFile,publicFile),/da ton tai/);
  assert.equal(readFileSync(publicFile,'utf8'),'existing public data');assert.equal(readFileSync(privateFile,'utf8'),pem);
 }finally{rmSync(dir,{recursive:true,force:true});}
});
