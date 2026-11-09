import {EmbedBuilder,ActionRowBuilder,ButtonBuilder,ButtonStyle} from 'discord.js';
const labels={week:'7 ngày',month:'30 ngày',year:'365 ngày',lifetime:'Vĩnh viễn'};
const png=Buffer.from([137,80,78,71,13,10,26,10]);
async function qrImage(url,fetcher){
 const target=new URL(url);
 if(target.origin!=='https://qr.sepay.vn'||target.pathname!=='/img')throw new Error('unsupported_qr_host');
 const response=await fetcher(target.href,{redirect:'error',signal:AbortSignal.timeout(6000)});
 if(!response.ok||!/^image\/png(?:;|$)/i.test(response.headers.get('content-type')||''))throw new Error('qr_unavailable');
 const reader=response.body.getReader(),chunks=[];let size=0;
 try{
  for(;;){const {done,value}=await reader.read();if(done)break;size+=value.length;if(size>1024*1024)throw new Error('qr_too_large');chunks.push(Buffer.from(value));}
 }finally{await reader.cancel().catch(()=>{});}
 const bytes=Buffer.concat(chunks);
 if(bytes.length<8||!bytes.subarray(0,8).equals(png))throw new Error('qr_not_png');
 return bytes;
}
export function orderTextReply(order){
 return {content:'Đơn '+order.code+' — '+new Intl.NumberFormat('vi-VN').format(order.amount)+'đ\nDiscord chưa hiển thị được ảnh QR. Mở đơn để quét QR và nhận key: '+order.url+'\nGửi riêng link cho khách; link có quyền xem key sau thanh toán.',embeds:[],components:[],attachments:[],allowedMentions:{parse:[]}};
}
export async function orderQRReply(order,{fetcher=fetch}={}){
 const qr=new URL(order.qr),checkout=new URL(order.url);
 if(qr.origin!=='https://qr.sepay.vn'||qr.pathname!=='/img'||checkout.protocol!=='https:'||!/^SVX[A-F0-9]{16}$/.test(order.code)||!Number.isSafeInteger(order.amount)||order.amount<=0)throw new Error('invalid_order');
 const embed=new EmbedBuilder().setColor(0x5eead4).setTitle('SoraVynx · Thanh toán BIDV')
  .setDescription('Quét QR bên dưới. Giữ đúng số tiền và nội dung chuyển khoản.')
  .addFields({name:'Gói key',value:labels[order.plan]||order.plan,inline:true},{name:'Số tiền',value:new Intl.NumberFormat('vi-VN').format(order.amount)+'đ',inline:true},{name:'Nội dung chuyển khoản',value:'`'+order.code+'`'})
  .setFooter({text:'Đơn có hiệu lực 30 phút · SePay tự xác nhận tiền vào'});
 let files=[];
 try{const image=await qrImage(qr.href,fetcher);files=[{attachment:image,name:'soravynx-qr.png'}];embed.setImage('attachment://soravynx-qr.png');}
 catch{embed.setImage(qr.href);}
 const row=new ActionRowBuilder().addComponents(new ButtonBuilder().setStyle(ButtonStyle.Link).setLabel('Mở đơn / nhận key').setURL(checkout.href));
 return {content:'Quét QR để thanh toán. Khi SePay xác nhận tiền và đăng ký xong, bot tự hiện key trong tin này. Nếu bot khởi động lại hoặc tin quá lâu, key được gửi qua DM riêng. Gửi riêng link đơn cho khách; không đăng công khai.',embeds:[embed],components:[row],files,allowedMentions:{parse:[]}};
}
