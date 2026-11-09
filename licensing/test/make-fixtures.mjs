/* Ephemeral CI issuer. Neither this private key nor test keys are release material. */
import {generateKeyPairSync} from 'node:crypto';
import {mkdirSync,writeFileSync} from 'node:fs';
import {resolve} from 'node:path';
import {createShortLicense,publicHeader} from '../issuer.mjs';
const dir=resolve(process.argv[2]);mkdirSync(dir,{recursive:true});
const keys=generateKeyPairSync('ec',{namedCurve:'prime256v1'});
const pem=keys.privateKey.export({format:'pem',type:'pkcs8'}),pub=keys.publicKey.export({format:'pem',type:'spki'});
writeFileSync(resolve(dir,'fixture-private.pem'),pem,{mode:0o600});writeFileSync(resolve(dir,'fixture_public.h'),publicHeader(pub));
const now=1800000000,record=createShortLicense(pem,'week',now),life=createShortLicense(pem,'lifetime',now);
const key=record.receipt,machine=record.hash;
const cases=[['valid',key,machine,now,1],['expired',key,machine,now+604800,0],['future',key,machine,now-301,0],['other-code',key,'b'.repeat(64),now,0],['changed-expiry',key.replace(String(now+604800),'9999999999'),machine,now,0],['changed-signature',key.slice(0,-1)+(key.at(-1)==='a'?'b':'a'),machine,now,0],['lifetime',life.receipt,life.hash,now+31536000,1],['short',key.slice(1),machine,now,0],['trailing',key+'a',machine,now,0]];
writeFileSync(resolve(dir,'fixture_cases.h'),'static const struct TestCase {const char *name,*key,*machine;uint64_t now;int valid;} cases[]={\n'+cases.map(c=>'{'+c.map(v=>typeof v==='string'?JSON.stringify(v):v).join(',')+'}').join(',\n')+'\n};\n');
const ui=createShortLicense(pem,'week');writeFileSync(resolve(dir,'ui-key.txt'),ui.key);writeFileSync(resolve(dir,'ui-record.txt'),ui.receipt);
console.log('Ephemeral short-code fixtures prepared.');
