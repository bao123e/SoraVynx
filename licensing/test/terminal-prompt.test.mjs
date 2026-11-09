import {test} from 'node:test';
import assert from 'node:assert/strict';
import {PassThrough,Writable} from 'node:stream';
import {terminalPrompt} from '../terminal-prompt.mjs';
function terminal(){
 const input=new PassThrough();input.isTTY=true;
 let visible='';const output=new Writable({write(chunk,_encoding,done){visible+=chunk.toString();done();}});
 const prompt=terminalPrompt(input,output);
 return {input,prompt,text:()=>visible};
}
test('first question renders its full label and default before input and Enter keeps the default',async()=>{
 const t=terminal();try{
  const answer=t.prompt.ask('Duong dan issuer-private.pem cu','./secrets/issuer-private.pem');
  assert.match(t.text(),/Duong dan issuer-private\.pem cu \[\.\/secrets\/issuer-private\.pem\]: /);
  t.input.write('\r');assert.equal(await answer,'./secrets/issuer-private.pem');
 }finally{t.prompt.close();}
});
test('token question stays visible but typed token and existing token are not echoed',async()=>{
 const t=terminal();try{
  const answer=t.prompt.ask('Dan Bot Token','existing-private-token',true);
  assert.match(t.text(),/Dan Bot Token \[Enter: giu gia tri cu\]: /);
  assert.ok(!t.text().includes('existing-private-token'));
  t.input.write('new-private-token\r');assert.equal(await answer,'new-private-token');
  assert.ok(!t.text().includes('new-private-token'));
  const next=t.prompt.ask('Server ID');assert.match(t.text(),/Server ID: /);
  t.input.write('123\r');assert.equal(await next,'123');
 }finally{t.prompt.close();}
});
