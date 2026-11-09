'use strict';
const $=id=>document.getElementById(id),money=n=>new Intl.NumberFormat('vi-VN').format(n)+'đ';
const names={week:'7 ngày',month:'30 ngày',year:'1 năm',lifetime:'Vĩnh viễn',topup:'Nạp ví'};
const states={pending:'Chờ thanh toán',publishing:'Đã nhận tiền · đang đăng ký key',ready:'Đã giao key',credited:'Đã cộng vào ví',expired:'Đã hết hạn'};
let mode='login',config=null,current=null,epoch=0,busy=false,openOrder=null,lastAction=null;
function notice(text='',error=false){$('notice').textContent=text;$('notice').classList.toggle('error',error);}
async function api(path,body){
 const r=await fetch(path,{method:body===undefined?'GET':'POST',credentials:'same-origin',cache:'no-store',headers:body===undefined?{}:{'Content-Type':'application/json'},...(body===undefined?{}:{body:JSON.stringify(body)})});
 const data=await r.json();if(!r.ok)throw Object.assign(new Error(data.error||'Chưa kết nối được dịch vụ.'),{status:r.status});return data;
}
function authMode(value){mode=value;$('login-mode').setAttribute('aria-pressed',String(mode==='login'));$('register-mode').setAttribute('aria-pressed',String(mode==='register'));$('confirm-field').hidden=mode!=='register';$('confirm').required=mode==='register';$('password').autocomplete=mode==='register'?'new-password':'current-password';$('auth-title').textContent=mode==='register'?'Tạo tài khoản SoraVynx.':'Chào bạn trở lại.';$('auth-submit').textContent=mode==='register'?'Đăng ký':'Đăng nhập';notice();}
$('login-mode').onclick=()=>authMode('login');$('register-mode').onclick=()=>authMode('register');
function signedOut(){current=null;lastAction=null;epoch++;$('auth').hidden=false;$('dashboard').hidden=true;$('logout').hidden=true;$('orders').replaceChildren();$('ledger').replaceChildren();$('balance').textContent='0đ';$('payment-key').textContent='';if($('payment-dialog').open)$('payment-dialog').close();}
async function refresh(){
 const requestEpoch=epoch;
 try{const data=await api('/api/account');if(requestEpoch!==epoch)return;current=data;render();}
 catch(e){if(e.status===401)signedOut();else notice(e.message,true);}
}
function row(tag,text,className){const el=document.createElement(tag);el.textContent=text;if(className)el.className=className;return el;}
async function copyKey(key,button){try{await navigator.clipboard.writeText(key);button.textContent='Đã sao chép';}catch{notice('Hãy chọn và sao chép mã key đang hiển thị.',true);}}
function render(){
 $('auth').hidden=true;$('dashboard').hidden=false;$('logout').hidden=false;$('hello').textContent='Chào '+current.user.username+'.';$('balance').textContent=money(current.user.balance);
 $('orders').replaceChildren();$('empty-orders').hidden=!!current.orders.length;$('order-count').textContent=current.orders.length+' đơn gần nhất';
 for(const order of current.orders){
  const article=row('article','','order'),info=document.createElement('div');info.append(row('h3',(names[order.plan]||order.plan)+' · '+money(order.amount)),row('p',order.code),row('span',states[order.state]||order.state,'badge'));
  article.append(info);
  if(order.key){info.append(row('code',order.key));const button=row('button','Sao chép key','ghost');button.onclick=()=>copyKey(order.key,button);article.append(button);}
  else if(order.state==='pending'){const button=row('button','Xem QR','ghost');button.onclick=()=>showPayment(order);article.append(button);}
  $('orders').append(article);
 }
 $('ledger').replaceChildren();$('empty-ledger').hidden=!!current.ledger.length;
 for(const item of current.ledger){const el=row('div','','ledger-row');el.append(row('span',(item.kind==='topup'?'Nạp ví':'Mua key')+' · '+new Date(item.created*1000).toLocaleString('vi-VN',{timeZone:'Asia/Ho_Chi_Minh'})),row('strong',(item.amount>0?'+':'')+money(item.amount),item.amount>0?'positive':''));$('ledger').append(el);}
 if(openOrder){const order=current.orders.find(o=>o.id===openOrder);if(order)updatePayment(order);}
 if(lastAction){const order=current.orders.find(o=>o.id===lastAction);if(order?.state==='ready')notice('Key đã sẵn sàng trong Đơn hàng & key.');else if(order?.state==='credited')notice('Đã nạp '+money(order.amount)+' vào ví.');}
}
function updatePayment(order){
 $('payment-title').textContent=order.kind==='topup'?'Nạp tiền vào ví':'Thanh toán key';$('payment-status').textContent=states[order.state]||'Đang xử lý';$('payment-details').hidden=order.state!=='pending';$('payment-result').hidden=!order.key;
 $('payment-amount').textContent=money(order.amount);$('payment-code').textContent=order.code;
 if(order.state==='pending'){
  const url=new URL(order.qr);if(url.origin==='https://qr.sepay.vn'&&url.pathname==='/img'&&$('payment-qr').src!==url.href)$('payment-qr').src=url.href;
 }
 if(order.key){$('payment-key').textContent=order.key;$('copy-payment-key').onclick=()=>copyKey(order.key,$('copy-payment-key'));}
 if(order.state==='credited')$('payment-status').textContent='Đã nạp '+money(order.amount)+' vào ví. Bạn có thể mua key bằng số dư.';
}
function showPayment(order){openOrder=order.id;updatePayment(order);if(!$('payment-dialog').open)$('payment-dialog').showModal();}
$('close-payment').onclick=()=>{$('payment-dialog').close();openOrder=null;};$('payment-dialog').addEventListener('close',()=>openOrder=null);
$('auth-form').onsubmit=async event=>{
 event.preventDefault();if(busy)return;
 if(mode==='register'&&$('password').value!==$('confirm').value){notice('Mật khẩu nhập lại chưa khớp.',true);return;}
 busy=true;$('auth-submit').disabled=true;
 try{await api('/api/auth/'+mode,{username:$('username').value,password:$('password').value});epoch++;$('password').value='';$('confirm').value='';notice();await refresh();}
 catch(e){notice(e.message,true);}finally{busy=false;$('auth-submit').disabled=false;}
};
$('logout').onclick=async()=>{try{await api('/api/auth/logout',{});signedOut();notice('Đã đăng xuất.');}catch(e){notice(e.message,true);}};
$('refresh').onclick=()=>refresh();
function selectedPrice(){if(config){$('price').textContent=money(config.prices[$('plan').value]);$('buy-submit').disabled=false;}}
$('plan').onchange=selectedPrice;
document.querySelectorAll('[data-amount]').forEach(button=>button.onclick=()=>{$('topup-amount').value=button.dataset.amount;});
async function submit(form,action){if(busy)return;busy=true;const button=form.querySelector('button[type=submit]');button.disabled=true;try{await action();}catch(e){notice(e.message,true);}finally{busy=false;button.disabled=false;}}
$('topup-form').onsubmit=event=>{event.preventDefault();void submit(event.currentTarget,async()=>{const data=await api('/api/account/topups',{amount:Number($('topup-amount').value)});lastAction=data.order.id;notice('Đơn nạp đã tạo. Quét đúng QR của đơn này.');await refresh();showPayment(data.order);});};
$('buy-form').onsubmit=event=>{
 event.preventDefault();if(!config){notice('Chưa lấy được giá bán. Vui lòng tải lại trang.',true);return;}void submit(event.currentTarget,async()=>{
  const plan=$('plan').value,method=document.querySelector('[name=method]:checked').value;
  let body={plan},path='/api/account/orders',storageKey='svx-wallet-request:'+plan;
  if(method==='wallet'){path='/api/account/wallet-buy';let requestKey;try{requestKey=sessionStorage.getItem(storageKey)||crypto.randomUUID();sessionStorage.setItem(storageKey,requestKey);}catch{throw new Error('Cần cho phép lưu phiên trình duyệt để mua bằng ví.');}body.requestKey=requestKey;body.expectedAmount=config.prices[plan];}
  const data=await api(path,body);lastAction=data.order.id;if(method==='wallet')sessionStorage.removeItem(storageKey);notice(method==='wallet'?'Đã trừ ví một lần. Đang đăng ký key; xem ở Đơn hàng & key.':'Đơn đã tạo. Quét QR để thanh toán trực tiếp.');await refresh();showPayment(current?.orders.find(o=>o.id===data.order.id)||data.order);
 });
};
async function init(){try{config=await api('/api/public/config');if(config.discordInvite)document.querySelectorAll('.discord-link').forEach(a=>{const u=new URL(config.discordInvite);if(['discord.gg','discord.com'].includes(u.hostname))a.href=u.href;});const requested=new URL(location.href).searchParams.get('plan');if(Object.hasOwn(config.prices,requested))$('plan').value=requested;selectedPrice();await refresh();}catch(e){notice('Chưa kết nối được cổng thanh toán. Thử lại sau hoặc liên hệ Discord.',true);}}
if(new URL(location.href).searchParams.get('view')==='register')authMode('register');
if(new URL(location.href).searchParams.get('method')==='wallet')document.querySelector('[name=method][value=wallet]').checked=true;
void init();setInterval(()=>{if(current)void refresh();},5000);
