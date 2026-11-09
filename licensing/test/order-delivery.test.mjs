import {test} from 'node:test';
import assert from 'node:assert/strict';
import {mkdtempSync,rmSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {OrderDelivery} from '../order-delivery.mjs';
const owner='123456789012345678',id='a'.repeat(48),code='SVX0123456789ABCDEF',key='ABCDEFGH2345678';
function fixture(options={}){
 let clock=1800000000,state='pending',edits=[],dms=[];
 const worker=new OrderDelivery({file:':memory:',owner,now:()=>clock,fetchOrder:async()=>({id,code,state,key}),sendDM:async(user,message)=>dms.push({user,message}),...options});
 const interaction={user:{id:owner},createdTimestamp:clock*1000,editReply:async message=>edits.push(message)};
 return {worker,interaction,edits,dms,setState:s=>state=s,advance:n=>clock+=n};
}
test('QR stays pending; publishing removes QR without exposing a key; ready edits the private invoice once',async()=>{
 const f=fixture();try{
  f.worker.track({id,code},f.interaction);await f.worker.tick();assert.equal(f.edits.length,0);
  f.setState('publishing');f.advance(5);await f.worker.tick();assert.equal(f.edits.length,1);assert.ok(!f.edits[0].content.includes(key));assert.deepEqual(f.edits[0].attachments,[]);
  f.setState('ready');f.advance(5);await f.worker.tick();assert.ok(f.edits[1].content.includes(key));assert.equal(f.dms.length,0);
  f.advance(5);await f.worker.tick();assert.equal(f.edits.length,2);
 }finally{f.worker.close();}
});
test('expired interaction uses only the configured owner DM with a stable nonce',async()=>{
 const f=fixture();try{
  f.worker.track({id,code},f.interaction);f.advance(15*60);f.setState('ready');await f.worker.tick();
  assert.equal(f.edits.length,0);assert.equal(f.dms.length,1);assert.equal(f.dms[0].user,owner);assert.ok(f.dms[0].message.content.includes(key));assert.equal(f.dms[0].message.enforceNonce,true);
  assert.throws(()=>f.worker.track({id,code},{...f.interaction,user:{id:'other'}}));
 }finally{f.worker.close();}
});
test('pending delivery survives restart; blocked DM retries the same key instead of issuing another',async()=>{
 const dir=mkdtempSync(join(tmpdir(),'svx-delivery-')),file=join(dir,'delivery.sqlite');let clock=1800000000,sent=0;
 let f=fixture({file,now:()=>clock});f.worker.track({id,code},f.interaction);f.worker.close();
 const create=sendDM=>new OrderDelivery({file,owner,now:()=>clock,fetchOrder:async()=>({id,code,state:'ready',key}),sendDM});
 let worker=create(async()=>{throw new Error('DM disabled');});
 try{
  await worker.tick();assert.equal(worker.db.prepare('SELECT done FROM deliveries').get().done,0);worker.close();clock+=31;
  worker=create(async(user,message)=>{assert.equal(user,owner);assert.ok(message.content.includes(key));sent++;});await worker.tick();await worker.tick();assert.equal(sent,1);
 }finally{worker.close();rmSync(dir,{recursive:true,force:true});}
});
test('unknown order or missing ready key never sends a message',async()=>{
 for(const response of [{id:'b'.repeat(48),code,state:'ready',key},{id,code,state:'ready'},{id,code,state:'pending',key}]){
  const f=fixture({fetchOrder:async()=>response});try{f.worker.track({id,code},f.interaction);await f.worker.tick();assert.equal(f.edits.length+f.dms.length,0);}finally{f.worker.close();}
 }
});
test('if editing the invoice fails, the same registered key falls back to a private DM',async()=>{
 const f=fixture();try{
  f.interaction.editReply=async()=>{throw new Error('interaction expired');};f.worker.track({id,code},f.interaction);f.setState('ready');await f.worker.tick();
  assert.equal(f.dms.length,1);assert.equal(f.dms[0].user,owner);assert.ok(f.dms[0].message.content.includes(key));
 }finally{f.worker.close();}
});
test('two bot workers sharing the queue cannot normally deliver the same order concurrently',async()=>{
 const dir=mkdtempSync(join(tmpdir(),'svx-delivery-lease-')),file=join(dir,'delivery.sqlite');let unblock,sent=0;
 const gate=new Promise(resolve=>unblock=resolve);
 const first=fixture({file,fetchOrder:async()=>{await gate;return {id,code,state:'ready',key};}});
 const second=new OrderDelivery({file,owner,now:()=>1800000000,fetchOrder:async()=>({id,code,state:'ready',key}),sendDM:async()=>{sent++;}});
 try{
  first.worker.track({id,code},first.interaction);const pending=first.worker.tick();await second.tick();assert.equal(sent,0);
  unblock();await pending;assert.equal(first.edits.length,1);await second.tick();assert.equal(sent,0);
 }finally{unblock();first.worker.close();second.close();rmSync(dir,{recursive:true,force:true});}
});
