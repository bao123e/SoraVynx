/* Ephemeral CI issuer. Neither this private key nor test keys are release material. */
import {generateKeyPairSync} from 'node:crypto';
import {mkdirSync,writeFileSync} from 'node:fs';
import {resolve} from 'node:path';
import {issueLicense,publicHeader} from '../issuer.mjs';
const dir=resolve(process.argv[2]);mkdirSync(dir,{recursive:true});
const keys=generateKeyPairSync('ec',{namedCurve:'prime256v1'});
const pem=keys.privateKey.export({format:'pem',type:'pkcs8'}),pub=keys.publicKey.export({format:'pem',type:'spki'});
writeFileSync(resolve(dir,'fixture-private.pem'),pem,{mode:0o600});writeFileSync(resolve(dir,'fixture_public.h'),publicHeader(pub));
const now=1800000000,machine='a'.repeat(64),key=issueLicense(pem,machine,'week',now),life=issueLicense(pem,machine,'lifetime',now);
const cases=[['valid',key,machine,now,1],['expired',key,machine,now+604800,0],['future',key,machine,now-301,0],['other-machine',key,'b'.repeat(64),now,0],['changed-expiry',key.replace(String(now+604800),'9999999999'),machine,now,0],['changed-signature',key.slice(0,-1)+(key.at(-1)==='a'?'b':'a'),machine,now,0],['lifetime',life,machine,now+31536000,1],['short',key.slice(1),machine,now,0],['trailing',key+'a',machine,now,0]];
writeFileSync(resolve(dir,'fixture_cases.h'),'static const struct TestCase {const char *name,*key,*machine;uint64_t now;int valid;} cases[]={\n'+cases.map(c=>'{'+c.map(v=>typeof v==='string'?JSON.stringify(v):v).join(',')+'}').join(',\n')+'\n};\n');
console.log('Ephemeral Windows test fixtures prepared.');
