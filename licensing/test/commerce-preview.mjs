// Isolated browser QA fixture ONLY. Synthetic bank/VA, in-memory issuer and publishing. Never deploy.
import {generateKeyPairSync} from 'node:crypto';
import {PaymentStore} from '../payment-store.mjs';
import {paymentServer} from '../payment-server.mjs';
const pem=generateKeyPairSync('ec',{namedCurve:'prime256v1'}).privateKey.export({type:'pkcs8',format:'pem'});
const store=new PaymentStore({file:':memory:',pem,account:'12345678901234',virtualAccount:'96247TESTVA',publish:async()=>{}});
paymentServer({store,webhookKey:'s'.repeat(64),adminKey:'a'.repeat(64),publicURL:'http://127.0.0.1:8791'}).listen(8791,'127.0.0.1',()=>console.log('Isolated commerce fixture ready'));
