import {createServer} from 'node:http';
import {timingSafeEqual} from 'node:crypto';
import {resolve} from 'node:path';
import {fileURLToPath} from 'node:url';
import {directory,matchingIssuer} from './owner-config.mjs';
import {PaymentStore} from './payment-store.mjs';
import {checkoutHTML} from './payment-ui.mjs';
export function equalSecret(actual,expected){const a=Buffer.from(actual||''),b=Buffer.from(expected||'');return b.length>=32&&a.length===b.length&&timingSafeEqual(a,b);}
export function paymentServer({store,webhookKey,adminKey,publicURL}){
 if(webhookKey?.length<32||adminKey?.length<32||!webhookKey||!adminKey)throw new Error('Can khoa webhook/admin it nhat 32 ky tu.');
 return createServer(async(req,res)=>{
  const send=(status,data)=>{res.writeHead(status,{'Content-Type':'application/json; charset=utf-8','Cache-Control':'no-store','X-Content-Type-Options':'nosniff'});res.end(JSON.stringify(data));};
  const path=(req.url||'').split('?')[0];
  if(req.method==='GET'&&path==='/health'){send(200,{ok:true});return;}
  if(req.method==='GET'&&path==='/checkout'){
   res.writeHead(200,{'Content-Type':'text/html; charset=utf-8','Cache-Control':'no-store','Referrer-Policy':'no-referrer','X-Frame-Options':'DENY','Content-Security-Policy':"default-src 'none'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; img-src https://qr.sepay.vn; connect-src 'self'; frame-ancestors 'none'; base-uri 'none'"});res.end(checkoutHTML);return;
  }
  const status=path.match(/^\/api\/orders\/([a-f\d]{48})$/);
  if(req.method==='GET'&&status){const order=store.view(status[1]);send(order?200:404,order||{error:'not_found'});return;}
  if(req.method!=='POST'||!['/webhooks/sepay','/admin/orders'].includes(path)){send(404,{error:'not_found'});return;}
  const webhook=path==='/webhooks/sepay';
  if(!equalSecret(req.headers.authorization,(webhook?'Apikey ':'Bearer ')+(webhook?webhookKey:adminKey))){send(401,{success:false});return;}
  if(!/^application\/json(?:;|$)/i.test(req.headers['content-type']||'')){send(415,{success:false});return;}
  let body,bytes=0;const parts=[];
  try{
   for await(const part of req){bytes+=part.length;if(bytes>16384){send(413,{success:false});return;}parts.push(part);}
   body=JSON.parse(Buffer.concat(parts).toString('utf8'));
  }catch{send(400,{success:false});return;}
  try{
   if(webhook){const outcome=store.webhook(body);send(201,{success:true,result:outcome});void store.flush();}
   else{
    if(!publicURL||!/^https:\/\/[a-z\d.-]+(?::\d+)?$/i.test(publicURL)){send(503,{error:'configure_https_public_url'});return;}
    const order=store.create(body.plan);send(201,{...order,url:publicURL+'/checkout#'+order.id});
   }
  }catch(e){send(e.message==='transaction_conflict'?409:e.message==='Goi key khong hop le.'||e.message==='invalid_payload'||e.message==='invalid_date'?400:503,{success:false});}
 });
}
if(process.argv[1]&&resolve(process.argv[1])===fileURLToPath(import.meta.url)){
 try{
  if(Number(process.versions.node.split('.')[0])<24)throw new Error('Can Node.js 24 LTS.');
  if(!process.env.GITHUB_TOKEN)throw new Error('Chua co GITHUB_TOKEN. Mo 3-Setup-Bot.cmd.');
  const store=new PaymentStore({file:resolve(directory,process.env.PAYMENT_DB_FILE||'secrets/payments.sqlite'),pem:matchingIssuer(process.env.LICENSE_PRIVATE_KEY_FILE||'secrets/issuer-private.pem'),account:process.env.SEPAY_ACCOUNT_NUMBER,gateway:process.env.SEPAY_GATEWAY||'BIDV'});
  const server=paymentServer({store,webhookKey:process.env.SEPAY_WEBHOOK_API_KEY,adminKey:process.env.PAYMENT_ADMIN_KEY,publicURL:process.env.PUBLIC_BASE_URL});
  server.requestTimeout=15000;server.headersTimeout=10000;
  server.listen(8787,'127.0.0.1',()=>console.log('SoraVynx webhook dang chay tai http://127.0.0.1:8787. Giu cua so nay mo. Huong dan: SEPAY.vi.md.'));
  const timer=setInterval(()=>{void store.flush();},15000);void store.flush();
  for(const signal of ['SIGINT','SIGTERM'])process.on(signal,()=>{clearInterval(timer);server.close(()=>process.exit(0));});
 }catch(e){console.error(e.message);process.exitCode=1;}
}
