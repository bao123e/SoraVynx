import {randomBytes,createHash,scrypt,timingSafeEqual} from 'node:crypto';
import {promisify} from 'node:util';
const derive=promisify(scrypt),duration=7*86400;
export const sessionCookie='__Host-soravynx';
export function cookieValue(token='',clear=false){return sessionCookie+'='+token+'; Path=/; HttpOnly; Secure; SameSite=Lax; Max-Age='+(clear?0:duration);}
function sessionHash(token){return createHash('sha256').update(token).digest('hex');}
export class AccountAuth {
 constructor(db,{now=()=>Math.floor(Date.now()/1000)}={}){this.db=db;this.now=now;this.inflight=0;this.attempts=new Map();}
 limit(bucket,max=10,window=900){
  const now=this.now(),entry=this.attempts.get(bucket);
  if(!entry||entry.until<=now){if(this.attempts.size>10000)this.attempts.clear();this.attempts.set(bucket,{count:1,until:now+window});return;}
  if(++entry.count>max)throw Object.assign(new Error('Thao tác quá nhiều. Vui lòng thử lại sau.'),{status:429});
 }
 async credentials(username,password){
  if(typeof username!=='string'||typeof password!=='string')throw Object.assign(new Error('Nhập tên đăng nhập và mật khẩu.'),{status:400});
  username=username.trim().toLowerCase();
  if(!/^[a-z0-9_.-]{3,32}$/.test(username)||password.length<8||Buffer.byteLength(password)>256)throw Object.assign(new Error('Tên đăng nhập 3–32 ký tự a–z, số, dấu . _ -; mật khẩu ít nhất 8 ký tự.'),{status:400});
  return {username,password};
 }
 async hash(password,salt){
  if(this.inflight>=4)throw Object.assign(new Error('Máy chủ đang bận, thử lại sau.'),{status:429});
  this.inflight++;try{return await derive(password,salt,64,{N:16384,r:8,p:1,maxmem:64*1024*1024});}finally{this.inflight--;}
 }
 session(userId){
  const token=randomBytes(32).toString('hex'),now=this.now();
  this.db.prepare('DELETE FROM sessions WHERE expires<=?').run(now);
  this.db.prepare('INSERT INTO sessions(hash,user_id,expires,created) VALUES(?,?,?,?)').run(sessionHash(token),userId,now+duration,now);return token;
 }
 async register(username,password,ip){
  this.limit('register:'+ip,10,3600);const c=await this.credentials(username,password);
  const salt=randomBytes(16).toString('hex'),hash=(await this.hash(c.password,salt)).toString('hex'),id=randomBytes(16).toString('hex');
  try{this.db.prepare('INSERT INTO accounts(id,username,password_hash,salt,created) VALUES(?,?,?,?,?)').run(id,c.username,hash,salt,this.now());}
  catch(e){if(e.code?.includes('CONSTRAINT')||String(e.message).includes('UNIQUE'))throw Object.assign(new Error('Tên đăng nhập đã được sử dụng.'),{status:409});throw e;}
  return this.session(id);
 }
 async login(username,password,ip){
  this.limit('login-ip:'+ip,100);const c=await this.credentials(username,password);this.limit('login-user:'+c.username);
  const account=this.db.prepare('SELECT * FROM accounts WHERE username=?').get(c.username);
  const hash=await this.hash(c.password,account?.salt||'00000000000000000000000000000000');
  if(!account||!timingSafeEqual(hash,Buffer.from(account.password_hash,'hex')))throw Object.assign(new Error('Tên đăng nhập hoặc mật khẩu không đúng.'),{status:401});
  return this.session(account.id);
 }
 token(cookie){const value=(cookie||'').split(';').map(s=>s.trim()).find(s=>s.startsWith(sessionCookie+'='))?.slice(sessionCookie.length+1);return /^[a-f0-9]{64}$/.test(value||'')?value:null;}
 me(cookie){const token=this.token(cookie);if(!token)return null;return this.db.prepare('SELECT a.id,a.username,a.balance FROM sessions s JOIN accounts a ON a.id=s.user_id WHERE s.hash=? AND s.expires>?').get(sessionHash(token),this.now())||null;}
 logout(cookie){const token=this.token(cookie);if(token)this.db.prepare('DELETE FROM sessions WHERE hash=?').run(sessionHash(token));}
}
