import {createInterface} from 'node:readline/promises';
import {Writable} from 'node:stream';
export function terminalPrompt(input=process.stdin,output=process.stdout){
 let muted=false;
 const sink=new Writable({write(chunk,_encoding,done){if(!muted)output.write(chunk);done();}});
 const io=createInterface({input,output:sink,terminal:!!input.isTTY});
 return {
  async ask(label,current='',secret=false){
   const hint=current?(secret?' [Enter: giu gia tri cu]':' ['+current+']'):'';
   if(secret&&!input.isTTY)throw new Error('Mo 3-Setup-Bot.cmd truc tiep, khong chay qua pipe.');
   // Readline redraws its own prompt. An external stdout.write followed by
   // question('') is erased on Windows terminals. Supply the actual prompt.
   const answer=io.question(label+hint+': ');muted=secret;
   try{return (await answer).trim()||current;}finally{muted=false;if(secret)output.write('\n');}
  },
  close(){io.close();}
 };
}
