import {test} from 'node:test';
import assert from 'node:assert/strict';
import {generateKeyPairSync} from 'node:crypto';
import {mkdtempSync,readdirSync,rmSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {DatabaseSync} from 'node:sqlite';
import {once} from 'node:events';
import {PaymentStore} from '../payment-store.mjs';
import {AccountAuth,cookieValue} from '../account-auth.mjs';
import {paymentServer} from '../payment-server.mjs';
import {publishStoreConfig} from '../connect-website.mjs';
const pair=generateKeyPairSync('ec',{namedCurve:'prime256v1'}),pem=pair.privateKey.export({type:'pkcs8',format:'pem'}),now=1800000000,account='12345678901234',va='96247TESTVA';
const make=options=>new PaymentStore({file:':memory:',pem,account,virtualAccount:va,now:()=>now,publish:async()=>{},...options});
const payment=(order,id=1)=>({id,gateway:'BIDV',accountNumber:account,subAccount:va,transferType:'in',transferAmount:order.amount,content:order.code,transactionDate:new Date((now+7*3600)*1000).toISOString().slice(0,19).replace('T',' ')});
async function member(store,name='buyerone'){const auth=new AccountAuth(store.db);const token=await auth.register(name,'correct-horse-battery','local');return {auth,token,user:auth.me(cookieValue(token))};}
test('registration uses salted password hashes; login session is revoked by logout and expires',async()=>{
 const store=make();try{
  const one=await member(store),two=await member(store,'buyertwo');
  const hashes=store.db.prepare('SELECT password_hash,salt FROM accounts').all();assert.notEqual(hashes[0].password_hash,hashes[1].password_hash);assert.ok(!JSON.stringify(hashes).includes('correct-horse'));
  await assert.rejects(one.auth.login('buyerone','wrong-password','other'),{status:401});
  const token=await one.auth.login('BUYERONE','correct-horse-battery','other');assert.equal(one.auth.me(cookieValue(token)).id,one.user.id);
  one.auth.logout(cookieValue(token));assert.equal(one.auth.me(cookieValue(token)),null);assert.equal(two.user.balance,0);
  const expired=new AccountAuth(store.db,{now:()=>Math.floor(Date.now()/1000)+8*86400});assert.equal(expired.me(cookieValue(one.token)),null);
 }finally{store.close();}
});
test('a verified topup credits only its account once; unrelated and replayed transfers cannot create credit',async()=>{
 const store=make();try{
  const a=await member(store),b=await member(store,'otherbuyer');const topup=store.createTopup(a.user.id,29000);
  assert.equal(store.webhook({...payment(topup),transferAmount:1}),'ignored');assert.equal(a.auth.me(cookieValue(a.token)).balance,0);
  assert.equal(store.webhook(payment(topup)),'accepted');assert.equal(store.webhook(payment(topup)),'duplicate');assert.equal(store.view(topup.id).state,'credited');
  assert.equal(a.auth.me(cookieValue(a.token)).balance,29000);assert.equal(b.auth.me(cookieValue(b.token)).balance,0);assert.equal(store.ledger(a.user.id).length,1);
  const other=store.createTopup(b.user.id,29000);assert.throws(()=>store.webhook(payment(other)),/transaction_conflict/);assert.equal(b.auth.me(cookieValue(b.token)).balance,0);
 }finally{store.close();}
});
test('wallet purchase debits once, retries the same order and never uses client-supplied pricing',async()=>{
 const store=make();try{
  const a=await member(store),topup=store.createTopup(a.user.id,79000);store.webhook(payment(topup));
  const order=store.walletBuy(a.user.id,'week','request-000000000001');const retry=store.walletBuy(a.user.id,'week','request-000000000001');assert.equal(order.id,retry.id);
  assert.equal(a.auth.me(cookieValue(a.token)).balance,50000);assert.equal(store.ledger(a.user.id).filter(l=>l.kind==='purchase').length,1);assert.equal(store.view(order.id).key,undefined);
  await store.flush();assert.equal(store.view(order.id).key.length,15);
  assert.throws(()=>store.walletBuy(a.user.id,'month','request-000000000001'),{status:409});assert.throws(()=>store.walletBuy(a.user.id,'year','request-000000000002'),{status:409});assert.equal(a.auth.me(cookieValue(a.token)).balance,50000);
  store.pem='invalid private key';assert.throws(()=>store.walletBuy(a.user.id,'week','request-000000000003'));assert.equal(a.auth.me(cookieValue(a.token)).balance,50000);assert.equal(store.ledger(a.user.id).filter(l=>l.kind==='purchase').length,1);
 }finally{store.close();}
});
test('wallet and paid-key records survive reopening without a second debit',async()=>{
 const dir=mkdtempSync(join(tmpdir(),'svx-wallet-')),file=join(dir,'payments.sqlite');let store=make({file});
 try{const a=await member(store),topup=store.createTopup(a.user.id,29000);store.webhook(payment(topup));const first=store.walletBuy(a.user.id,'week','request-000000000001');store.close();store=make({file});const second=store.walletBuy(a.user.id,'week','request-000000000001');assert.equal(first.id,second.id);assert.equal(store.db.prepare('SELECT balance FROM accounts WHERE id=?').get(a.user.id).balance,0);assert.equal(store.ledger(a.user.id).length,2);}
 finally{store.close();rmSync(dir,{recursive:true,force:true});}
});
test('migration snapshots the legacy payment database and preserves its orders',()=>{
 const dir=mkdtempSync(join(tmpdir(),'svx-migrate-')),file=join(dir,'payments.sqlite');const legacy=new DatabaseSync(file);
 legacy.exec("CREATE TABLE orders(id TEXT PRIMARY KEY,code TEXT UNIQUE,plan TEXT,amount INTEGER,created INTEGER,deadline INTEGER,state TEXT,access_key TEXT,receipt TEXT,hash TEXT);CREATE TABLE payments(id TEXT PRIMARY KEY,digest TEXT,order_id TEXT);");
 legacy.prepare("INSERT INTO orders(id,code,plan,amount,created,deadline,state) VALUES(?,?,'week',29000,?,?,'pending')").run('a'.repeat(48),'SVX0123456789ABCDEF',now,now+1800);legacy.close();let store;
 try{store=make({file});assert.equal(store.view('a'.repeat(48)).amount,29000);const backups=readdirSync(join(dir,'backups'));assert.equal(backups.length,1);const snapshot=new DatabaseSync(join(dir,'backups',backups[0]),{readOnly:true});assert.equal(snapshot.prepare('SELECT COUNT(*) AS n FROM orders').get().n,1);assert.ok(!snapshot.prepare('PRAGMA table_info(orders)').all().some(c=>c.name==='kind'));snapshot.close();}
 finally{store?.close();rmSync(dir,{recursive:true,force:true});}
});
test('account HTTP routes enforce session ownership, secure cookies and origin checks',async()=>{
 const store=make(),origin='https://checkout.example.com',secret='s'.repeat(64),admin='a'.repeat(64);const server=paymentServer({store,webhookKey:secret,adminKey:admin,publicURL:origin});server.listen(0,'127.0.0.1');await once(server,'listening');const base='http://127.0.0.1:'+server.address().port;
 const post=(path,body,cookie='',source=origin)=>fetch(base+path,{method:'POST',headers:{'Content-Type':'application/json',Origin:source,Cookie:cookie},body:JSON.stringify(body)});
 try{
  const create=await post('/api/auth/register',{username:'webbuyer',password:'password-long-enough'});assert.equal(create.status,200);const cookie=create.headers.get('set-cookie').split(';')[0];assert.match(create.headers.get('set-cookie'),/HttpOnly; Secure; SameSite=Lax/);
  assert.equal((await post('/api/account/topups',{amount:29000},cookie,'https://evil.example')).status,403);
  assert.equal((await post('/api/account/topups',{amount:29000})).status,401);
  const a=await (await fetch(base+'/api/account',{headers:{Cookie:cookie}})).json();
  const b=await member(store,'secondwebbuyer');const foreign=store.create('year',{userId:b.user.id});
  const own=await (await post('/api/account/orders',{plan:'week',amount:1,userId:b.user.id},cookie)).json();assert.equal(own.order.amount,29000);assert.equal(store.myOrders(a.user.id)[0].id,own.order.id);
  const result=await (await fetch(base+'/api/account?user_id='+b.user.id,{headers:{Cookie:cookie}})).json();assert.ok(!result.orders.some(o=>o.id===foreign.id));
  const topup=(await (await post('/api/account/topups',{amount:29000},cookie)).json()).order;
  const webhook=await fetch(base+'/webhooks/sepay',{method:'POST',headers:{'Content-Type':'application/json',Authorization:'Apikey '+secret},body:JSON.stringify(payment(topup))});assert.equal(webhook.status,201);
  assert.equal((await post('/api/account/wallet-buy',{plan:'week',requestKey:'request-000000000001',expectedAmount:1},cookie)).status,409);
  assert.equal(store.db.prepare('SELECT balance FROM accounts WHERE id=?').get(a.user.id).balance,29000);
  const buy=await post('/api/account/wallet-buy',{plan:'week',requestKey:'request-000000000001',amount:1},cookie);assert.equal(buy.status,201);assert.equal((await buy.json()).order.amount,29000);
  const after=await (await fetch(base+'/api/account',{headers:{Cookie:cookie}})).json();assert.equal(after.user.balance,0);
  await post('/api/auth/logout',{},cookie);assert.equal((await fetch(base+'/api/account',{headers:{Cookie:cookie}})).status,401);
 }finally{server.closeAllConnections();await new Promise(r=>server.close(r));store.close();}
});
test('website connection publishes only public configuration and never sends credentials to checkout host',async()=>{
 let published;
 const publicURL='https://checkout.example.com',token='private-test-token';
 const fetcher=async(url,options)=>{
  if(url.startsWith(publicURL)){assert.equal(options.headers,undefined);return new Response(JSON.stringify({apiVersion:1,publicOrigin:publicURL,features:['wallet']}));}
  assert.equal(options.headers.Authorization,'Bearer '+token);
  if(options.method==='PUT'){published=JSON.parse(options.body);return new Response('{}',{status:201});}return new Response('{}',{status:404});
 };
 await publishStoreConfig({publicURL,token,fetcher});const value=JSON.parse(Buffer.from(published.content,'base64').toString());assert.deepEqual(Object.keys(value).sort(),['apiBase','discordInvite','version']);assert.ok(!JSON.stringify(value).includes(token));
 await assert.rejects(publishStoreConfig({publicURL,token,fetcher:async()=>new Response('{}',{status:404})}),/Backend chua san sang/);
});
