// Observe backend status only. This worker never confirms a payment or issues a key.
import {DatabaseSync} from 'node:sqlite';
import {mkdirSync,chmodSync} from 'node:fs';
import {dirname} from 'node:path';
export class OrderDelivery {
 constructor({file,owner,guild,allowBuyers=false,fetchOrder,sendDM,now=()=>Math.floor(Date.now()/1000)}){
  if(file!==':memory:')mkdirSync(dirname(file),{recursive:true,mode:0o700});
  this.db=new DatabaseSync(file);if(file!==':memory:')chmodSync(file,0o600);
  this.db.exec(`PRAGMA journal_mode=WAL; PRAGMA busy_timeout=5000;
   CREATE TABLE IF NOT EXISTS deliveries(order_id TEXT PRIMARY KEY,code TEXT NOT NULL,user_id TEXT NOT NULL,done INTEGER NOT NULL DEFAULT 0,next_try INTEGER NOT NULL DEFAULT 0,lease_until INTEGER NOT NULL DEFAULT 0);`);
  Object.assign(this,{owner,guild,allowBuyers,fetchOrder,sendDM,now});this.live=new Map();this.working=false;
 }
 track(order,interaction){
  const buyer=interaction.user.id;
  if(!/^[a-f0-9]{48}$/.test(order.id)||!/^SVX[A-F0-9]{16}$/.test(order.code)||!/^\d{17,20}$/.test(buyer)||(this.allowBuyers?interaction.guildId!==this.guild:buyer!==this.owner))throw new Error('invalid_delivery_recipient');
  const existing=this.db.prepare('SELECT user_id FROM deliveries WHERE order_id=?').get(order.id);if(existing&&existing.user_id!==buyer)throw new Error('recipient_mismatch');
  this.db.prepare('INSERT INTO deliveries(order_id,code,user_id) VALUES(?,?,?) ON CONFLICT(order_id) DO NOTHING').run(order.id,order.code,buyer);
  this.live.set(order.id,{interaction,until:Math.floor(interaction.createdTimestamp/1000)+14*60,state:'pending'});
 }
 async tick(){
  if(this.working)return;this.working=true;
  try{
   for(let i=0;i<10;i++){
    const now=this.now();
    const row=this.db.prepare(`UPDATE deliveries SET lease_until=? WHERE order_id=(SELECT order_id FROM deliveries WHERE done=0 AND next_try<=? AND lease_until<=? ORDER BY next_try LIMIT 1) RETURNING *`).get(now+120,now,now);
    if(!row)break;
    let delivered=false,delay=5;
    try{
     if(!/^\d{17,20}$/.test(row.user_id)||(!this.allowBuyers&&row.user_id!==this.owner))throw new Error('recipient_mismatch');
     const order=await this.fetchOrder(row.order_id);
     if(order.id!==row.order_id||order.code!==row.code)throw new Error('order_mismatch');
     let live=this.live.get(row.order_id);
     if(live&&this.now()>=live.until){this.live.delete(row.order_id);live=null;}
     if(order.state==='ready'){
      if(!/^[A-HJ-NP-Z2-9]{15}$/.test(order.key||''))throw new Error('missing_registered_key');
      const content='**Đã thanh toán — SoraVynx**\nĐơn: `'+order.code+'`\nKey truy cập: `'+order.key+'`\nKey đã được đăng ký. Lưu riêng mã này để vào tool.';
      const reply={content,embeds:[],components:[],attachments:[],allowedMentions:{parse:[]}};
      if(live){try{await live.interaction.editReply(reply);delivered=true;}catch{this.live.delete(row.order_id);}}
      // Interaction tokens expire. A private DM also survives a bot restart.
      if(!delivered){await this.sendDM(row.user_id,{...reply,nonce:row.order_id.slice(0,24),enforceNonce:true});delivered=true;}
     }else if(order.state==='publishing'){
      if(live&&live.state!=='publishing'){
       try{await live.interaction.editReply({content:'**Đã nhận tiền — đang đăng ký key.**\nĐơn: `'+order.code+'`\nBot sẽ tự hiện key khi đăng ký xong. Không chuyển thêm tiền.',embeds:[],components:[],attachments:[],allowedMentions:{parse:[]}});live.state='publishing';}catch{this.live.delete(row.order_id);}
      }
     }else if(order.state==='expired')delay=60;
    }catch{delay=30;/* Keep the order for retry; never create a replacement key. */}
    if(delivered){this.db.prepare('UPDATE deliveries SET done=1,lease_until=0 WHERE order_id=?').run(row.order_id);this.live.delete(row.order_id);}
    else this.db.prepare('UPDATE deliveries SET next_try=?,lease_until=0 WHERE order_id=?').run(this.now()+delay,row.order_id);
   }
  }finally{this.working=false;}
 }
 start(){this.timer=setInterval(()=>{void this.tick().catch(()=>{});},5000);void this.tick().catch(()=>{});}
 close(){clearInterval(this.timer);this.db.close();}
}
