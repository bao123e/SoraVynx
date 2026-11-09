import {SlashCommandBuilder,PermissionFlagsBits} from 'discord.js';
export const command=new SlashCommandBuilder().setName('keycreate').setDescription('Owner: create a short access SoraVynx key after confirming payment').setDefaultMemberPermissions(PermissionFlagsBits.Administrator)
 .addStringOption(o=>o.setName('plan').setDescription('License lifetime').setRequired(true).addChoices({name:'7 ngày',value:'week'},{name:'30 ngày',value:'month'},{name:'365 ngày',value:'year'},{name:'Vĩnh viễn',value:'lifetime'}));
