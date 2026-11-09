export function canUseCommand(command,guildId,userId,{guild,owner}){
 if(guildId!==guild||!['buy','order','keycreate'].includes(command))return false;
 return command==='buy'||userId===owner;
}
