import {REST,Routes} from 'discord.js';
import {commandData} from './command-data.mjs';
for(const name of ['DISCORD_BOT_TOKEN','DISCORD_APPLICATION_ID','DISCORD_GUILD_ID','DISCORD_OWNER_ID'])if(!process.env[name])throw new Error('Missing '+name);
const rest=new REST({version:'10'}).setToken(process.env.DISCORD_BOT_TOKEN);
for(const body of commandData)await rest.post(Routes.applicationGuildCommands(process.env.DISCORD_APPLICATION_ID,process.env.DISCORD_GUILD_ID),{body});
console.log('Owner key command registered for the configured server.');
