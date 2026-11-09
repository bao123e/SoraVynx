import {mkdirSync,chmodSync} from 'node:fs';
import {dirname,join} from 'node:path';
import {randomBytes} from 'node:crypto';
export function commerceSchema(db,file){
 const columns=db.prepare('PRAGMA table_info(orders)').all().map(c=>c.name);
 if(!columns.includes('kind')&&file!==':memory:'){
  const folder=join(dirname(file),'backups');mkdirSync(folder,{recursive:true,mode:0o700});
  const backup=join(folder,'payments-before-accounts-'+Date.now()+'-'+randomBytes(4).toString('hex')+'.sqlite');
  db.exec("VACUUM INTO '"+backup.replaceAll("'","''")+"'");chmodSync(backup,0o600);
 }
 db.exec('BEGIN IMMEDIATE');
 try{
  if(!columns.includes('kind'))db.exec("ALTER TABLE orders ADD COLUMN kind TEXT NOT NULL DEFAULT 'license'");
  if(!columns.includes('user_id'))db.exec('ALTER TABLE orders ADD COLUMN user_id TEXT');
  db.exec(`CREATE TABLE IF NOT EXISTS accounts(id TEXT PRIMARY KEY,username TEXT UNIQUE NOT NULL,password_hash TEXT NOT NULL,salt TEXT NOT NULL,balance INTEGER NOT NULL DEFAULT 0 CHECK(balance>=0),created INTEGER NOT NULL);
   CREATE TABLE IF NOT EXISTS sessions(hash TEXT PRIMARY KEY,user_id TEXT NOT NULL REFERENCES accounts(id),expires INTEGER NOT NULL,created INTEGER NOT NULL);
   CREATE INDEX IF NOT EXISTS sessions_user ON sessions(user_id);
   CREATE TABLE IF NOT EXISTS wallet_ledger(id INTEGER PRIMARY KEY,user_id TEXT NOT NULL REFERENCES accounts(id),amount INTEGER NOT NULL,kind TEXT NOT NULL,reference TEXT UNIQUE NOT NULL,created INTEGER NOT NULL);
   CREATE TABLE IF NOT EXISTS wallet_requests(user_id TEXT NOT NULL,request_key TEXT NOT NULL,order_id TEXT NOT NULL REFERENCES orders(id),plan TEXT NOT NULL,PRIMARY KEY(user_id,request_key));
   CREATE INDEX IF NOT EXISTS orders_user ON orders(user_id);
   PRAGMA user_version=2;`);
  db.exec('COMMIT');
 }catch(e){db.exec('ROLLBACK');throw e;}
}
