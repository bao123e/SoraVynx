import {test} from 'node:test';
import assert from 'node:assert/strict';
import {canUseCommand} from '../bot-policy.mjs';
import {commandData} from '../command-data.mjs';
import {OrderDelivery} from '../order-delivery.mjs';
const owner='123456789012345678',buyer='223456789012345678',guild='323456789012345678',id='a'.repeat(48),code='SVX0123456789ABCDEF',key='ABCDEFGH2345678';
test('customers may buy only in the configured server and may never use owner issuing commands',()=>{
 assert.equal(canUseCommand('buy',guild,buyer,{owner,guild}),true);
 for(const command of ['keycreate','order','unknown'])assert.equal(canUseCommand(command,guild,buyer,{owner,guild}),false);
 assert.equal(canUseCommand('buy','another-server',buyer,{owner,guild}),false);assert.equal(canUseCommand('keycreate',guild,owner,{owner,guild}),true);
 assert.equal(commandData.find(c=>c.name==='buy').default_member_permissions,null);assert.equal(commandData.find(c=>c.name==='keycreate').default_member_permissions,'8');
});
test('a paid buyer key goes to that buyer, not the owner or a second claimant',async()=>{
 let clock=1800000000,sent=[];
 const worker=new OrderDelivery({file:':memory:',owner,guild,allowBuyers:true,now:()=>clock,fetchOrder:async()=>({id,code,state:'ready',key}),sendDM:async(user,message)=>sent.push({user,message})});
 const interaction={user:{id:buyer},guildId:guild,createdTimestamp:clock*1000,editReply:async()=>assert.fail('expired interaction')};
 try{
  worker.track({id,code},interaction);assert.throws(()=>worker.track({id,code},{...interaction,user:{id:owner}}),/recipient_mismatch/);
  assert.throws(()=>worker.track({id:'b'.repeat(48),code},{...interaction,guildId:'another'}));
  clock+=900;await worker.tick();assert.equal(sent.length,1);assert.equal(sent[0].user,buyer);assert.ok(sent[0].message.content.includes(key));
 }finally{worker.close();}
});
