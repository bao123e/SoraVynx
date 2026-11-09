/* Manual owner command first. This process is NOT evidence of bank payment. */
import {Client,GatewayIntentBits,MessageFlags} from 'discord.js';
import {readFileSync} from 'node:fs';
import {createShortLicense,publishShortLicense} from './issuer.mjs';
for(const name of ['DISCORD_BOT_TOKEN','DISCORD_GUILD_ID','DISCORD_OWNER_ID','LICENSE_PRIVATE_KEY_FILE','GITHUB_TOKEN'])if(!process.env[name])throw new Error('Missing '+name);
const pem=readFileSync(process.env.LICENSE_PRIVATE_KEY_FILE,'utf8');
const client=new Client({intents:[GatewayIntentBits.Guilds]});
client.once('clientReady',()=>console.log('SoraVynx owner bot ready. Payment confirmation is manual.'));
client.on('interactionCreate',async interaction=>{
 if(!interaction.isChatInputCommand()||interaction.commandName!=='keycreate')return;
 if(interaction.guildId!==process.env.DISCORD_GUILD_ID||interaction.user.id!==process.env.DISCORD_OWNER_ID){await interaction.reply({content:'Only the configured tool owner can issue keys.',flags:MessageFlags.Ephemeral});return;}
 try{
  await interaction.deferReply({flags:MessageFlags.Ephemeral});
  const record=createShortLicense(pem,interaction.options.getString('plan',true));
  await publishShortLicense(record);
  await interaction.editReply({content:'Key: `'+record.key+'` — wait for website deployment before use. Lifetime starts now. Send privately to the customer.'});
 }catch{if(interaction.deferred)await interaction.editReply({content:'Cannot register the key. Check the website publishing configuration.'});else await interaction.reply({content:'Cannot create key.',flags:MessageFlags.Ephemeral});}
});
await client.login(process.env.DISCORD_BOT_TOKEN);
