import {test} from 'node:test';
import assert from 'node:assert/strict';
import {generateKeyPairSync} from 'node:crypto';
import {mkdtempSync,rmSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {once} from 'node:events';
import {PaymentStore} from '../payment-store.mjs';
import {paymentServer} from '../payment-server.mjs';
import {verifyShortReceipt,publishShortLicense} from '../issuer.mjs';
const pair=generateKeyPairSync('ec',{namedCurve:'prime256v1'}),pem=pair.privateKey.export({type:'pkcs8',format:'pem'});
const now=1800000000,account='12345678901234';
const make=(options={})=>new PaymentStore({file:':memory:',pem,account,now:()=>now,publish:async()=>{},...options});
const date=seconds=>new Date((seconds+7*3600)*1000).toISOString().slice(0,19).replace('T',' ');
const body=(order,overrides={})=>({id:123,gateway:'BIDV',accountNumber:account,transferType:'in',transferAmount:order.amount,code:null,content:order.code,transactionDate:date(now),...overrides});
test('only an exact incoming payment receives one verifiable key; duplicate delivery does not reissue',async()=>{
 const issued=[];const store=make({publish:async r=>issued.push(r)});
 try{
  const order=store.create('week');assert.equal(order.amount,29000);assert.equal(store.view(order.id).key,undefined);
  assert.equal(store.webhook(body(order)),'accepted');assert.equal(store.view(order.id).state,'publishing');assert.equal(store.view(order.id).key,undefined);
  assert.equal(store.webhook(body(order)),'duplicate');await store.flush();await store.flush();
  const result=store.view(order.id);assert.equal(result.state,'ready');assert.match(result.key,/^[A-HJ-NP-Z2-9]{15}$/);assert.equal(issued.length,1);
  assert.equal(verifyShortReceipt(pair.publicKey,issued[0].receipt,result.key,now),true);
  assert.equal(store.webhook(body(order,{id:124})),'ignored');assert.equal(store.view(order.id).key,result.key);
 }finally{store.close();}
});
test('wrong bank/account/amount/direction, mixed orders, old/future payments and subaccounts cannot unlock',()=>{
 const store=make();try{
  const order=store.create('month'),other=store.create('year');
  for(const change of [{accountNumber:'999999999'},{gateway:'OtherBank'},{transferAmount:1},{transferAmount:order.amount+1},{transferType:'out'},{content:'unknown'},{content:order.code+' '+other.code},{code:other.code},{transactionDate:date(now-301)},{transactionDate:date(now+1801)},{subAccount:'other'}]){
   assert.equal(store.webhook(body(order,change)),'ignored');assert.equal(store.view(order.id).state,'pending');
  }
  assert.throws(()=>store.webhook(body(order,{id:'123'})));
  assert.throws(()=>store.webhook(body(order,{transferAmount:NaN})));
  assert.throws(()=>store.webhook(body(order,{transactionDate:'not a date'})));
 }finally{store.close();}
});
test('one transaction cannot fund two orders, even with a modified replay',()=>{
 const store=make();try{
  const a=store.create('week'),b=store.create('week');store.webhook(body(a));
  assert.throws(()=>store.webhook(body(b)),/transaction_conflict/);
  assert.equal(store.view(b.id).state,'pending');
 }finally{store.close();}
});
test('a delayed webhook accepts a payment made in time, but not a payment made after expiry',()=>{
 let clock=now;const store=make({now:()=>clock});try{
  const a=store.create('week'),b=store.create('week');clock+=3600;
  assert.equal(store.view(a.id).state,'expired');
  assert.equal(store.webhook(body(a,{transactionDate:date(now+60)})),'accepted');
  assert.equal(store.webhook(body(b,{id:124,transactionDate:date(now+1900)})),'ignored');
 }finally{store.close();}
});
test('publishing failure and process restart retain the same paid key, then resume once',async()=>{
 const dir=mkdtempSync(join(tmpdir(),'svx-pay-')),file=join(dir,'payments.sqlite');let attempts=0;
 let store=make({file,publish:async()=>{attempts++;throw new Error('network');}});
 try{
  const order=store.create('lifetime');store.webhook(body(order));await store.flush();assert.equal(attempts,1);
  const first=store.db.prepare('SELECT access_key FROM orders WHERE id=?').get(order.id).access_key;
  assert.equal(store.view(order.id).key,undefined);store.close();
  store=make({file,publish:async()=>{attempts++;}});assert.equal(store.webhook(body(order)),'duplicate');await store.flush();
  assert.equal(store.view(order.id).key,first);assert.equal(attempts,2);
 }finally{store.close();rmSync(dir,{recursive:true,force:true});}
});
test('HTTP endpoints reject unauthorized and oversized webhooks, keep order secrets private and price on server',async()=>{
 const store=make(),secret='s'.repeat(64),admin='a'.repeat(64);
 const server=paymentServer({store,webhookKey:secret,adminKey:admin,publicURL:'https://checkout.example.com'});server.listen(0,'127.0.0.1');await once(server,'listening');
 const base='http://127.0.0.1:'+server.address().port;
 const post=(path,value,auth)=>fetch(base+path,{method:'POST',headers:{'Content-Type':'application/json',...(auth?{Authorization:auth}:{})},body:JSON.stringify(value)});
 try{
  assert.equal((await post('/admin/orders',{plan:'week'})).status,401);
  const r=await post('/admin/orders',{plan:'week',amount:1},'Bearer '+admin);assert.equal(r.status,201);const order=await r.json();assert.equal(order.amount,29000);assert.match(order.url,/^https:\/\/checkout.example.com\/checkout#/);
  assert.equal((await post('/webhooks/sepay',body(order),'Apikey bad')).status,401);assert.equal(store.view(order.id).state,'pending');
  assert.equal((await post('/webhooks/sepay',{x:'x'.repeat(17000)},'Apikey '+secret)).status,413);
  const paid=await post('/webhooks/sepay',body(order),'Apikey '+secret);assert.equal(paid.status,201);assert.equal((await paid.json()).success,true);await store.flush();
  const result=await fetch(base+'/api/orders/'+order.id);assert.equal(result.headers.get('cache-control'),'no-store');assert.match((await result.json()).key,/^[A-HJ-NP-Z2-9]{15}$/);
  assert.equal((await fetch(base+'/api/orders/'+order.code)).status,404);
  const page=await fetch(base+'/checkout');assert.equal(page.headers.get('referrer-policy'),'no-referrer');
 }finally{server.closeAllConnections();await new Promise(r=>server.close(r));store.close();}
});
test('GitHub publish retry accepts only the identical already-committed signed record',async()=>{
 const record={hash:'f'.repeat(64),receipt:'signed record'};let requests=0;
 const fetcher=async()=>++requests%2?{status:422,ok:false}:{ok:true,json:async()=>({encoding:'base64',content:Buffer.from(record.receipt+'\n').toString('base64')})};
 assert.equal(await publishShortLicense(record,'test-token',{fetcher}),true);assert.equal(requests,2);
 await assert.rejects(publishShortLicense(record,'test-token',{fetcher:async()=>({status:422,ok:false})}));
});
