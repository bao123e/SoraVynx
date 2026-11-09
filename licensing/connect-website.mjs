import {resolve} from 'node:path';
import {fileURLToPath} from 'node:url';
export async function publishStoreConfig({publicURL,discordInvite='https://discord.gg/J3BevcpAxF',token,fetcher=fetch}){
 if(!/^https:\/\/[a-z0-9.-]+(?::\d+)?$/i.test(publicURL||''))throw new Error('Chua co PUBLIC_BASE_URL HTTPS. Mo file 7 truoc.');
 publicURL=new URL(publicURL).origin;
 if(!/^https:\/\/(discord\.gg\/[a-zA-Z0-9-]+|discord\.com\/invite\/[a-zA-Z0-9-]+)$/.test(discordInvite))throw new Error('Link moi Discord khong hop le.');
 if(!token)throw new Error('Chua co GITHUB_TOKEN.');
 const capability=await fetcher(publicURL+'/api/public/config',{redirect:'error',signal:AbortSignal.timeout(15000)});
 if(!capability.ok)throw new Error('Backend chua san sang. Cap nhat va mo lai file 5 truoc khi ket noi web.');
 const data=await capability.json();if(data.apiVersion!==1||!data.features?.includes('wallet'))throw new Error('Backend chua co phien ban tai khoan/vi.');
 if(data.publicOrigin!==publicURL)throw new Error('Backend chua nhan URL moi. Dung cua so file 5 cu bang Ctrl+C va mo lai truoc khi chay file 9.');
 const value={version:1,apiBase:publicURL,discordInvite},content=JSON.stringify(value,null,2)+'\n';
 const url='https://api.github.com/repos/bao123e/soravynx.github.io/contents/store-config.json',headers={Authorization:'Bearer '+token,Accept:'application/vnd.github+json','Content-Type':'application/json','X-GitHub-Api-Version':'2022-11-28'};
 const old=await fetcher(url+'?ref=main',{headers,redirect:'error',signal:AbortSignal.timeout(15000)});let sha;
 if(old.ok){const item=await old.json();sha=item.sha;if(Buffer.from(item.content||'','base64').toString('utf8')===content)return value;}
 else if(old.status!==404)throw new Error('GitHub HTTP '+old.status+'. Kiem tra quyen repo website.');
 const written=await fetcher(url,{method:'PUT',headers,redirect:'error',signal:AbortSignal.timeout(15000),body:JSON.stringify({message:'Connect account portal and QR checkout',branch:'main',...(sha?{sha}:{}),content:Buffer.from(content).toString('base64')})});
 if(!written.ok)throw new Error('Khong cap nhat duoc website: GitHub HTTP '+written.status);
 return value;
}
if(process.argv[1]&&resolve(process.argv[1])===fileURLToPath(import.meta.url)){
 try{await publishStoreConfig({publicURL:process.env.PUBLIC_BASE_URL,discordInvite:process.env.DISCORD_INVITE_URL||'https://discord.gg/J3BevcpAxF',token:process.env.GITHUB_TOKEN});console.log('Da ket noi soravynx.online voi cong tai khoan/vi/QR. Doi GitHub Pages cap nhat. Khi URL tunnel doi, chay lai file 9.');}
 catch(e){console.error(e.message);process.exitCode=1;}
}
