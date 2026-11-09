import {SlashCommandBuilder} from 'discord.js';
import {commandData} from './command-data.mjs';
export const commands=commandData.map(data=>new SlashCommandBuilder().setName(data.name).setDescription(data.description).setDefaultMemberPermissions(BigInt(data.default_member_permissions))
 .addStringOption(option=>option.setName(data.options[0].name).setDescription(data.options[0].description).setRequired(true).addChoices(...data.options[0].choices)));
export const command=commands[0];
