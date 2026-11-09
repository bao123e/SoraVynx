import {SlashCommandBuilder} from 'discord.js';
import {commandData} from './command-data.mjs';
export const commands=commandData.map(data=>{
 const builder=new SlashCommandBuilder().setName(data.name).setDescription(data.description).addStringOption(option=>option.setName(data.options[0].name).setDescription(data.options[0].description).setRequired(true).addChoices(...data.options[0].choices));
 if(data.default_member_permissions)builder.setDefaultMemberPermissions(BigInt(data.default_member_permissions));
 return builder;
});
export const command=commands.find(c=>c.name==='keycreate');
