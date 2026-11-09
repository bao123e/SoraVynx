import {test} from 'node:test';
import assert from 'node:assert/strict';
import {orderQRReply,orderTextReply} from '../order-reply.mjs';
const order={code:'SVX0123456789ABCDEF',plan:'week',amount:29000,qr:'https://qr.sepay.vn/img?acc=123456789&bank=BIDV&amount=29000&des=SVX0123456789ABCDEF',url:'https://example.com/checkout#'+'a'.repeat(48)};
const image=Buffer.from('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAwMCAO+aT1sAAAAASUVORK5CYII=','base64');
test('order reply attaches a QR image with correct payment fields and private checkout button',async()=>{
 let calls=0;
 const reply=await orderQRReply(order,{fetcher:async(url,options)=>{
  calls++;assert.ok(url.startsWith('https://qr.sepay.vn/img?'));assert.ok(!url.includes(order.url.split('#')[1]));assert.equal(options.redirect,'error');
  return new Response(image,{headers:{'content-type':'image/png'}});
 }});
 assert.equal(calls,1);assert.deepEqual(reply.files[0].attachment,image);
 const embed=reply.embeds[0].toJSON();assert.equal(embed.image.url,'attachment://soravynx-qr.png');
 assert.equal(embed.fields.find(f=>f.name==='Số tiền').value,'29.000đ');assert.ok(embed.fields.some(f=>f.value==='`'+order.code+'`'));
 assert.equal(reply.components[0].toJSON().components[0].url,order.url);assert.deepEqual(reply.allowedMentions,{parse:[]});
});
test('QR download failure retains the same order, external image and receive-key button',async()=>{
 for(const fetcher of [async()=>{throw new Error('offline');},async()=>new Response('not an image',{headers:{'content-type':'text/html'}}),async()=>new Response('bad png',{headers:{'content-type':'image/png'}}),async()=>new Response(Buffer.alloc(1024*1024+1),{headers:{'content-type':'image/png'}})]){
  const reply=await orderQRReply(order,{fetcher});assert.equal(reply.files.length,0);assert.equal(reply.embeds[0].toJSON().image.url,order.qr);assert.equal(reply.components[0].toJSON().components[0].url,order.url);
 }
 const fallback=orderTextReply(order);assert.ok(fallback.content.includes(order.url));assert.ok(fallback.content.includes(order.code));
});
test('only the fixed QR service is contacted, never arbitrary URLs',async()=>{
 await assert.rejects(orderQRReply({...order,qr:'http://127.0.0.1:8787/admin/orders'},{fetcher:()=>assert.fail('must not fetch')}));
});
