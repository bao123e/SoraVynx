import {isIP} from 'node:net';
import {AccountAuth,cookieValue} from './account-auth.mjs';
import {prices} from './payment-store.mjs';
import {accountHTML,accountJS,accountCSS} from './account-ui.mjs';
export function commerceRoutes({store,publicURL,discordInvite='https://discord.gg/J3BevcpAxF'}){
 const auth=new AccountAuth(store.db);
 const origin=publicURL?new URL(publicURL).origin:null;
 return async(req,res)=>{
  const path=(req.url||'').split('?')[0];
  if(!['/account','/account.js','/account.css','/api/public/config','/api/auth/register','/api/auth/login','/api/auth/logout','/api/account','/api/account/orders','/api/account/topups','/api/account/wallet-buy'].includes(path))return false;
  const send=(status,data,headers={})=>{res.writeHead(status,{'Content-Type':'application/json; charset=utf-8','Cache-Control':'no-store','X-Content-Type-Options':'nosniff',...headers});res.end(JSON.stringify(data));};
  try{
   if(req.method==='GET'&&['/account','/account.js','/account.css'].includes(path)){
    const asset=path==='/account'?[accountHTML,'text/html']:path==='/account.js'?[accountJS,'application/javascript']:[accountCSS,'text/css'];
    res.writeHead(200,{'Content-Type':asset[1]+'; charset=utf-8','Cache-Control':'no-store','Referrer-Policy':'no-referrer','X-Frame-Options':'DENY','Content-Security-Policy':"default-src 'none'; script-src 'self'; style-src 'self'; img-src https://qr.sepay.vn; connect-src 'self'; form-action 'self'; base-uri 'none'; frame-ancestors 'none'"});res.end(asset[0]);return true;
   }
   if(req.method==='GET'&&path==='/api/public/config'){send(200,{apiVersion:1,publicOrigin:origin,features:['accounts','wallet','direct-qr'],prices,discordInvite});return true;}
   if(req.method==='GET'&&path==='/api/account'){
    const user=auth.me(req.headers.cookie);if(!user){send(401,{error:'Bạn cần đăng nhập.'});return true;}
    send(200,{user,orders:store.myOrders(user.id),ledger:store.ledger(user.id)});return true;
   }
   if(req.method!=='POST'){send(405,{error:'Phương thức không hợp lệ.'});return true;}
   if(!origin||req.headers.origin!==origin){send(403,{error:'Yêu cầu không hợp lệ. Mở trang tài khoản và thử lại.'});return true;}
   if(!/^application\/json(?:;|$)/i.test(req.headers['content-type']||'')){send(415,{error:'Dữ liệu không hợp lệ.'});return true;}
   let bytes=0;const chunks=[];for await(const part of req){bytes+=part.length;if(bytes>16384){send(413,{error:'Dữ liệu quá dài.'});return true;}chunks.push(part);}
   let body;try{body=JSON.parse(Buffer.concat(chunks).toString('utf8'));if(!body||Array.isArray(body))throw new Error();}catch{send(400,{error:'Dữ liệu không hợp lệ.'});return true;}
   const forwarded=req.headers['cf-connecting-ip'];const ip=isIP(forwarded||'')&&['127.0.0.1','::1'].includes(req.socket.remoteAddress)?forwarded:req.socket.remoteAddress;
   if(path==='/api/auth/register'||path==='/api/auth/login'){
    const token=await auth[path.endsWith('register')?'register':'login'](body.username,body.password,ip);
    send(200,{user:auth.me(cookieValue(token))},{'Set-Cookie':cookieValue(token)});return true;
   }
   if(path==='/api/auth/logout'){auth.logout(req.headers.cookie);send(200,{ok:true},{'Set-Cookie':cookieValue('',true)});return true;}
   const user=auth.me(req.headers.cookie);if(!user){send(401,{error:'Bạn cần đăng nhập.'});return true;}
   auth.limit('orders:'+user.id,30,3600);
   let order;
   if(path==='/api/account/orders'){
    if(!Object.hasOwn(prices,body.plan)){send(400,{error:'Gói key không hợp lệ.'});return true;}
    order=store.create(body.plan,{userId:user.id});
   }else if(path==='/api/account/topups')order=store.createTopup(user.id,body.amount);
   else if(path==='/api/account/wallet-buy'){
    if(body.expectedAmount!==undefined&&body.expectedAmount!==prices[body.plan]){send(409,{error:'Giá bán đã thay đổi. Tải lại trang trước khi xác nhận mua.'});return true;}
    order=store.walletBuy(user.id,body.plan,body.requestKey);
   }
   else{send(404,{error:'Không tìm thấy.'});return true;}
   send(201,{order});void store.flush().catch(()=>{});return true;
  }catch(e){send(e.status||500,{error:e.status?e.message:'Chưa xử lý được yêu cầu. Vui lòng thử lại sau.'});return true;}
 };
}
