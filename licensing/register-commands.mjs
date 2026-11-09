import {REST,Routes} from 'discord.js';
import {command} from './commands.mjs';
for(const name of ['DISCORD_BOT_TOKEN','DISCORD_APPLICATION_ID','DISCORD_GUILD_ID','DISCORD_OWNER_ID'])if(!process.env[name])throw new Error('Missing '+name);
const rest=new REST({version:'10'}).setToken(process.env.DISCORD_BOT_TOKEN);
await rest.put(Routes.applicationGuildCommands(process.env.DISCORD_APPLICATION_ID,process.env.DISCORD_GUILD_ID),{body:[command.toJSON()]});
console.log('Owner key command registered for the configured server.');
