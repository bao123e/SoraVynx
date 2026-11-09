/* Manual owner command first. This process is NOT evidence of bank payment. */
import {Client,GatewayIntentBits,MessageFlags} from 'discord.js';
import {mkdirSync,writeFileSync} from 'node:fs';
import {resolve} from 'node:path';
import {directory,matchingIssuer} from './owner-config.mjs';
import {createShortLicense,publishShortLicense} from './issuer.mjs';
import {orderQRReply,orderTextReply} from './order-reply.mjs';
for(const name of ['DISCORD_BOT_TOKEN','DISCORD_GUILD_ID','DISCORD_OWNER_ID','LICENSE_PRIVATE_KEY_FILE','GITHUB_TOKEN'])if(!process.env[name])throw new Error('Missing '+name);
const pem=matchingIssuer(process.env.LICENSE_PRIVATE_KEY_FILE);
const client=new Client({intents:[GatewayIntentBits.Guilds]});
client.once('clientReady',()=>console.log('SoraVynx owner bot ready. /keycreate: manual payment check; /order: SePay webhook.'));
client.on('interactionCreate',async interaction=>{
 if(!interaction.isChatInputCommand()||!['keycreate','order'].includes(interaction.commandName))return;
 if(interaction.guildId!==process.env.DISCORD_GUILD_ID||interaction.user.id!==process.env.DISCORD_OWNER_ID){await interaction.reply({content:'Only the configured tool owner can issue keys.',flags:MessageFlags.Ephemeral});return;}
 try{
  await interaction.deferReply({flags:MessageFlags.Ephemeral});
  if(interaction.commandName==='order'){
   if(!process.env.PAYMENT_ADMIN_KEY)throw new Error('payment_not_configured');
   const response=await fetch('http://127.0.0.1:8787/admin/orders',{method:'POST',signal:AbortSignal.timeout(10000),headers:{Authorization:'Bearer '+process.env.PAYMENT_ADMIN_KEY,'Content-Type':'application/json'},body:JSON.stringify({plan:interaction.options.getString('plan',true)})});
   if(!response.ok)throw new Error('payment_not_ready');
   const order=await response.json();
   const reply=await orderQRReply(order);
   try{await interaction.editReply(reply);}catch{await interaction.editReply(orderTextReply(order));}return;
  }
  const record=createShortLicense(pem,interaction.options.getString('plan',true));
  const folder=resolve(directory,'secrets/manual-keys');mkdirSync(folder,{recursive:true,mode:0o700});
  writeFileSync(resolve(folder,record.hash+'.json'),JSON.stringify(record),{flag:'wx',mode:0o600});
  await publishShortLicense(record);
  await interaction.editReply({content:'Key: `'+record.key+'` — đã đăng ký. Đợi website cập nhật rồi gửi riêng cho khách. Hạn bắt đầu từ lúc cấp.',allowedMentions:{parse:[]}});
 }catch{const content=interaction.commandName==='order'?'Chưa tạo được đơn. Mở 5-Start-Webhook.cmd và kiểm tra PUBLIC_BASE_URL trong .env.':'Chưa đăng ký được key. Kiểm tra quyền Contents: Read and write của GitHub token. Bản sao được giữ riêng trong secrets/manual-keys.';if(interaction.deferred)await interaction.editReply({content});else await interaction.reply({content,flags:MessageFlags.Ephemeral});}
});
await client.login(process.env.DISCORD_BOT_TOKEN);
