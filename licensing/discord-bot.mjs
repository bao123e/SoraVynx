/* Manual owner command first. This process is NOT evidence of bank payment. */
import {Client,GatewayIntentBits,MessageFlags} from 'discord.js';
import {readFileSync} from 'node:fs';
import {issueLicense} from './issuer.mjs';
for(const name of ['DISCORD_BOT_TOKEN','DISCORD_GUILD_ID','DISCORD_OWNER_ID','LICENSE_PRIVATE_KEY_FILE'])if(!process.env[name])throw new Error('Missing '+name);
const pem=readFileSync(process.env.LICENSE_PRIVATE_KEY_FILE,'utf8');
const client=new Client({intents:[GatewayIntentBits.Guilds]});
client.once('clientReady',()=>console.log('SoraVynx owner bot ready. Payment confirmation is manual.'));
client.on('interactionCreate',async interaction=>{
 if(!interaction.isChatInputCommand()||interaction.commandName!=='keycreate')return;
 if(interaction.guildId!==process.env.DISCORD_GUILD_ID||interaction.user.id!==process.env.DISCORD_OWNER_ID){await interaction.reply({content:'Only the configured tool owner can issue keys.',flags:MessageFlags.Ephemeral});return;}
 try{
  const key=issueLicense(pem,interaction.options.getString('machine',true).trim().toLowerCase(),interaction.options.getString('plan',true));
  await interaction.reply({content:'Key signed. Lifetime starts now; send privately to the correct customer.\n```text\n'+key+'\n```',flags:MessageFlags.Ephemeral});
 }catch{await interaction.reply({content:'Cannot issue key. Check the machine code and plan.',flags:MessageFlags.Ephemeral});}
});
await client.login(process.env.DISCORD_BOT_TOKEN);
