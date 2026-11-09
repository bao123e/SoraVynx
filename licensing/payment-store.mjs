import {DatabaseSync} from 'node:sqlite';
import {randomBytes,createHash} from 'node:crypto';
import {mkdirSync,chmodSync} from 'node:fs';
import {dirname} from 'node:path';
import {createShortLicense,publishShortLicense} from './issuer.mjs';
export const prices=Object.freeze({week:29000,month:79000,year:499000,lifetime:999000});
async function publishPaidRecord(record){
 await publishShortLicense(record);
 const response=await fetch('https://raw.githubusercontent.com/bao123e/soravynx.github.io/main/licenses/'+record.hash+'.lic',{redirect:'error',signal:AbortSignal.timeout(10000),cache:'no-store'});
 if(!response.ok||(await response.text()).trim()!==record.receipt)throw new Error('public_record_not_ready');
}
export class PaymentStore {
 constructor({file,pem,account,gateway='BIDV',publish=publishPaidRecord,now=()=>Math.floor(Date.now()/1000)}){
  if(!/^\d{6,30}$/.test(account))throw new Error('Chua cau hinh dung SEPAY_ACCOUNT_NUMBER.');
  if(file!==':memory:'){mkdirSync(dirname(file),{recursive:true,mode:0o700});}
  this.db=new DatabaseSync(file);if(file!==':memory:')chmodSync(file,0o600);
  this.db.exec(`PRAGMA journal_mode=WAL; PRAGMA busy_timeout=5000;
   CREATE TABLE IF NOT EXISTS orders(id TEXT PRIMARY KEY,code TEXT UNIQUE NOT NULL,plan TEXT NOT NULL,amount INTEGER NOT NULL,created INTEGER NOT NULL,deadline INTEGER NOT NULL,state TEXT NOT NULL DEFAULT 'pending',access_key TEXT,receipt TEXT,hash TEXT);
   CREATE TABLE IF NOT EXISTS payments(id TEXT PRIMARY KEY,digest TEXT NOT NULL,order_id TEXT UNIQUE NOT NULL REFERENCES orders(id));`);
  Object.assign(this,{pem,account,gateway,publish,now});this.working=false;
 }
 create(plan){
  if(!Object.hasOwn(prices,plan))throw new Error('Goi key khong hop le.');
  const order={id:randomBytes(24).toString('hex'),code:'SVX'+randomBytes(8).toString('hex').toUpperCase(),plan,amount:prices[plan],created:this.now(),deadline:this.now()+1800};
  this.db.prepare('INSERT INTO orders(id,code,plan,amount,created,deadline) VALUES(?,?,?,?,?,?)').run(order.id,order.code,plan,order.amount,order.created,order.deadline);
  return this.view(order.id);
 }
 view(id){
  if(!/^[a-f\d]{48}$/.test(id))return null;
  const row=this.db.prepare('SELECT * FROM orders WHERE id=?').get(id);if(!row)return null;
  return {id:row.id,code:row.code,plan:row.plan,amount:row.amount,deadline:row.deadline,state:row.state==='pending'&&this.now()>row.deadline?'expired':row.state,...(row.state==='ready'?{key:row.access_key}:{}),qr:'https://qr.sepay.vn/img?'+new URLSearchParams({acc:this.account,bank:this.gateway,amount:String(row.amount),des:row.code})};
 }
 webhook(body){
  if(!body||!Number.isSafeInteger(body.id)||body.id<=0||!Number.isSafeInteger(body.transferAmount)||body.transferAmount<=0)throw new Error('invalid_payload');
  if(body.transferType!=='in')return 'ignored';
  if(body.accountNumber!==this.account||String(body.gateway).toUpperCase()!==this.gateway.toUpperCase()||body.subAccount)return 'ignored';
  const text=typeof body.content==='string'?body.content.toUpperCase():'';
  const codes=[...text.matchAll(/(?<![A-Z0-9])SVX[A-F0-9]{16}(?![A-Z0-9])/g)].map(m=>m[0]);
  if(body.code){const code=String(body.code).toUpperCase();if(!/^SVX[A-F0-9]{16}$/.test(code)||(codes.length&&codes.some(c=>c!==code)))return 'ignored';codes.push(code);}
  const unique=[...new Set(codes)];if(unique.length!==1)return 'ignored';
  if(typeof body.transactionDate!=='string'||!/^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}$/.test(body.transactionDate))throw new Error('invalid_date');
  const paid=Math.floor(Date.parse(body.transactionDate.replace(' ','T')+'+07:00')/1000);
  if(!Number.isFinite(paid)||paid>this.now()+300)return 'ignored';
  const digest=createHash('sha256').update(JSON.stringify([body.id,body.transferType,body.transferAmount,body.accountNumber,String(body.gateway).toUpperCase(),unique[0],body.transactionDate])).digest('hex');
  this.db.exec('BEGIN IMMEDIATE');
  try{
   const seen=this.db.prepare('SELECT digest FROM payments WHERE id=?').get(String(body.id));
   if(seen){if(seen.digest!==digest)throw new Error('transaction_conflict');this.db.exec('COMMIT');return 'duplicate';}
   const row=this.db.prepare('SELECT * FROM orders WHERE code=?').get(unique[0]);
   if(!row||row.state!=='pending'||row.amount!==body.transferAmount||paid<row.created-300||paid>row.deadline){this.db.exec('COMMIT');return 'ignored';}
   const record=createShortLicense(this.pem,row.plan,this.now());
   this.db.prepare('INSERT INTO payments(id,digest,order_id) VALUES(?,?,?)').run(String(body.id),digest,row.id);
   this.db.prepare("UPDATE orders SET state='publishing',access_key=?,receipt=?,hash=? WHERE id=?").run(record.key,record.receipt,record.hash,row.id);
   this.db.exec('COMMIT');return 'accepted';
  }catch(e){this.db.exec('ROLLBACK');throw e;}
 }
 async flush(){
  if(this.working)return;this.working=true;
  try{
   const rows=this.db.prepare("SELECT id,hash,receipt FROM orders WHERE state='publishing' LIMIT 10").all();
   for(const row of rows){try{await this.publish(row);this.db.prepare("UPDATE orders SET state='ready' WHERE id=? AND state='publishing'").run(row.id);}catch{/* Retain the same key and retry on the next tick, including after restart. */}}
  }finally{this.working=false;}
 }
 close(){this.db.close();}
}
