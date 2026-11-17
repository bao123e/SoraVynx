# SPDX-License-Identifier: AGPL-3.0-or-later
param([long]$GameWindow=0,[int]$GamePid=0,[int]$Tasks=512,[switch]$SelfTest)
$ErrorActionPreference='Stop'
$ProgressPreference='SilentlyContinue'
function Signal([string]$code){[Console]::Out.WriteLine('SV1|'+$code);[Console]::Out.Flush()}
try {
 Add-Type -AssemblyName System.Drawing
 Add-Type -AssemblyName System.Runtime.WindowsRuntime
 [Windows.Media.Ocr.OcrEngine,Windows.Foundation,ContentType=WindowsRuntime] | Out-Null
 [Windows.Globalization.Language,Windows.Globalization,ContentType=WindowsRuntime] | Out-Null
 [Windows.Graphics.Imaging.SoftwareBitmap,Windows.Foundation,ContentType=WindowsRuntime] | Out-Null
 [Windows.Storage.Streams.DataWriter,Windows.Storage.Streams,ContentType=WindowsRuntime] | Out-Null
 $engine=[Windows.Media.Ocr.OcrEngine]::TryCreateFromLanguage([Windows.Globalization.Language]::new('en-US'))
 if($null -eq $engine){Signal 'ERR_OCR_LANGUAGE';exit 2}
 $asTask=[System.WindowsRuntimeSystemExtensions].GetMethods() | Where-Object {$_.Name -eq 'AsTask' -and $_.IsGenericMethod -and $_.GetParameters().Count -eq 1 -and $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncOperation`1'} | Select-Object -First 1
 if($null -eq $asTask){Signal 'ERR_OCR_RUNTIME';exit 2}
} catch {Signal 'ERR_OCR_UNAVAILABLE';exit 2}
Add-Type -TypeDefinition @'
using System;using System.Drawing;using System.Drawing.Imaging;using System.Runtime.InteropServices;using System.Text;using System.Threading;
public static class CommissionDesktop {
 [StructLayout(LayoutKind.Sequential)] public struct Point {public int X,Y;}
 [StructLayout(LayoutKind.Sequential)] public struct Rect {public int L,T,R,B;}
 [StructLayout(LayoutKind.Sequential)] struct Mouse {public int X,Y;public uint Data,Flags,Time;public UIntPtr Extra;}
 [StructLayout(LayoutKind.Sequential)] struct Key {public ushort Vk,Scan;public uint Flags,Time;public UIntPtr Extra;}
 [StructLayout(LayoutKind.Explicit)] struct Payload {[FieldOffset(0)]public Mouse M;[FieldOffset(0)]public Key K;}
 [StructLayout(LayoutKind.Sequential)] struct Input {public uint Type;public Payload Data;}
 [DllImport("user32.dll")] static extern bool IsWindow(IntPtr h);
 [DllImport("user32.dll")] static extern bool IsIconic(IntPtr h);
 [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr h,out uint pid);
 [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern int GetClassName(IntPtr h,StringBuilder name,int cap);
 [DllImport("user32.dll")] static extern IntPtr GetForegroundWindow();
 [DllImport("user32.dll")] static extern bool GetClientRect(IntPtr h,out Rect r);
 [DllImport("user32.dll")] static extern bool ClientToScreen(IntPtr h,ref Point p);
 [DllImport("user32.dll")] static extern bool GetCursorPos(out Point p);
 [DllImport("user32.dll")] static extern bool SetCursorPos(int x,int y);
 [DllImport("user32.dll")] static extern IntPtr WindowFromPoint(Point p);
 [DllImport("user32.dll")] static extern IntPtr GetAncestor(IntPtr h,uint flag);
 [DllImport("user32.dll")] static extern short GetAsyncKeyState(int key);
 [DllImport("user32.dll")] static extern uint SendInput(uint count,Input[] input,int size);
 [DllImport("user32.dll")] static extern IntPtr SetThreadDpiAwarenessContext(IntPtr context);
 static IntPtr Window;static uint Pid;static int Width,Height;
 public static void Init(long window,int pid){Window=new IntPtr(window);Pid=(uint)pid;try{SetThreadDpiAwarenessContext(new IntPtr(-4));}catch(EntryPointNotFoundException){} View();}
 static Rect View(){
  uint pid;var name=new StringBuilder(64);Rect r;
  if(!IsWindow(Window)||IsIconic(Window)||GetWindowThreadProcessId(Window,out pid)==0||pid!=Pid||GetClassName(Window,name,64)==0||name.ToString()!="UnityWndClass"||GetForegroundWindow()!=Window||!GetClientRect(Window,out r))throw new Exception("CONTEXT");
  int w=r.R-r.L,h=r.B-r.T;if(w<320||h<180)throw new Exception("SIZE");
  if(Width!=0&&(w!=Width||h!=Height))throw new Exception("RESIZED");Width=w;Height=h;return r;
 }
 public static Bitmap PromptCrop(Bitmap source,bool invert){
  int w=Math.Max(1,(int)(source.Width*0.28)),h=Math.Max(1,(int)(source.Height*0.18));
  var output=new Bitmap(w*2,h*2,PixelFormat.Format32bppArgb);
  using(var g=Graphics.FromImage(output))using(var attributes=new ImageAttributes()){
   if(invert)attributes.SetColorMatrix(new ColorMatrix(new float[][]{new float[]{-1,0,0,0,0},new float[]{0,-1,0,0,0},new float[]{0,0,-1,0,0},new float[]{0,0,0,1,0},new float[]{1,1,1,0,1}}));
   g.DrawImage(source,new Rectangle(0,0,w*2,h*2),0,0,w,h,GraphicsUnit.Pixel,attributes);
  }return output;
 }
 public static Bitmap Region(Bitmap source,double rx,double ry,double rw,double rh,bool invert){
  int x=(int)(source.Width*rx),y=(int)(source.Height*ry),w=Math.Max(1,(int)(source.Width*rw)),h=Math.Max(1,(int)(source.Height*rh));
  var output=new Bitmap(w*3,h*3,PixelFormat.Format32bppArgb);
  using(var g=Graphics.FromImage(output))using(var attributes=new ImageAttributes()){
   if(invert)attributes.SetColorMatrix(new ColorMatrix(new float[][]{new float[]{-1,0,0,0,0},new float[]{0,-1,0,0,0},new float[]{0,0,-1,0,0},new float[]{0,0,0,1,0},new float[]{1,1,1,0,1}}));
   g.DrawImage(source,new Rectangle(0,0,w*3,h*3),x,y,w,h,GraphicsUnit.Pixel,attributes);
  }return output;
 }
 public static Bitmap HomeCrop(Bitmap source,bool invert){
  int x=(int)(source.Width*0.68),y=(int)(source.Height*0.10);
  int w=source.Width-x,h=Math.Max(1,(int)(source.Height*0.20));
  var output=new Bitmap(w*3,h*3,PixelFormat.Format32bppArgb);
  using(var g=Graphics.FromImage(output))using(var attributes=new ImageAttributes()){
   if(invert)attributes.SetColorMatrix(new ColorMatrix(new float[][]{new float[]{-1,0,0,0,0},new float[]{0,-1,0,0,0},new float[]{0,0,-1,0,0},new float[]{0,0,0,1,0},new float[]{1,1,1,0,1}}));
   g.DrawImage(source,new Rectangle(0,0,w*3,h*3),x,y,w,h,GraphicsUnit.Pixel,attributes);
  }return output;
 }
 // Numeric fallback for isolated counters that Windows OCR omits. Templates
 // are generated locally from installed Windows fonts; never from account data.
 static bool[] Normalize(bool[] source,int sw,int l,int t,int w,int h){
  var result=new bool[24*32];double scale=Math.Min(20.0/w,28.0/h);int nw=Math.Max(1,(int)Math.Round(w*scale)),nh=Math.Max(1,(int)Math.Round(h*scale));
  int ox=(24-nw)/2,oy=(32-nh)/2;
  for(int y=0;y<nh;y++)for(int x=0;x<nw;x++)result[(y+oy)*24+x+ox]=source[(t+Math.Min(h-1,(int)(y/scale)))*sw+l+Math.Min(w-1,(int)(x/scale))];return result;
 }
 static readonly System.Collections.Generic.List<bool[]> DigitTemplates=new System.Collections.Generic.List<bool[]>();
 static readonly System.Collections.Generic.List<int> TemplateDigits=new System.Collections.Generic.List<int>();
 static void MakeDigits(){
  if(DigitTemplates.Count!=0)return;
  foreach(string family in new[]{"Segoe UI","Arial","Calibri","Tahoma","Verdana"})foreach(FontStyle style in new[]{FontStyle.Regular,FontStyle.Bold})for(int digit=0;digit<10;digit++){
   using(var b=new Bitmap(100,100))using(var g=Graphics.FromImage(b))using(var f=new Font(family,64,style,GraphicsUnit.Pixel)){
    g.Clear(Color.Black);g.TextRenderingHint=System.Drawing.Text.TextRenderingHint.AntiAliasGridFit;g.DrawString(digit.ToString(),f,Brushes.White,0,0);g.Flush();
    var mask=new bool[10000];int l=100,t=100,r=0,bt=0;
    for(int y=0;y<100;y++)for(int x=0;x<100;x++)if(b.GetPixel(x,y).R>150){mask[y*100+x]=true;l=Math.Min(l,x);t=Math.Min(t,y);r=Math.Max(r,x);bt=Math.Max(bt,y);}
    if(r>=l&&bt>=t){DigitTemplates.Add(Normalize(mask,100,l,t,r-l+1,bt-t+1));TemplateDigits.Add(digit);}
   }
  }
 }
 static int Classify(bool[] sample){
  var scores=new double[10];for(int d=0;d<10;d++)scores[d]=1;
  for(int i=0;i<DigitTemplates.Count;i++){
   bool[] target=DigitTemplates[i];double best=1;
   for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){
    int mismatch=0,union=0;
    for(int y=0;y<32;y++)for(int x=0;x<24;x++){
     bool a=sample[y*24+x],b=x+dx>=0&&x+dx<24&&y+dy>=0&&y+dy<32&&target[(y+dy)*24+x+dx];if(a||b)union++;if(a!=b)mismatch++;
    }if(union>0)best=Math.Min(best,(double)mismatch/union);
   }scores[TemplateDigits[i]]=Math.Min(scores[TemplateDigits[i]],best);
  }
  int winner=0;for(int d=1;d<10;d++)if(scores[d]<scores[winner])winner=d;
  double runner=1;for(int d=0;d<10;d++)if(d!=winner)runner=Math.Min(runner,scores[d]);
  return scores[winner]<=0.30&&runner-scores[winner]>=0.035?winner:-1;
 }
 public static int Number(Bitmap source,double rx,double ry,double rw,double rh,bool bright,double minHeight,double maxHeight){
  MakeDigits();int left=(int)(rx*source.Width),top=(int)(ry*source.Height),w=(int)(rw*source.Width),h=(int)(rh*source.Height);
  var mask=new bool[w*h];var seen=new bool[w*h];var queue=new int[w*h];var components=new System.Collections.Generic.List<int[]>();
  for(int y=0;y<h;y++)for(int x=0;x<w;x++){
   Color c=source.GetPixel(left+x,top+y);mask[y*w+x]=bright?(c.R>150&&c.G>140&&(c.B<180||(c.R>190&&c.G>190&&c.B>190))):(c.R<130&&c.G<150&&c.B<220);
  }
  for(int p=0;p<mask.Length;p++)if(mask[p]&&!seen[p]){
   int head=0,tail=0,l=w,t=h,r=0,bt=0;queue[tail++]=p;seen[p]=true;
   while(head<tail){int v=queue[head++],x=v%w,y=v/w;l=Math.Min(l,x);t=Math.Min(t,y);r=Math.Max(r,x);bt=Math.Max(bt,y);
    for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){int nx=x+dx,ny=y+dy;if(nx<0||nx>=w||ny<0||ny>=h)continue;int n=ny*w+nx;if(mask[n]&&!seen[n]){seen[n]=true;queue[tail++]=n;}}
   }
   int ch=bt-t+1,cw=r-l+1;
   if(l>0&&t>0&&r<w-1&&bt<h-1&&ch>=minHeight*source.Height&&ch<=maxHeight*source.Height&&cw>=2&&cw<=ch*1.1)components.Add(new[]{l,t,cw,ch});
  }
  if(components.Count==0)return -1;int tallest=0;foreach(var c in components)tallest=Math.Max(tallest,c[3]);
  components.RemoveAll(c=>c[3]<tallest*0.80);components.Sort((a,b)=>a[0].CompareTo(b[0]));
  if(components.Count==0||components.Count>6)return -1;int result=0;
  foreach(var c in components){int digit=Classify(Normalize(mask,w,c[0],c[1],c[2],c[3]));if(digit<0)return -1;result=result*10+digit;}return result;
 }
 public static Bitmap Capture(){
  Rect r=View();Point p=new Point();if(!ClientToScreen(Window,ref p))throw new Exception("CONTEXT");
  var bitmap=new Bitmap(Width,Height,PixelFormat.Format32bppArgb);
  try{using(var g=Graphics.FromImage(bitmap)){g.CopyFromScreen(p.X,p.Y,0,0,new Size(Width,Height),CopyPixelOperation.SourceCopy);}View();return bitmap;}catch{bitmap.Dispose();throw;}
 }
 static void Emit(string code){Console.Out.WriteLine("SV1|"+code);Console.Out.Flush();}
 static bool InputEvent(bool key,bool up){var input=new Input();input.Type=key?1u:0u;if(key){input.Data.K.Vk=27;input.Data.K.Flags=up?2u:0u;}else input.Data.M.Flags=up?4u:2u;return SendInput(1,new[]{input},Marshal.SizeOf(typeof(Input)))==1;}
 public static void Act(bool escape,double x,double y){
  View();if(x<0||x>1||y<0||y>1||GetAsyncKeyState(1)<0||GetAsyncKeyState(27)<0)throw new Exception("INPUT_STATE");
  Point restore,target=new Point();bool moved=false,held=false;
  if(!GetCursorPos(out restore))throw new Exception("CURSOR");
  try{
   if(!escape){target.X=(int)Math.Round(x*(Width-1));target.Y=(int)Math.Round(y*(Height-1));if(!ClientToScreen(Window,ref target)||!SetCursorPos(target.X,target.Y))throw new Exception("CURSOR");moved=true;Thread.Sleep(80);View();if(GetAncestor(WindowFromPoint(target),2)!=Window)throw new Exception("COVERED");}
   View();Emit(escape?"HOLD_ESCAPE":"HOLD_MOUSE");held=true;
   if(!InputEvent(escape,false))throw new Exception("INPUT");Thread.Sleep(80);
   if(!InputEvent(escape,true))throw new Exception("RELEASE");held=false;Emit(escape?"UP_ESCAPE":"UP_MOUSE");Thread.Sleep(120);
  }finally{
   if(held){for(int i=0;i<3;i++)if(InputEvent(escape,true)){held=false;Emit(escape?"UP_ESCAPE":"UP_MOUSE");break;}}
   Point now;if(moved&&GetCursorPos(out now)&&now.X==target.X&&now.Y==target.Y)SetCursorPos(restore.X,restore.Y);
  }
 }
}
'@ -ReferencedAssemblies System.Drawing
function Read-Image($bitmap){
 $copy=$null;$software=$null;$writer=$null
 try {
  $max=[Windows.Media.Ocr.OcrEngine]::MaxImageDimension
  $scale=[Math]::Min(1.0,$max/[double][Math]::Max($bitmap.Width,$bitmap.Height))
  $copy=[Drawing.Bitmap]::new([int]($bitmap.Width*$scale),[int]($bitmap.Height*$scale),[Drawing.Imaging.PixelFormat]::Format32bppArgb)
  $graphics=[Drawing.Graphics]::FromImage($copy);try{$graphics.DrawImage($bitmap,0,0,$copy.Width,$copy.Height)}finally{$graphics.Dispose()}
  $rect=[Drawing.Rectangle]::new(0,0,$copy.Width,$copy.Height)
  $bits=$copy.LockBits($rect,[Drawing.Imaging.ImageLockMode]::ReadOnly,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
  try{$bytes=[byte[]]::new($copy.Width*$copy.Height*4);[Runtime.InteropServices.Marshal]::Copy($bits.Scan0,$bytes,0,$bytes.Length)}finally{$copy.UnlockBits($bits)}
  $buffer=[System.Runtime.InteropServices.WindowsRuntime.WindowsRuntimeBufferExtensions]::AsBuffer($bytes)
  $software=[Windows.Graphics.Imaging.SoftwareBitmap]::new([Windows.Graphics.Imaging.BitmapPixelFormat]::Bgra8,$copy.Width,$copy.Height)
  $software.CopyFromBuffer($buffer)
  $task=$asTask.MakeGenericMethod([Windows.Media.Ocr.OcrResult]).Invoke($null,@($engine.RecognizeAsync($software)))
  if(!$task.Wait(8000)){throw 'OCR_TIMEOUT'}
  $words=@();$lines=@(foreach($line in $task.Result.Lines){
   $left=1e9;$top=1e9;$right=0;$bottom=0;$lineWords=@()
   foreach($word in $line.Words){$b=$word.BoundingRect;$wordBox=[pscustomobject]@{Text=$word.Text;X=$b.X/$copy.Width;Y=$b.Y/$copy.Height;W=$b.Width/$copy.Width;H=$b.Height/$copy.Height};$words+=$wordBox;$lineWords+=$wordBox;$left=[Math]::Min($left,$b.X);$top=[Math]::Min($top,$b.Y);$right=[Math]::Max($right,$b.X+$b.Width);$bottom=[Math]::Max($bottom,$b.Y+$b.Height)}
   if($right -gt $left){[pscustomobject]@{Text=$line.Text;Words=$lineWords;X=$left/$copy.Width;Y=$top/$copy.Height;W=($right-$left)/$copy.Width;H=($bottom-$top)/$copy.Height}}
  })
  $result=[pscustomobject]@{Lines=$lines;Words=$words;Bitmap=$copy};$copy=$null;return $result
 }finally{if($software){$software.Dispose()};if($writer){$writer.Dispose()};if($copy){$copy.Dispose()}}
}
function Normal([string]$text){return [regex]::Replace($text.ToLowerInvariant(),'[^a-z0-9]','')}
function Cyan-Button($bitmap,$line){
 $hits=0
 foreach($point in @(@(($line.X-.008),($line.Y+$line.H/2)),@(($line.X+$line.W+.008),($line.Y+$line.H/2)),@(($line.X+$line.W/2),($line.Y-.01)),@(($line.X+$line.W/2),($line.Y+$line.H+.01)))){
  $x=[int]($point[0]*$bitmap.Width);$y=[int]($point[1]*$bitmap.Height)
  if($x -ge 0 -and $x -lt $bitmap.Width -and $y -ge 0 -and $y -lt $bitmap.Height){$c=$bitmap.GetPixel($x,$y);if($c.G -gt 130 -and $c.B -gt 120 -and $c.G-$c.R -gt 30 -and $c.B-$c.R -gt 20){$hits++}}
 }
 return $hits -ge 2
}
# Only printed labels and fixed UI controls from the owner's reference layout
# are used. No private game objects, item names or account text leave this process.
function Label($lines,[string]$name,[double]$minX,[double]$maxX,[double]$minY,[double]$maxY){
 $matchesFound=@()
 foreach($line in $lines){
  $candidates=@()
  if((Normal $line.Text) -eq $name){$candidates=@($line)}
  elseif($line.Words){
   $parts=@($line.Words)
   for($start=0;$start -lt $parts.Count;$start++){
    $joined=''
    for($end=$start;$end -lt $parts.Count;$end++){
     $joined+=(Normal $parts[$end].Text)
     if($joined -eq $name){
      $left=$parts[$start].X;$top=1.0;$right=0.0;$bottom=0.0
      for($i=$start;$i -le $end;$i++){$top=[Math]::Min($top,$parts[$i].Y);$right=[Math]::Max($right,$parts[$i].X+$parts[$i].W);$bottom=[Math]::Max($bottom,$parts[$i].Y+$parts[$i].H)}
      $candidates+=[pscustomobject]@{Text=$name;X=$left;Y=$top;W=$right-$left;H=$bottom-$top};break
     }
     if(!$name.StartsWith($joined)){break}
    }
   }
  }
  foreach($c in $candidates){if($c.X -ge $minX -and $c.X -lt $maxX -and $c.Y -ge $minY -and $c.Y -lt $maxY){$matchesFound+=$c}}
 }
 return $matchesFound
}
function Low-Cards($words){
 $cards=@()
 foreach($word in $words){
  if($word.X -lt .04 -or $word.X -gt .94 -or $word.Y -lt .30 -or $word.Y -gt .88){continue}
  $level=0
  if($word.Text -match '^([Ll][Vv])[. ]?\s*([0-9]{1,3})$'){$level=[int]$Matches[2]}
  elseif($word.Text -match '^[Ll][Vv][.]?$'){
   $numbers=@($words | Where-Object {$_.Text -match '^\d{1,3}$' -and [Math]::Abs($_.Y-$word.Y) -lt .02 -and $_.X -ge $word.X+$word.W-.005 -and $_.X -lt $word.X+$word.W+.035})
   if($numbers.Count -eq 1){$level=[int]$numbers[0].Text}
  }
  if($level -ge 1 -and $level -le 10){$cards+=[pscustomobject]@{Level=$level;X=$word.X+$word.W/2;Y=$word.Y-.08}}
 }
 return @($cards | Sort-Object Level,Y,X)
}
function Visible-Level($words){
 $values=@($words | Where-Object {$_.Text -match '^\d{1,3}(?:/\d{1,3})?$' -and $_.X -gt .70 -and $_.X -lt .85 -and $_.Y -gt .13 -and $_.Y -lt .30 -and $_.H -gt .032})
 if($values.Count -eq 1){return [int]($values[0].Text.Split('/')[0])}
 return 0
}
function Upgrade-Observation($lines,$words,$bitmap){
 $kind='';$page=''
 foreach($entry in @(@('disclist','DISC','LIST'),@('trekkerlist','TREKKER','LIST'),@('trekkerdetails','TREKKER','DETAILS'),@('trekkertraining','TREKKER','EDIT'),@('disc','DISC','DETAILS'))){
  $header=@(Label $lines $entry[0] 0 .30 0 .12)
  if($header.Count -eq 1){$kind=$entry[1];$page=$entry[2];break}
 }
 if(!$kind){return $null}
 if($page -eq 'LIST'){
  $cards=@(Low-Cards $words);return [pscustomobject]@{State=($kind+'_LIST');X=0.0;Y=0.0;Cards=$cards}
 }
 $one=@(Label $lines 'upgrade1level' .62 .85 .60 .78)
 $max=@(Label $lines 'upgradetomax' .78 .98 .60 .78)
 $confirm=@(Label $lines 'upgrade' .65 .95 .68 .88 | Where-Object {Cyan-Button $bitmap $_})
 $level=Visible-Level $words
 if($one.Count -eq 1 -and $max.Count -eq 1 -and $confirm.Count -eq 1){
  $minY=.68;$maxY=.75;if($kind -eq 'TREKKER'){$minY=.75;$maxY=.81}
  $prices=@($words | Where-Object {$_.Text -match '^\d[\d,]*$' -and $_.X -gt .82 -and $_.X -lt .90 -and $_.Y -gt $minY -and $_.Y -lt $maxY})
  $cost=-1;if($prices.Count -eq 1){$cost=[int]($prices[0].Text.Replace(',',''))}
  return [pscustomobject]@{State=($kind+'_EDIT');X=0.0;Y=0.0;Level=$level;Cost=$cost;OneX=$one[0].X+$one[0].W/2;OneY=$one[0].Y+$one[0].H/2;CommitX=$confirm[0].X+$confirm[0].W/2;CommitY=$confirm[0].Y+$confirm[0].H/2}
 }
 if($kind -eq 'DISC'){
  $tab=@(Label $lines 'upgrade' .34 .52 .88 1)
  $skill=@(Label $lines 'melodyskill' .65 .9 .15 .35)
  if($tab.Count -eq 1 -and $skill.Count -eq 1){return [pscustomobject]@{State='DISC_DETAILS';X=$tab[0].X+$tab[0].W/2;Y=$tab[0].Y+$tab[0].H/2;Level=$level}}
 }elseif($page -eq 'DETAILS' -and $confirm.Count -eq 1){
  return [pscustomobject]@{State='TREKKER_DETAILS';X=$confirm[0].X+$confirm[0].W/2;Y=$confirm[0].Y+$confirm[0].H/2;Level=$level}
 }
 return $null
}
function Home-Targets($observation,$words){
 if($observation.State -ne 'HOME'){return $observation}
 foreach($name in @('Disc','Trekker')){
  $buttons=@(Label $words $name.ToLowerInvariant() .72 .93 .65 .88)
  $x=0.0;$y=0.0;if($buttons.Count -eq 1){$x=$buttons[0].X+$buttons[0].W/2;$y=$buttons[0].Y+$buttons[0].H/2}
  $observation | Add-Member -NotePropertyName ($name+'X') -NotePropertyValue $x -Force
  $observation | Add-Member -NotePropertyName ($name+'Y') -NotePropertyValue $y -Force
 }
 return $observation
}
function Observe($lines,$bitmap,[string]$promptTokens='', $homeWords=@()){
 $top=@($lines | Where-Object {$_.X -lt .25 -and $_.Y -lt .18} | ForEach-Object {Normal $_.Text}) -join ''
 $top+=$promptTokens
 $bottom=@($lines | Where-Object {$_.X -gt .65 -and $_.Y -gt .8} | ForEach-Object {Normal $_.Text}) -join ''
 if($top.Contains('esc') -and $top.Contains('skip') -and $bottom.Contains('space') -and $bottom.Contains('next')){return [pscustomobject]@{State='STORY';X=0.0;Y=0.0}}
 $items=@($lines | Where-Object {(Normal $_.Text).Contains('itemsobtained') -and $_.Y -lt .5})
 $continue=@($lines | Where-Object {(Normal $_.Text).Contains('selectanywheretocontinue') -and $_.Y -gt .6})
 if($items.Count -gt 0 -and $continue.Count -eq 1){$p=$continue[0];return [pscustomobject]@{State='ITEMS';X=$p.X+$p.W/2;Y=$p.Y+$p.H/2}}
 $complete=@($lines | Where-Object {(Normal $_.Text).Contains('commissioncomplete') -and $_.Y -lt .3})
 $again=@($lines | Where-Object {(Normal $_.Text) -eq 'dispatchagain' -and $_.Y -gt .6 -and $_.X -gt .4})
 if($complete.Count -gt 0 -and $again.Count -eq 1 -and (Cyan-Button $bitmap $again[0])){$p=$again[0];return [pscustomobject]@{State='RESULT';X=$p.X+$p.W/2;Y=$p.Y+$p.H/2}}
 $header=@($lines | Where-Object {(Normal $_.Text) -eq 'commission' -and $_.X -lt .4 -and $_.Y -lt .15})
 $claim=@($lines | Where-Object {(Normal $_.Text) -eq 'claimall' -and $_.X -gt .65 -and $_.Y -gt .6})
 if($header.Count -eq 1 -and $claim.Count -eq 1){$p=$claim[0];$state='NO_REWARDS';if(Cyan-Button $bitmap $p){$state='CLAIM'};return [pscustomobject]@{State=$state;X=$p.X+$p.W/2;Y=$p.Y+$p.H/2}}
 $upgrade=Upgrade-Observation $lines $homeWords $bitmap
 if($upgrade){return $upgrade}
 $commissionHome=@($homeWords | Where-Object {(Normal $_.Text) -eq 'commission' -and $_.X -gt .7 -and $_.Y -lt .3})
 if($commissionHome.Count -eq 1){$p=$commissionHome[0];return [pscustomobject]@{State='HOME';X=$p.X+$p.W/2;Y=$p.Y+$p.H/2}}
 return [pscustomobject]@{State='UNKNOWN';X=0.0;Y=0.0}
}
function Read-Observation($bitmap){
 $read=Read-Image $bitmap
 try {
  $tokens=''
  $top=@($read.Lines | Where-Object {$_.X -lt .25 -and $_.Y -lt .18} | ForEach-Object {Normal $_.Text}) -join ''
  $bottom=@($read.Lines | Where-Object {$_.X -gt .65 -and $_.Y -gt .8} | ForEach-Object {Normal $_.Text}) -join ''
  if($bottom.Contains('space') -and $bottom.Contains('next') -and !($top.Contains('esc') -and $top.Contains('skip'))){
   foreach($invert in @($false,$true)){
    $crop=[CommissionDesktop]::PromptCrop($bitmap,$invert);$detail=$null
    try{$detail=Read-Image $crop;$tokens+=(@($detail.Lines | ForEach-Object {Normal $_.Text}) -join '')}finally{$crop.Dispose();if($detail){$detail.Bitmap.Dispose()}}
    if(($top+$tokens).Contains('esc') -and ($top+$tokens).Contains('skip')){break}
   }
  }
  $observation=Observe $read.Lines $read.Bitmap $tokens $read.Words
  if($observation.State -in @('DISC_DETAILS','TREKKER_DETAILS','DISC_EDIT','TREKKER_EDIT')){
   if($observation.Level -eq 0){
    foreach($invert in @($false,$true)){
     $crop=[CommissionDesktop]::Region($bitmap,.68,.12,.24,.20,$invert);$detail=$null
     try{
      $detail=Read-Image $crop
      $mapped=@(foreach($word in $detail.Words){[pscustomobject]@{Text=$word.Text;X=.68+$word.X*.24;Y=.12+$word.Y*.20;W=$word.W*.24;H=$word.H*.20}})
      $observation.Level=Visible-Level $mapped
      if($observation.Level -gt 0){break}
     }finally{$crop.Dispose();if($detail){$detail.Bitmap.Dispose()}}
    }
   }
  }
  if($observation.State -in @('DISC_DETAILS','TREKKER_DETAILS','DISC_EDIT','TREKKER_EDIT')){
   if(!$observation.Level){$n=[CommissionDesktop]::Number($bitmap,.72,.12,.14,.20,$true,.032,.10);if($n -gt 0){$observation.Level=$n}}
   if($observation.State -in @('DISC_EDIT','TREKKER_EDIT') -and $observation.Cost -lt 0){
    $priceY=.68;if($observation.State -eq 'TREKKER_EDIT'){$priceY=.75}
    $values=@()
    foreach($bright in @($true,$false)){$n=[CommissionDesktop]::Number($bitmap,.82,$priceY,.08,.08,$bright,.012,.045);if($n -ge 0){$values+=$n}}
    $values=@($values | Select-Object -Unique);if($values.Count -eq 1){$observation.Cost=$values[0]}
   }
  }
  if($observation.State -in @('DISC_LIST','TREKKER_LIST') -and $observation.Cards.Count -eq 0){
   foreach($invert in @($false,$true)){
    $crop=[CommissionDesktop]::Region($bitmap,.04,.30,.91,.58,$invert);$detail=$null
    try{
     $detail=Read-Image $crop
     $mapped=@(foreach($word in $detail.Words){[pscustomobject]@{Text=$word.Text;X=.04+$word.X*.91;Y=.30+$word.Y*.58;W=$word.W*.91;H=$word.H*.58}})
     $observation.Cards=@(Low-Cards $mapped)
     if($observation.Cards.Count -gt 0){break}
    }finally{$crop.Dispose();if($detail){$detail.Bitmap.Dispose()}}
   }
  }
  if($observation.State -ne 'UNKNOWN'){return Home-Targets $observation $read.Words}
  # WinRT often merges neighboring home labels into one line. Use individual
  # word boxes and retry only the icon strip at higher scale in both polarities.
  foreach($invert in @($false,$true)){
   $crop=[CommissionDesktop]::HomeCrop($bitmap,$invert);$detail=$null
   try{
    $detail=Read-Image $crop
    $mapped=@(foreach($word in $detail.Words){[pscustomobject]@{Text=$word.Text;X=.68+$word.X*.32;Y=.10+$word.Y*.20;W=$word.W*.32;H=$word.H*.20}})
    $observation=Observe $read.Lines $read.Bitmap $tokens $mapped
    if($observation.State -eq 'HOME'){return Home-Targets $observation $read.Words}
   }finally{$crop.Dispose();if($detail){$detail.Bitmap.Dispose()}}
  }
  return $observation
 }finally{$read.Bitmap.Dispose()}
}
function New-Session([int]$selectedTasks=512) {return @{DiscEnabled=($selectedTasks -band 4) -ne 0;TrekkerEnabled=($selectedTasks -band 8) -ne 0;DiscDone=$false;TrekkerDone=$false;UpgradeKind='';UpgradePhase='';UpgradeAt=0;BeforeLevel=0;TargetLevel=0;UpgradeCommits=0;TargetX=0.0;TargetY=0.0;HomeEnabled=($selectedTasks -band 64) -ne 0;DispatchEnabled=($selectedTasks -band 512) -ne 0;Interact=0;Entry=0;Claim=0;Skip=0;Items=0;Again=0;AgainAt=0;StableSince=-1;StableState='';Last=0;Unknown=0;Actions=0;Commission=$false}}
function Next-Upgrade($session,$observation,[long]$now){
 $kind=$session.UpgradeKind
 if(!$kind){
  if($session.DiscEnabled -and !$session.DiscDone){$kind='DISC'}
  elseif($session.TrekkerEnabled -and !$session.TrekkerDone){$kind='TREKKER'}
  else{return ''}
  if($observation.State -eq 'UNKNOWN'){$session.Unknown++;if($session.Unknown -ge 8){return 'ERR_UNKNOWN_SCREEN'};return 'WAIT'}
  if($observation.State -ne 'HOME'){return 'ERR_HOME_REQUIRED'}
  $prefix='Disc';if($kind -eq 'TREKKER'){$prefix='Trekker'}
  $x=$observation.($prefix+'X');$y=$observation.($prefix+'Y')
  if(!$x -or !$y){return 'ERR_UPGRADE_ENTRY_NOT_FOUND'}
  $session.UpgradeKind=$kind;$session.UpgradePhase='LIST';$session.UpgradeAt=$now
  $session.TargetX=$x;$session.TargetY=$y;return ('ENTER_'+$kind)
 }
 $phase=$session.UpgradePhase
 if($phase -eq 'RETURN' -and $observation.State -eq 'HOME'){
  $session.UpgradeKind='';$session.UpgradePhase='';$session.Unknown=0;return 'WAIT'
 }
 if($now-$session.Last -lt 900){return 'WAIT'}
 if($phase -eq 'LIST' -and $observation.State -eq ($kind+'_LIST')){
  # If tiny card levels are omitted by OCR, open the first reference card
  # for inspection only. Its large detail level must pass before any spend.
  $p=[pscustomobject]@{X=.14;Y=.31}
  if($observation.Cards.Count -gt 0){$p=$observation.Cards[0]}
  $session.TargetX=$p.X;$session.TargetY=$p.Y
  $session.UpgradePhase='DETAILS';$session.UpgradeAt=$now;return 'SELECT_LOW'
 }
 if($phase -eq 'DETAILS' -and $observation.State -eq ($kind+'_DETAILS')){
  if($observation.Level -lt 1){return 'ERR_UPGRADE_LEVEL_UNREADABLE'}
  if($observation.Level -gt 10){return 'ERR_LOW_LEVEL_NOT_FOUND'}
  $session.BeforeLevel=$observation.Level;$session.TargetX=$observation.X;$session.TargetY=$observation.Y
  $session.UpgradePhase='EDIT';$session.UpgradeAt=$now;return 'OPEN_UPGRADE'
 }
 if($phase -eq 'EDIT' -and $observation.State -eq ($kind+'_EDIT')){
  if($observation.Cost -ne 0 -or $observation.Level -ne $session.BeforeLevel){return 'ERR_UPGRADE_EXISTING_SELECTION'}
  $session.TargetX=$observation.OneX;$session.TargetY=$observation.OneY
  $session.UpgradePhase='PREPARED';$session.UpgradeAt=$now;return 'PREPARE_LEVEL'
 }
 if($phase -eq 'PREPARED' -and $observation.State -eq ($kind+'_EDIT')){
  if($observation.Level -gt $session.BeforeLevel -and $observation.Cost -gt 0){
   $session.TargetLevel=$observation.Level;$session.UpgradeCommits++
   $session.TargetX=$observation.CommitX;$session.TargetY=$observation.CommitY
   $session.UpgradePhase='VERIFY';$session.UpgradeAt=$now;return 'COMMIT_UPGRADE'
  }
 }
 if($phase -eq 'VERIFY' -and $observation.State -eq ($kind+'_EDIT')){
  # Selection must clear AND the new level remain visible. A preview alone
  # never proves the spend succeeded. Never send the spend a second time.
  if($observation.Cost -eq 0 -and $observation.Level -ge $session.TargetLevel){
   if($kind -eq 'DISC'){$session.DiscDone=$true}else{$session.TrekkerDone=$true}
   $session.UpgradePhase='RETURN';$session.UpgradeAt=$now
   $session.TargetX=.295;$session.TargetY=.061;return 'RETURN_HOME'
  }
 }
 if($observation.State -eq 'UNKNOWN'){$session.Unknown++;if($session.Unknown -ge 8){return 'ERR_UNKNOWN_SCREEN'}}else{$session.Unknown=0}
 if($now-$session.UpgradeAt -ge 10000){
  if($phase -eq 'PREPARED'){return 'ERR_UPGRADE_NOT_PREPARED'}
  if($phase -eq 'VERIFY'){return 'ERR_UPGRADE_NOT_CONFIRMED'}
  return 'ERR_UPGRADE_SCREEN_NOT_CHANGED'
 }
 return 'WAIT'
}
function Next-Action($session,$observation,[long]$now){
 if(!$session.HomeEnabled -and !$session.DispatchEnabled -and !$session.DiscEnabled -and !$session.TrekkerEnabled){return 'ERR_NO_TASKS'}
 if($session.Actions -ge 32){return 'ERR_ACTION_LIMIT'}
 if((($session.HomeEnabled -and !$session.Interact) -or (($session.DiscEnabled -or $session.TrekkerEnabled) -and !$session.Actions)) -and $observation.State -ne 'HOME' -and $observation.State -ne 'UNKNOWN'){return 'ERR_HOME_REQUIRED'}
 # Home touch runs first. Upgrades then return home before Commission starts.
 if(!($observation.State -eq 'HOME' -and $session.HomeEnabled -and !$session.Interact)){
  if($session.Interact -gt 0 -and $session.Actions -eq 1 -and $now-$session.Last -lt 1600){return 'WAIT'}
  $upgradeAction=Next-Upgrade $session $observation $now
  if($upgradeAction){
   if($upgradeAction -ne 'WAIT' -and !$upgradeAction.StartsWith('ERR_')){$session.Last=$now;$session.Actions++;$session.Unknown=0}
   return $upgradeAction
  }
 }
 # Dispatch Again can be followed by delayed dialogue and reward overlays.
 # Finish only on a recognized Commission page after an observation window,
 # never merely because the dispatch input was sent or its modal persists.
 if($session.Again -gt 0){
  if($observation.State -in @('CLAIM','NO_REWARDS')){
   $session.Unknown=0
   if($session.StableState -ne $observation.State){$session.StableSince=$now;$session.StableState=$observation.State}
   if($now-$session.AgainAt -ge 8000 -and $now-$session.StableSince -ge 2500){return 'DONE'}
   return 'WAIT'
  }
  $session.StableSince=-1;$session.StableState=''
  if($observation.State -eq 'RESULT'){
   $session.Unknown=0
   if($now-$session.Last -lt 8000){return 'WAIT'}
   return 'ERR_DISPATCH_NOT_CHANGED'
  }
  if($observation.State -eq 'HOME'){return 'ERR_LEFT_COMMISSION'}
 }
 switch($observation.State){
  'HOME' {
   if($session.HomeEnabled -and !$session.Interact){$session.Interact++;$action='INTERACT'}
   else {
    if($session.Interact -gt 0 -and $now-$session.Last -lt 1600){return 'WAIT'}
    if(!$session.DispatchEnabled){return 'DONE'}
    if($session.Entry -gt 0){if($now-$session.Last -lt 3000){return 'WAIT'};return 'ERR_ENTRY_NOT_CHANGED'}
    $session.Entry++;$action='OPEN'
   }
  }
  'CLAIM' {$session.Commission=$true;if($now-$session.Last -lt 1500){return 'WAIT'};if($session.Claim -ge 2){return 'ERR_CLAIM_NOT_CHANGED'};$session.Claim++;$action='CLAIM'}
  'STORY' {if(!$session.Commission){return 'ERR_NOT_COMMISSION'};if($now-$session.Last -lt 800){return 'WAIT'};if($session.Skip -ge 4){return 'ERR_STORY_LIMIT'};$session.Skip++;$action='ESC'}
  'ITEMS' {if(!$session.Commission){return 'ERR_NOT_COMMISSION'};if($now-$session.Last -lt 800){return 'WAIT'};if($session.Items -ge 4){return 'ERR_ITEMS_LIMIT'};$session.Items++;$action='CONTINUE'}
  'RESULT' {$session.Commission=$true;if($now-$session.Last -lt 800){return 'WAIT'};$session.Again++;$session.AgainAt=$now;$action='AGAIN'}
  'NO_REWARDS' {if($session.Claim -gt 0){if($now-$session.Last -lt 4000){return 'WAIT'};return 'ERR_REWARD_RESULT_NOT_RECOGNIZED'};return 'NO_REWARDS'}
  default {$session.Unknown++;if($session.Unknown -ge 8){return 'ERR_UNKNOWN_SCREEN'};return 'WAIT'}
 }
 $session.Unknown=0;$session.Last=$now;$session.Actions++;return $action
}
if($SelfTest){
 function Check($condition,[string]$message){if(!$condition){throw $message}}
 $bitmap=[Drawing.Bitmap]::new(1280,720);$g=[Drawing.Graphics]::FromImage($bitmap)
 try{
  $g.Clear([Drawing.Color]::FromArgb(25,35,48));$font=[Drawing.Font]::new('Segoe UI',32,[Drawing.FontStyle]::Regular,[Drawing.GraphicsUnit]::Pixel)
  $g.DrawString('Commission Complete!',$font,[Drawing.Brushes]::White,320,70)
  $g.FillRectangle([Drawing.Brushes]::Turquoise,660,550,390,90);$g.DrawString('Dispatch Again',$font,[Drawing.Brushes]::White,675,565)
  $o=Read-Observation $bitmap;Check ($o.State -eq 'RESULT') ('Real OCR recognition failed: '+$o.State)
  foreach($kind in @('STORY','ITEMS','CLAIM','HOME')){
   $g.Clear([Drawing.Color]::FromArgb(25,35,48))
   switch($kind){
    'STORY' {$g.FillRectangle([Drawing.Brushes]::White,25,25,75,45);$small=[Drawing.Font]::new('Segoe UI',22,[Drawing.FontStyle]::Regular,[Drawing.GraphicsUnit]::Pixel);try{$g.DrawString('Esc',$small,[Drawing.Brushes]::DarkBlue,30,29);$g.DrawString('Skip',$font,[Drawing.Brushes]::White,110,23);$g.FillRectangle([Drawing.Brushes]::WhiteSmoke,200,490,850,160);$g.DrawString('Minova',$font,[Drawing.Brushes]::DarkBlue,240,500);$g.DrawString('Finished already?',$font,[Drawing.Brushes]::DarkBlue,240,560);$g.DrawString('Space Next',$small,[Drawing.Brushes]::White,1040,665)}finally{$small.Dispose()}}
    'ITEMS' {$g.DrawString('Items Obtained!',$font,[Drawing.Brushes]::White,400,210);$g.DrawString('Select anywhere to continue',$font,[Drawing.Brushes]::White,360,600)}
    'CLAIM' {$g.DrawString('Commission',$font,[Drawing.Brushes]::White,120,30);$g.FillRectangle([Drawing.Brushes]::Turquoise,1000,600,260,90);$g.DrawString('Claim All',$font,[Drawing.Brushes]::White,1020,620)}
    'HOME' {
     $g.Clear([Drawing.Color]::FromArgb(100,180,170))
     $labelFont=[Drawing.Font]::new('Segoe UI',20,[Drawing.FontStyle]::Bold,[Drawing.GraphicsUnit]::Pixel)
     try{
      foreach($label in @(@('Missions',880),@('Heartlink',975),@('Purchase',1070),@('Commission',1160))){
       $path=[Drawing.Drawing2D.GraphicsPath]::new();$pen=[Drawing.Pen]::new([Drawing.Color]::FromArgb(40,65,110),4)
       try{$path.AddString($label[0],$labelFont.FontFamily,[int][Drawing.FontStyle]::Bold,20,[Drawing.PointF]::new([single]$label[1],130),[Drawing.StringFormat]::GenericDefault);$g.DrawPath($pen,$path);$g.FillPath([Drawing.Brushes]::White,$path)}finally{$path.Dispose();$pen.Dispose()}
      }
     }finally{$labelFont.Dispose()}
    }
   }
   $g.Flush([Drawing.Drawing2D.FlushIntention]::Sync)
   $o=Read-Observation $bitmap
   if($o.State -ne $kind){
    if($env:RUNNER_TEMP){$bitmap.Save([IO.Path]::Combine($env:RUNNER_TEMP,('ocr-fixture-'+$kind+'.png')),[Drawing.Imaging.ImageFormat]::Png)}
    $diagnostic=Read-Image $bitmap
    try{foreach($line in $diagnostic.Lines){[Console]::Out.WriteLine(('FIXTURE {0}: {1} at {2:N3},{3:N3}' -f $kind,$line.Text,$line.X,$line.Y))}}finally{$diagnostic.Bitmap.Dispose()}
   }
   Check ($o.State -eq $kind) ('Real OCR screen recognition failed: '+$kind+' -> '+$o.State)
  }
  $merged=@([pscustomobject]@{Text='Purchase Commission';X=.83;Y=.19;W=.16;H=.03})
  $word=@([pscustomobject]@{Text='Commission';X=.91;Y=.19;W=.08;H=.03})
  $o=Observe $merged $bitmap '' $word;Check ($o.State -eq 'HOME' -and [Math]::Abs($o.X-.95) -lt .001) 'Merged home line must target the Commission word only'
  $o=Observe $merged $bitmap '' @($word[0],$word[0]);Check ($o.State -eq 'UNKNOWN') 'Ambiguous home words must not click'
  $o=Observe $merged $bitmap '' @();Check ($o.State -eq 'UNKNOWN') 'Missing home word must not use guessed coordinates'
  $session=New-Session;$story=[pscustomobject]@{State='STORY'};$items=[pscustomobject]@{State='ITEMS'};$result=[pscustomobject]@{State='RESULT'}
  Check ((Next-Action $session ([pscustomobject]@{State='CLAIM'}) 2000) -eq 'CLAIM') 'Claim missing'
  Check ((Next-Action $session $story 2300) -eq 'WAIT') 'Cooldown ignored'
  Check ((Next-Action $session $story 3000) -eq 'ESC') 'Story skip missing'
  Check ((Next-Action $session $items 4000) -eq 'CONTINUE') 'Overlay dismissal missing'
  Check ((Next-Action $session $result 5000) -eq 'AGAIN') 'Repeat missing'
  Check ((Next-Action $session $result 6000) -eq 'WAIT') 'Persistent result ended the run early'
  Check ((Next-Action $session $result 13000) -eq 'ERR_DISPATCH_NOT_CHANGED') 'Unchanged Dispatch Again modal was accepted'
  Check ($session.Again -eq 1) 'Repeat was duplicated'
  # Normal run without optional overlays still observes the page before ending.
  $session=New-Session;$null=Next-Action $session $result 1000
  $page=[pscustomobject]@{State='NO_REWARDS'}
  Check ((Next-Action $session $page 2000) -eq 'WAIT') 'Dispatch ended without observing delayed overlays'
  Check ((Next-Action $session $page 8999) -eq 'WAIT') 'Post-dispatch observation window was shortened'
  Check ((Next-Action $session $page 9000) -eq 'DONE') 'Stable Commission page did not finish'
  # Optional dialogue/loot can appear after Dispatch Again, even after a page.
  $session=New-Session;$null=Next-Action $session $result 1000
  $null=Next-Action $session $page 2000
  Check ((Next-Action $session $story 7000) -eq 'ESC') 'Delayed post-dispatch story was ignored'
  Check ((Next-Action $session $items 7200) -eq 'WAIT') 'Post-story cooldown was skipped'
  Check ((Next-Action $session $items 8000) -eq 'CONTINUE') 'Post-dispatch loot was ignored'
  Check ((Next-Action $session $page 9000) -eq 'WAIT') 'Overlay exit did not reset page stability'
  Check ((Next-Action $session $page 11499) -eq 'WAIT') 'Page stability window was shortened'
  Check ((Next-Action $session $page 11500) -eq 'DONE') 'Post-dispatch overlays did not finish cleanly'
  Check ($session.Again -eq 1 -and $session.Claim -eq 0) 'Post-dispatch cleanup sent another claim/dispatch'
  $session=New-Session;$null=Next-Action $session $result 1000
  for($i=0;$i -lt 8;$i++){$last=Next-Action $session ([pscustomobject]@{State='UNKNOWN'}) (2000+$i*1000)}
  Check ($last -eq 'ERR_UNKNOWN_SCREEN') 'Post-dispatch unknown screen was accepted'
  $session=New-Session;$null=Next-Action $session $result 1000
  Check ((Next-Action $session ([pscustomobject]@{State='HOME'}) 2000) -eq 'ERR_LEFT_COMMISSION') 'Post-dispatch navigated home again'
  $session=New-Session;Check ((Next-Action $session $result 1000) -eq 'AGAIN') 'Resume at results failed'
  $session=New-Session;Check ((Next-Action $session ([pscustomobject]@{State='NO_REWARDS'}) 2000) -eq 'NO_REWARDS') 'Disabled claim was clicked'
  for($i=0;$i -lt 8;$i++){$last=Next-Action $session ([pscustomobject]@{State='UNKNOWN'}) (3000+$i*1000)};Check ($last -eq 'ERR_UNKNOWN_SCREEN') 'Unknown screen did not stop'
  $session=New-Session;Check ((Next-Action $session $story 2000) -eq 'ERR_NOT_COMMISSION') 'Unrelated story was skipped'
  $session=New-Session;Check ((Next-Action $session $items 2000) -eq 'ERR_NOT_COMMISSION') 'Unrelated loot was dismissed'
  $session=New-Session;$commissionHome=[pscustomobject]@{State='HOME'};Check ((Next-Action $session $commissionHome 2000) -eq 'OPEN') 'Home navigation missing';Check ((Next-Action $session $commissionHome 2500) -eq 'WAIT') 'Navigation settle ignored'
  $session=New-Session;$null=Next-Action $session ([pscustomobject]@{State='CLAIM'}) 2000
  Check ((Next-Action $session ([pscustomobject]@{State='NO_REWARDS'}) 2500) -eq 'WAIT') 'Temporary disabled button ended the run'
  Check ((Next-Action $session ([pscustomobject]@{State='NO_REWARDS'}) 7000) -eq 'ERR_REWARD_RESULT_NOT_RECOGNIZED') 'Missing result was reported as no input'
  # Task selections are independent: home-only, dispatch-only, both, neither.
  $homeScreen=[pscustomobject]@{State='HOME'}
  $session=New-Session 576
  Check ((Next-Action $session $homeScreen 1000) -eq 'INTERACT') 'Home touch missing'
  Check ((Next-Action $session $homeScreen 1500) -eq 'WAIT') 'Home interaction did not settle'
  Check ((Next-Action $session $homeScreen 2600) -eq 'OPEN') 'Commission did not follow interaction'
  Check ((Next-Action $session $homeScreen 3000) -eq 'WAIT') 'Home touch repeated during navigation'
  Check ($session.Interact -eq 1 -and $session.Entry -eq 1) 'Home interaction was duplicated'
  Check ((Next-Action $session ([pscustomobject]@{State='CLAIM'}) 5000) -eq 'CLAIM') 'Combined flow lost Commission'
  $session=New-Session 64
  Check ((Next-Action $session $homeScreen 1000) -eq 'INTERACT') 'Home-only touch missing'
  Check ((Next-Action $session $homeScreen 2600) -eq 'DONE') 'Home-only task opened Commission'
  Check ($session.Entry -eq 0 -and $session.Interact -eq 1) 'Home-only ran skipped task'
  $session=New-Session 512
  Check ((Next-Action $session $homeScreen 1000) -eq 'OPEN') 'Dispatch-only touched home character'
  Check ($session.Interact -eq 0) 'Skipped interaction was sent'
  $session=New-Session 576
  Check ((Next-Action $session $result 1000) -eq 'ERR_HOME_REQUIRED') 'Home task ran from Commission modal'
  Check ($session.Actions -eq 0) 'Wrong starting page sent input'
  $session=New-Session 0
  Check ((Next-Action $session $homeScreen 1000) -eq 'ERR_NO_TASKS') 'All skipped tasks sent input'
  # OCR actual synthetic inventory/detail/editor screens, including preview
  # versus committed level/cost. No real game or desktop input is used.
  $smallFont=[Drawing.Font]::new('Segoe UI',24,[Drawing.FontStyle]::Bold,[Drawing.GraphicsUnit]::Pixel)
  $levelFont=[Drawing.Font]::new('Segoe UI',56,[Drawing.FontStyle]::Bold,[Drawing.GraphicsUnit]::Pixel)
  try{
   foreach($kind in @('DISC','TREKKER')){
    foreach($screen in @('LIST','DETAILS','EDIT')){
     $g.Clear([Drawing.Color]::FromArgb(30,50,100))
     $caption='Disc';if($kind -eq 'TREKKER'){$caption='Trekker'}
     if($screen -eq 'LIST'){$caption+=' List'}elseif($kind -eq 'TREKKER'){if($screen -eq 'EDIT'){$caption+=' Training'}else{$caption+=' Details'}}
     $g.DrawString($caption,$font,[Drawing.Brushes]::White,120,25)
     if($screen -eq 'LIST'){
      $g.FillRectangle([Drawing.Brushes]::WhiteSmoke,80,140,230,220)
      $g.DrawString('Lv.1',$smallFont,[Drawing.Brushes]::DarkBlue,100,320)
     }else{
      $g.DrawString(($kind+' Level'),$smallFont,[Drawing.Brushes]::White,800,165);$g.DrawString('1',$levelFont,[Drawing.Brushes]::White,990,135);$g.DrawString('/10',$smallFont,[Drawing.Brushes]::White,1045,165)
      if($screen -eq 'DETAILS'){
       if($kind -eq 'DISC'){$g.DrawString('Melody Skill',$smallFont,[Drawing.Brushes]::White,890,230);$g.DrawString('Upgrade',$smallFont,[Drawing.Brushes]::White,500,655)}
       else{$g.FillRectangle([Drawing.Brushes]::Turquoise,950,510,240,65);$g.DrawString('Upgrade',$smallFont,[Drawing.Brushes]::White,990,525)}
      }else{
       $g.DrawString('Upgrade 1 level',$smallFont,[Drawing.Brushes]::White,860,460)
       $g.DrawString('Upgrade to Max',$smallFont,[Drawing.Brushes]::White,1070,460)
       $priceY=510;$buttonY=555;if($kind -eq 'TREKKER'){$priceY=550;$buttonY=600}
       $g.DrawString('0',$smallFont,[Drawing.Brushes]::White,1090,$priceY)
       $g.FillRectangle([Drawing.Brushes]::Turquoise,950,$buttonY,240,65);$g.DrawString('Upgrade',$smallFont,[Drawing.Brushes]::White,990,($buttonY+15))
      }
     }
     $g.Flush([Drawing.Drawing2D.FlushIntention]::Sync)
     $o=Read-Observation $bitmap
     if($o.State -ne ($kind+'_'+$screen) -or ($screen -ne 'LIST' -and $o.Level -ne 1) -or ($screen -eq 'EDIT' -and $o.Cost -ne 0)){
      $diagnostic=Read-Image $bitmap
      try{foreach($line in $diagnostic.Words){[Console]::Out.WriteLine(('UPGRADE FIXTURE {0}/{1}: {2} at {3:N3},{4:N3} height {5:N3}' -f $kind,$screen,$line.Text,$line.X,$line.Y,$line.H))}}finally{$diagnostic.Bitmap.Dispose()}
     }
     Check ($o.State -eq ($kind+'_'+$screen)) ('Upgrade OCR failed: '+$kind+'/'+$screen+' -> '+$o.State)
     if($screen -eq 'LIST'){Check ($null -ne $o.Cards) 'List candidate set missing'}
     else{Check ($o.Level -eq 1) 'Detail/editor level OCR missing'}
     if($screen -eq 'EDIT'){
      Check ($o.Cost -eq 0) ('Editor cost OCR missing: '+$o.Cost)
      foreach($costValue in @(100,0)){
       $bg=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(30,50,100))
       try{
        $g.FillRectangle($bg,920,90,180,140);$g.DrawString('2',$levelFont,[Drawing.Brushes]::White,990,135)
        $g.FillRectangle($bg,1050,($priceY-10),100,48)
        $ink=[Drawing.Brushes]::White
        if($costValue -eq 0){$g.FillRectangle([Drawing.Brushes]::White,1070,$priceY,75,42);$ink=[Drawing.Brushes]::DarkBlue}
        $g.DrawString([string]$costValue,$smallFont,$ink,1090,$priceY);$g.Flush([Drawing.Drawing2D.FlushIntention]::Sync)
        $changed=Read-Observation $bitmap
        Check ($changed.State -eq ($kind+'_EDIT') -and $changed.Level -eq 2 -and $changed.Cost -eq $costValue) ('Numeric preview/commit fixture failed: '+$kind+' level='+$changed.Level+' cost='+$changed.Cost)
       }finally{$bg.Dispose()}
      }
     }
    }
   }
  }finally{$smallFont.Dispose();$levelFont.Dispose()}
  # Upgrade planner: independent selections, no repeated spending, do not
  # mistake the preview for a committed level or return home too early.
  $upgradeHome=[pscustomobject]@{State='HOME';DiscX=.88;DiscY=.78;TrekkerX=.81;TrekkerY=.82}
  foreach($kind in @('DISC','TREKKER')){
   $mask=4;if($kind -eq 'TREKKER'){$mask=8}
   $session=New-Session $mask
   Check ((Next-Action $session $upgradeHome 1000) -eq ('ENTER_'+$kind)) 'Upgrade task did not open correct list'
   Check ((Next-Action $session $upgradeHome 1500) -eq 'WAIT') 'Upgrade navigation did not settle'
   $list=[pscustomobject]@{State=($kind+'_LIST');Cards=@([pscustomobject]@{Level=1;X=.13;Y=.35})}
   Check ((Next-Action $session $list 2200) -eq 'SELECT_LOW') 'Low-level item was not selected'
   $detail=[pscustomobject]@{State=($kind+'_DETAILS');Level=1;X=.8;Y=.75}
   Check ((Next-Action $session $detail 3400) -eq 'OPEN_UPGRADE') 'Editor did not open'
   $edit=[pscustomobject]@{State=($kind+'_EDIT');Level=1;Cost=0;OneX=.72;OneY=.67;CommitX=.8;CommitY=.8}
   Check ((Next-Action $session $edit 4600) -eq 'PREPARE_LEVEL') 'Upgrade 1 level was not used'
   Check ((Next-Action $session $edit 5800) -eq 'WAIT') 'Unchanged preview triggered spend'
   $edit.Level=2;$edit.Cost=100
   Check ((Next-Action $session $edit 7000) -eq 'COMMIT_UPGRADE') 'Prepared upgrade was not committed'
   Check ((Next-Action $session $edit 8200) -eq 'WAIT') 'Preview alone was accepted as committed'
   Check ($session.UpgradeCommits -eq 1) 'Upgrade spend was repeated'
   $edit.Cost=0
   Check ((Next-Action $session $edit 9400) -eq 'RETURN_HOME') 'Committed level did not return home'
   Check ((Next-Action $session $upgradeHome 10600) -eq 'WAIT') 'Return transition was missed'
   Check ((Next-Action $session $upgradeHome 11800) -eq 'DONE') 'Upgrade-only ran other tasks'
   Check ($session.UpgradeCommits -eq 1 -and $session.Entry -eq 0 -and $session.Interact -eq 0) 'Skipped tasks or duplicate upgrade ran'
  }
  $session=New-Session 4;$null=Next-Action $session $upgradeHome 1000
  Check ((Next-Action $session ([pscustomobject]@{State='DISC_LIST';Cards=@()}) 2200) -eq 'SELECT_LOW') 'Unreadable tiny labels did not open the inspection candidate'
  Check ((Next-Action $session ([pscustomobject]@{State='DISC_DETAILS';Level=35}) 3400) -eq 'ERR_LOW_LEVEL_NOT_FOUND') 'High-level fallback candidate was upgraded'
  Check ($session.UpgradeCommits -eq 0) 'Unvalidated card spent currency'
  $session=New-Session 4;$session.UpgradeKind='DISC';$session.UpgradePhase='VERIFY';$session.UpgradeAt=1000;$session.BeforeLevel=1;$session.TargetLevel=2;$session.UpgradeCommits=1
  Check ((Next-Action $session ([pscustomobject]@{State='DISC_EDIT';Level=2;Cost=100}) 11000) -eq 'ERR_UPGRADE_NOT_CONFIRMED') 'Unconfirmed spend was retried or accepted'
  Check ($session.UpgradeCommits -eq 1) 'Failed upgrade spent twice'
  $session=New-Session 4;$session.UpgradeKind='DISC';$session.UpgradePhase='EDIT';$session.BeforeLevel=1
  Check ((Next-Action $session ([pscustomobject]@{State='DISC_EDIT';Level=3;Cost=500}) 2200) -eq 'ERR_UPGRADE_EXISTING_SELECTION') 'Existing material selection was overwritten'
  $session=New-Session 588;$session.DiscDone=$true;$session.TrekkerDone=$true;$session.Interact=1;$session.Actions=15
  Check ((Next-Action $session $upgradeHome 20000) -eq 'OPEN') 'Completed upgrades did not hand off to Commission'
  $session=New-Session 12;$session.DiscDone=$true
  Check ((Next-Action $session $upgradeHome 2000) -eq 'ENTER_TREKKER') 'Disc did not hand off to Trekker'
  Signal 'SELF_TEST_PASS'
 }finally{$g.Dispose();$bitmap.Dispose();if($font){$font.Dispose()}}
 exit 0
}
try {
 [CommissionDesktop]::Init($GameWindow,$GamePid);$session=New-Session $Tasks;$clock=[Diagnostics.Stopwatch]::StartNew();Signal 'READY'
 while($clock.ElapsedMilliseconds -lt 180000){
  $bitmap=[CommissionDesktop]::Capture()
  try{$observation=Read-Observation $bitmap}finally{$bitmap.Dispose()}
  $action=Next-Action $session $observation $clock.ElapsedMilliseconds
  Signal ('STATE_'+$observation.State)
  if($action -eq 'DONE' -or $action -eq 'NO_REWARDS'){Signal $action;exit 0}
  if($action.StartsWith('ERR_')){Signal $action;exit 3}
  if($action -eq 'WAIT'){Start-Sleep -Milliseconds 450;continue}
  Signal ('ACTION_'+$action)
  # The owner confirmed one touch on the displayed home character completes
  # interaction. Its center point is based on the supplied home layout; this
  # is not character detection. Require recognized HOME before sending it.
  $targetX=$observation.X;$targetY=$observation.Y
  if($action -eq 'INTERACT'){$targetX=0.50;$targetY=0.50}
  if($action -in @('ENTER_DISC','ENTER_TREKKER','SELECT_LOW','OPEN_UPGRADE','PREPARE_LEVEL','COMMIT_UPGRADE','RETURN_HOME')){$targetX=$session.TargetX;$targetY=$session.TargetY}
  [CommissionDesktop]::Act(($action -eq 'ESC'),$targetX,$targetY)
  Start-Sleep -Milliseconds 500
 }
 Signal 'ERR_TIMEOUT';exit 3
} catch {Signal 'ERR_CONTEXT_OR_CAPTURE';exit 3}
