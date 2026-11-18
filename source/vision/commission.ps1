# SPDX-License-Identifier: AGPL-3.0-or-later
param([long]$GameWindow=0,[int]$GamePid=0,[int]$Tasks=512,[switch]$SelfTest,[switch]$Inspect,[ValidateSet('DISC','TREKKER')][string]$InspectKind='DISC')
$ErrorActionPreference='Stop'
$ProgressPreference='SilentlyContinue'
$script:TransitionTimeout=10000
$script:DispatchTimeout=8000;$script:NavigationRetry=1800;$script:EntryTimeout=3000;$script:ClaimRetry=1500;$script:OverlayRetry=1800
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
 public static void InspectForeground(){Window=GetForegroundWindow();GetWindowThreadProcessId(Window,out Pid);Width=Height=0;try{SetThreadDpiAwarenessContext(new IntPtr(-4));}catch(EntryPointNotFoundException){}View();}
 public static void Init(long window,int pid){Window=new IntPtr(window);Pid=(uint)pid;try{SetThreadDpiAwarenessContext(new IntPtr(-4));}catch(EntryPointNotFoundException){} View();}
 static Rect View(){
  uint pid;var name=new StringBuilder(64);Rect r;
  if(!IsWindow(Window)||IsIconic(Window)||GetWindowThreadProcessId(Window,out pid)==0||pid!=Pid||GetClassName(Window,name,64)==0||name.ToString()!="UnityWndClass"||GetForegroundWindow()!=Window||!GetClientRect(Window,out r))throw new Exception("CONTEXT");
  int w=r.R-r.L,h=r.B-r.T;if(w<320||h<180)throw new Exception("SIZE");
  if(Width!=0&&(w!=Width||h!=Height))throw new Exception("RESIZED");Width=w;Height=h;return r;
 }
 // Detect OS dialogs covering a numeric region; foreground Unity alone is insufficient.
 public static bool VisibleRegion(double x,double y,double w,double h){
  Rect r=View();
  for(int row=0;row<3;row++)for(int col=0;col<3;col++){
   var point=new Point{X=(int)((x+w*(col+.5)/3)*(r.R-r.L)),Y=(int)((y+h*(row+.5)/3)*(r.B-r.T))};
   if(!ClientToScreen(Window,ref point)||GetAncestor(WindowFromPoint(point),2)!=Window)return false;
  }return true;
 }
 // Isolate only the large yellow/green current-level glyphs. White cap/XP
 // text and adjacent ascension diamonds are outside this dedicated region.
 public static Bitmap CounterCrop(Bitmap source,double rx,double ry,double rw,double rh){
  int left=(int)(rx*source.Width),top=(int)(ry*source.Height),w=(int)(rw*source.Width),h=(int)(rh*source.Height);
  var result=new Bitmap(w*4+8,h*4+8,PixelFormat.Format32bppArgb);
  using(var g=Graphics.FromImage(result)){
   g.Clear(Color.Black);
   for(int y=0;y<h;y++)for(int x=0;x<w;x++){
    Color c=source.GetPixel(left+x,top+y);
    bool ink=(c.R>=155&&c.G>=140&&c.B<160)||(c.R>=80&&c.G>=145&&c.G-c.R>=20&&c.B<=155);
    if(!ink)continue;
    if(x==0||y==0||x==w-1||y==h-1){result.Dispose();return null;}
    g.FillRectangle(Brushes.White,4+x*4,4+y*4,4,4);
   }
  }return result;
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
 static readonly System.Collections.Generic.List<bool[]> CostTemplates=new System.Collections.Generic.List<bool[]>();
 static readonly System.Collections.Generic.List<int> CostTemplateDigits=new System.Collections.Generic.List<int>();
 static void MakeDigits(bool tilted){
  var templates=tilted?CostTemplates:DigitTemplates;var digits=tilted?CostTemplateDigits:TemplateDigits;
  if(templates.Count!=0)return;
  foreach(string family in new[]{"Segoe UI","Arial","Calibri","Tahoma","Verdana"})foreach(FontStyle style in new[]{FontStyle.Regular,FontStyle.Bold})for(int digit=0;digit<10;digit++)foreach(int angle in tilted?new[]{-12,-6,0,6,12}:new[]{0}){
   using(var b=new Bitmap(100,100))using(var g=Graphics.FromImage(b))using(var f=new Font(family,64,style,GraphicsUnit.Pixel)){
    g.Clear(Color.Black);g.TranslateTransform(25,10);g.RotateTransform(angle);g.TextRenderingHint=System.Drawing.Text.TextRenderingHint.AntiAliasGridFit;g.DrawString(digit.ToString(),f,Brushes.White,0,0);g.Flush();
    var mask=new bool[10000];int l=100,t=100,r=0,bt=0;
    for(int y=0;y<100;y++)for(int x=0;x<100;x++)if(b.GetPixel(x,y).R>150){mask[y*100+x]=true;l=Math.Min(l,x);t=Math.Min(t,y);r=Math.Max(r,x);bt=Math.Max(bt,y);}
    if(r>=l&&bt>=t){templates.Add(Normalize(mask,100,l,t,r-l+1,bt-t+1));digits.Add(digit);}
   }
  }
 }
 static int Classify(bool[] sample,bool tilted){
  var templates=tilted?CostTemplates:DigitTemplates;var digits=tilted?CostTemplateDigits:TemplateDigits;
  var scores=new double[10];for(int d=0;d<10;d++)scores[d]=1;
  for(int i=0;i<templates.Count;i++){
   bool[] target=templates[i];double best=1;
   for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){
    int mismatch=0,union=0;
    for(int y=0;y<32;y++)for(int x=0;x<24;x++){
     bool a=sample[y*24+x],b=x+dx>=0&&x+dx<24&&y+dy>=0&&y+dy<32&&target[(y+dy)*24+x+dx];if(a||b)union++;if(a!=b)mismatch++;
    }if(union>0)best=Math.Min(best,(double)mismatch/union);
   }scores[digits[i]]=Math.Min(scores[digits[i]],best);
  }
  int winner=0;for(int d=1;d<10;d++)if(scores[d]<scores[winner])winner=d;
  double runner=1;for(int d=0;d<10;d++)if(d!=winner)runner=Math.Min(runner,scores[d]);
  return scores[winner]<=0.30&&runner-scores[winner]>=0.035?winner:-1;
 }
 public static int Number(Bitmap source,double rx,double ry,double rw,double rh,bool bright,double minHeight,double maxHeight){return ReadNumber(source,rx,ry,rw,rh,bright,minHeight,maxHeight,false);}
 public static int CostNumber(Bitmap source,double rx,double ry,double rw,double rh,bool bright,double minHeight,double maxHeight){return ReadNumber(source,rx,ry,rw,rh,bright,minHeight,maxHeight,true);}
 static int ReadNumber(Bitmap source,double rx,double ry,double rw,double rh,bool bright,double minHeight,double maxHeight,bool tilted){
  MakeDigits(tilted);int left=(int)(rx*source.Width),top=(int)(ry*source.Height),w=(int)(rw*source.Width),h=(int)(rh*source.Height);
  var mask=new bool[w*h];var seen=new bool[w*h];var queue=new int[w*h];var components=new System.Collections.Generic.List<int[]>();
  for(int y=0;y<h;y++)for(int x=0;x<w;x++){
   Color c=source.GetPixel(left+x,top+y);mask[y*w+x]=bright?((c.R>150&&c.G>140&&(c.B<180||(c.R>190&&c.G>190&&c.B>190)))||(!tilted&&c.R>=80&&c.G>=145&&c.G-c.R>=20&&c.B<=155)):(c.R<130&&c.G<150&&c.B<220);
  }
  for(int p=0;p<mask.Length;p++)if(mask[p]&&!seen[p]){
   int head=0,tail=0,l=w,t=h,r=0,bt=0;queue[tail++]=p;seen[p]=true;
   while(head<tail){int v=queue[head++],x=v%w,y=v/w;l=Math.Min(l,x);t=Math.Min(t,y);r=Math.Max(r,x);bt=Math.Max(bt,y);
    for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){int nx=x+dx,ny=y+dy;if(nx<0||nx>=w||ny<0||ny>=h)continue;int n=ny*w+nx;if(mask[n]&&!seen[n]){seen[n]=true;queue[tail++]=n;}}
   }
   int ch=bt-t+1,cw=r-l+1;
   if(l>0&&t>0&&r<w-1&&bt<h-1&&ch>=minHeight*source.Height&&ch<=maxHeight*source.Height&&cw>=2){
    // Never drop a digit-height merged glyph group and parse only its suffix.
    if(tilted&&cw>ch*1.1)return -1;
    if(cw<=ch*1.1)components.Add(new[]{l,t,cw,ch});
   }
  }
  if(components.Count==0)return -1;int tallest=0;foreach(var c in components)tallest=Math.Max(tallest,c[3]);
  components.RemoveAll(c=>c[3]<tallest*0.80);components.Sort((a,b)=>a[0].CompareTo(b[0]));
  if(components.Count==0||components.Count>6)return -1;int result=0;
  foreach(var c in components){int digit=Classify(Normalize(mask,w,c[0],c[1],c[2],c[3]),tilted);if(digit<0)return -1;result=result*10+digit;}return result;
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
  if($level -ge 1 -and $level -lt 90){$cards+=[pscustomobject]@{Level=$level;X=$word.X+$word.W/2;Y=$word.Y-.08}}
 }
 return @($cards | Sort-Object Level,Y,X)
}
function Visible-Level($words,[bool]$discTop=$false){
 $minY=.13;$maxY=.30;if($discTop){$minY=.01;$maxY=.12}
 $values=@($words | Where-Object {$_.Text -match '^\d{1,3}(?:/\d{1,3})?$' -and $_.X -gt .70 -and $_.X -lt .85 -and $_.Y -gt $minY -and $_.Y -lt $maxY -and $_.H -gt .032})
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
 if($kind -eq 'TREKKER'){$level=Trekker-Level $words ('TREKKER_'+$page)}
 if($one.Count -eq 1 -and $max.Count -eq 1 -and $confirm.Count -eq 1){
  $minY=.68;$maxY=.75;if($kind -eq 'TREKKER'){$minY=.75;$maxY=.81}
  $prices=@($words | Where-Object {$_.Text -match '^\d[\d,]*$' -and $_.X -gt .82 -and $_.X -lt .90 -and $_.Y -gt $minY -and $_.Y -lt $maxY})
  $cost=-1;if($prices.Count -eq 1){$cost=[int]($prices[0].Text.Replace(',',''))}
  return [pscustomobject]@{State=($kind+'_EDIT');X=0.0;Y=0.0;Level=$level;Cost=$cost;OneX=$one[0].X+$one[0].W/2;OneY=$one[0].Y+$one[0].H/2;CommitX=$confirm[0].X+$confirm[0].W/2;CommitY=$confirm[0].Y+$confirm[0].H/2}
 }
 if($kind -eq 'DISC'){
  $tab=@(Label $lines 'upgrade' .34 .52 .88 1)
  $skill=@(Label $lines 'melodyskill' .65 .9 .15 .35)
  $harmony=@(Label $lines 'harmonyskill' .65 .95 .40 .65)
  if($skill.Count -eq 1 -and ($tab.Count -eq 1 -or $harmony.Count -eq 1)){
   $x=.435;$y=.923;if($tab.Count -eq 1){$x=$tab[0].X+$tab[0].W/2;$y=$tab[0].Y+$tab[0].H/2}
   return [pscustomobject]@{State='DISC_DETAILS';X=$x;Y=$y;Level=(Visible-Level $words $true)}
  }
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
# Optical fallback is limited to the Disc candidate flow already entered by
# this run. Multiple independent layout checks must agree; no text is guessed.
function Color-Patch($bitmap,[double]$x,[double]$y,[double]$w,[double]$h,[string]$kind){
 $hits=0
 for($row=0;$row -lt 3;$row++){for($col=0;$col -lt 5;$col++){
  $px=[int](($x+$w*($col+.5)/5)*$bitmap.Width);$py=[int](($y+$h*($row+.5)/3)*$bitmap.Height)
  $c=$bitmap.GetPixel($px,$py);$match=$false
  switch($kind){
   'CYAN' {$match=$c.R -lt 110 -and $c.G -gt 135 -and $c.B -gt 120 -and $c.G-$c.R -gt 45}
   'WHITE' {$match=$c.R -gt 190 -and $c.G -gt 190 -and $c.B -gt 180}
   'BLUE' {$match=$c.R -lt 110 -and $c.G -lt 145 -and $c.B -gt 55 -and $c.B-$c.R -gt 25 -and $c.B-$c.G -gt 5}
  }
  if($match){$hits++}
 }}
 return $hits -ge 10
}
function Disc-Layout-Checks($bitmap){
 $harmony=$false
 for($y=.40;$y -le .62;$y+=.006){if(Color-Patch $bitmap .84 $y .06 .012 'CYAN'){$harmony=$true;break}}
 return [pscustomobject]@{
  Header=(Color-Patch $bitmap .17 .04 .03 .04 'BLUE')
  House=(Color-Patch $bitmap .29 .055 .009 .012 'WHITE')
  DetailsActive=(Color-Patch $bitmap .30 .943 .015 .006 'BLUE')
  DetailsInactive=(Color-Patch $bitmap .30 .943 .015 .006 'WHITE')
  UpgradeActive=(Color-Patch $bitmap .48 .943 .012 .006 'BLUE')
  UpgradeInactive=(Color-Patch $bitmap .48 .943 .012 .006 'WHITE')
  Melody=(Color-Patch $bitmap .84 .207 .06 .015 'CYAN')
  Harmony=$harmony
  OnePill=(Color-Patch $bitmap .70 .636 .04 .008 'WHITE')
  MaxPill=(Color-Patch $bitmap .82 .636 .04 .008 'WHITE')
  Commit=(Color-Patch $bitmap .73 .736 .08 .007 'CYAN')
 }
}
function Disc-Layout-FromChecks($c,$session,[bool]$discCaption=$false){
 if(!$session -or $session.UpgradeKind -ne 'DISC'){return $null}
 if(!$c.Header -or !$c.House){return $null}
 # Actual owner inspection: header/tab checks succeed, skill-band color
 # checks fail. A localized Disc caption supplies the independent page anchor.
 $detailAnchor=$discCaption -or ($c.Melody -and $c.Harmony)
 if($session.UpgradePhase -in @('DETAILS','EDIT') -and $c.DetailsActive -and !$c.DetailsInactive -and $c.UpgradeInactive -and !$c.UpgradeActive -and !$c.Commit -and $detailAnchor){
  return [pscustomobject]@{State='DISC_DETAILS';X=.435;Y=.923;Level=0;Layout=$true}
 }
 if($session.UpgradePhase -in @('EDIT','PREPARED','VERIFY') -and $c.UpgradeActive -and $c.DetailsInactive -and $c.OnePill -and $c.MaxPill -and $c.Commit){
  return [pscustomobject]@{State='DISC_EDIT';X=0.0;Y=0.0;Level=0;Cost=-1;OneX=.72;OneY=.657;CommitX=.785;CommitY=.763;Layout=$true}
 }
 return $null
}
function Disc-Layout($bitmap,$session,[bool]$discCaption=$false){
 if(!$session -or $session.UpgradeKind -ne 'DISC'){return $null}
 return Disc-Layout-FromChecks (Disc-Layout-Checks $bitmap) $session $discCaption
}
function Trekker-Level-Region([string]$state){
 if($state -eq 'TREKKER_DETAILS'){return [pscustomobject]@{X=.805;Y=.20;W=.065;H=.105}}
 return [pscustomobject]@{X=.75;Y=.18;W=.065;H=.115}
}
function Trekker-Level($words,[string]$state){
 $r=Trekker-Level-Region $state
 $localWords=@($words | Where-Object {$_.X -ge $r.X -and $_.X -lt $r.X+$r.W -and $_.Y -ge $r.Y -and $_.Y -lt $r.Y+$r.H})
 return Visible-Level $localWords
}
function Trekker-Layout-Checks($bitmap){
 return [pscustomobject]@{
  Header=(Color-Patch $bitmap .085 .04 .009 .025 'BLUE')
  House=(Color-Patch $bitmap .29 .055 .009 .012 'WHITE')
  DetailsActive=(Color-Patch $bitmap .17 .943 .015 .006 'BLUE')
  SkillsInactive=(Color-Patch $bitmap .30 .943 .015 .006 'WHITE')
  HP=(Color-Patch $bitmap .88 .389 .035 .010 'WHITE')
  ATK=(Color-Patch $bitmap .88 .43 .035 .010 'WHITE')
  DEF=(Color-Patch $bitmap .88 .473 .035 .010 'WHITE')
  OpenUpgrade=(Color-Patch $bitmap .81 .704 .09 .012 'CYAN')
  OnePill=(Color-Patch $bitmap .71 .695 .04 .008 'WHITE')
  MaxPill=(Color-Patch $bitmap .84 .695 .04 .008 'WHITE')
  Commit=(Color-Patch $bitmap .75 .800 .07 .007 'CYAN')
 }
}
function Trekker-Layout($bitmap,$session,$lines){
 if(!$session -or $session.UpgradeKind -ne 'TREKKER'){return $null}
 $c=Trekker-Layout-Checks $bitmap
 if(!$c.Header -or !$c.House){return $null}
 $details=@(Label $lines 'trekkerdetails' 0 .30 0 .12).Count -eq 1
 $training=@(Label $lines 'trekkertraining' 0 .30 0 .12).Count -eq 1
 if($session.UpgradePhase -in @('DETAILS','EDIT','BACK_DETAILS') -and $details -and $c.DetailsActive -and $c.SkillsInactive -and $c.HP -and $c.ATK -and $c.DEF -and $c.OpenUpgrade -and !$c.Commit){
  return [pscustomobject]@{State='TREKKER_DETAILS';X=.865;Y=.73;Level=0;Layout=$true}
 }
 if($session.UpgradePhase -in @('EDIT','PREPARED','VERIFY') -and $training -and $c.OnePill -and $c.MaxPill -and $c.Commit -and !$c.OpenUpgrade){
  return [pscustomobject]@{State='TREKKER_EDIT';X=0.0;Y=0.0;Level=0;Cost=-1;OneX=.737;OneY=.715;CommitX=.804;CommitY=.826;Layout=$true}
 }
 return $null
}
function Upgrade-Layout($bitmap,$session,$lines){
 if(!$session){return $null}
 if($session.UpgradeKind -eq 'TREKKER'){return Trekker-Layout $bitmap $session $lines}
 $caption=@(Label $lines 'disc' 0 .30 0 .12).Count -eq 1
 return Disc-Layout $bitmap $session $caption
}
function Read-Trekker-Cost($bitmap){
 # Training's price is lower and farther right than the Disc price. Exclude
 # its own currency icon and retain tilted-digit recognition for this card.
 $values=@()
 foreach($bright in @($true,$false)){
  $n=[CommissionDesktop]::CostNumber($bitmap,.859,.75,.064,.078,$bright,.012,.045)
  if($n -ge 0){$values+=$n}
 }
 $values=@($values | Select-Object -Unique)
 if($values.Count -eq 1){return [int]$values[0]}
 return -1
}
function Read-Disc-Cost($bitmap){
 # Exclude the currency icon left of the digits. It is taller than the zero
 # and was previously selected as the largest numeric component.
 $values=@()
 foreach($bright in @($true,$false)){
  $n=[CommissionDesktop]::CostNumber($bitmap,.842,.68,.058,.08,$bright,.012,.045)
  if($n -ge 0){$values+=$n}
 }
 $values=@($values | Select-Object -Unique)
 if($values.Count -eq 1){return [int]$values[0]}
 return -1
}
function Upgrade-Result-Overlay($bitmap,$session){
 # Only recognize the full-width Power Up result after this run confirmed an
 # upgrade. The banner itself is not proof of the committed editor state.
 if(!$session -or $session.UpgradeKind -notin @('DISC','TREKKER') -or $session.UpgradePhase -ne 'VERIFY' -or $session.UpgradeCommits -lt 1){return $null}
 if((Color-Patch $bitmap .05 .36 .14 .025 'CYAN') -and
    (Color-Patch $bitmap .82 .36 .14 .025 'CYAN') -and
    (Color-Patch $bitmap .05 .46 .16 .13 'WHITE') -and
    (Color-Patch $bitmap .80 .46 .15 .13 'WHITE')){
  return [pscustomobject]@{State='POWER_UP';X=.50;Y=.85}
 }
 return $null
}
function Focused-Level-Region([string]$state){
 if($state -eq 'DISC_EDIT'){return [pscustomobject]@{X=.718;Y=.158;W=.045;H=.080}}
 if($state -eq 'TREKKER_EDIT'){return [pscustomobject]@{X=.748;Y=.18;W=.050;H=.10}}
 return $null
}
function Read-Focused-Level($bitmap,[string]$state){
 $script:FocusedFacts='NO_REGION';$r=Focused-Level-Region $state;if(!$r){return 0}
 $crop=[CommissionDesktop]::CounterCrop($bitmap,$r.X,$r.Y,$r.W,$r.H);if(!$crop){$script:FocusedFacts='CLIPPED';return 0}
 $read=$null
 try{
  $read=Read-Image $crop
  $values=@($read.Words | Where-Object {$_.Text -match '^\d{1,2}$'})
  $ocr=0;if($values.Count -eq 1){$ocr=[int]$values[0].Text}
  $shape=[CommissionDesktop]::Number($crop,0,0,1,1,$true,.15,.95)
  $script:FocusedFacts=('OCR={0}; shape={1}' -f $ocr,$shape)
  if($ocr -gt 0 -and $shape -gt 0 -and $ocr -ne $shape){return 0}
  if($ocr -ge 1 -and $ocr -le 99){return $ocr}
  if($shape -ge 1 -and $shape -le 99){return $shape}
  return 0
 }finally{if($read){$read.Bitmap.Dispose()};$crop.Dispose()}
}
function Read-Observation($bitmap,$session=$null){
 $resultOverlay=Upgrade-Result-Overlay $bitmap $session
 if($resultOverlay){return $resultOverlay}
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
  if($observation.State -eq 'UNKNOWN'){$layout=Upgrade-Layout $bitmap $session $read.Lines;if($layout){$observation=$layout}}
  if($observation.State -eq 'UNKNOWN'){
   $regions=@(@(.08,.015,.23,.085),@(.69,.18,.29,.09),@(.69,.52,.29,.08),@(.35,.89,.15,.075))
   foreach($region in $regions){
    $crop=[CommissionDesktop]::Region($bitmap,$region[0],$region[1],$region[2],$region[3],$false);$detail=$null
    try{
     $detail=Read-Image $crop
     # Replace overlapping full-frame lines instead of duplicating labels.
     $kept=@($read.Lines | Where-Object {!($_.X -ge $region[0] -and $_.X -lt ($region[0]+$region[2]) -and $_.Y -ge $region[1] -and $_.Y -lt ($region[1]+$region[3]))})
     $mapped=@(foreach($line in $detail.Lines){
      $mw=@(foreach($word in $line.Words){[pscustomobject]@{Text=$word.Text;X=$region[0]+$word.X*$region[2];Y=$region[1]+$word.Y*$region[3];W=$word.W*$region[2];H=$word.H*$region[3]}})
      [pscustomobject]@{Text=$line.Text;Words=$mw;X=$region[0]+$line.X*$region[2];Y=$region[1]+$line.Y*$region[3];W=$line.W*$region[2];H=$line.H*$region[3]}
     })
     $read.Lines=@($kept)+@($mapped)
     $observation=Observe $read.Lines $read.Bitmap $tokens $read.Words
     if($observation.State -eq 'UNKNOWN'){$layout=Upgrade-Layout $bitmap $session $read.Lines;if($layout){$observation=$layout}}
     if($observation.State -ne 'UNKNOWN'){break}
     # Continue lower Disc regions only when the header indicates a Disc page.
     if($region[1] -lt .1 -and @(Label $read.Lines 'disc' 0 .3 0 .12).Count -ne 1){break}
    }finally{$crop.Dispose();if($detail){$detail.Bitmap.Dispose()}}
   }
  }
  $discTop=$observation.State -eq 'DISC_DETAILS'
  $levelX=.72;$levelW=.14;$levelY=.12;$levelH=.20
  if($discTop){$levelY=.01;$levelH=.10}
  if($observation.State -in @('TREKKER_DETAILS','TREKKER_EDIT')){$r=Trekker-Level-Region $observation.State;$levelX=$r.X;$levelY=$r.Y;$levelW=$r.W;$levelH=$r.H}
  if($observation.State -in @('DISC_DETAILS','TREKKER_DETAILS','DISC_EDIT','TREKKER_EDIT')){
   if($observation.Level -eq 0 -and !($discTop -and $observation.Layout)){
    foreach($invert in @($false,$true)){
     $crop=[CommissionDesktop]::Region($bitmap,$levelX,$levelY,$levelW,$levelH,$invert);$detail=$null
     try{
      $detail=Read-Image $crop
      $mapped=@(foreach($word in $detail.Words){[pscustomobject]@{Text=$word.Text;X=$levelX+$word.X*$levelW;Y=$levelY+$word.Y*$levelH;W=$word.W*$levelW;H=$word.H*$levelH}})
      $observation.Level=Visible-Level $mapped $discTop
      if($observation.Level -gt 0){break}
     }finally{$crop.Dispose();if($detail){$detail.Bitmap.Dispose()}}
    }
   }
  }
  if($observation.State -in @('DISC_DETAILS','TREKKER_DETAILS','DISC_EDIT','TREKKER_EDIT')){
   if(!$observation.Level -and !($discTop -and $observation.Layout)){$n=[CommissionDesktop]::Number($bitmap,$levelX,$levelY,$levelW,$levelH,$true,.032,.10);if($n -gt 0){$observation.Level=$n}}
   if(!$observation.Level -and $observation.State -in @('DISC_EDIT','TREKKER_EDIT')){$observation.Level=Read-Focused-Level $bitmap $observation.State}
   if($observation.State -in @('DISC_EDIT','TREKKER_EDIT') -and $observation.Cost -lt 0){
    if($observation.State -eq 'DISC_EDIT'){$observation.Cost=Read-Disc-Cost $bitmap}
    else{$observation.Cost=Read-Trekker-Cost $bitmap}
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
function New-Session([int]$selectedTasks=512) {return @{DiscEnabled=($selectedTasks -band 4) -ne 0;TrekkerEnabled=($selectedTasks -band 8) -ne 0;DiscDone=$false;TrekkerDone=$false;TriedCards=@();UpgradeKind='';UpgradePhase='';UpgradeAt=0;BeforeLevel=0;TargetLevel=0;UpgradeCommits=0;UpgradeOpenAttempts=0;UpgradeOverlayCloses=0;TargetX=0.0;TargetY=0.0;HomeEnabled=($selectedTasks -band 64) -ne 0;DispatchEnabled=($selectedTasks -band 512) -ne 0;Interact=0;Entry=0;Claim=0;Skip=0;Items=0;Again=0;AgainAt=0;StableSince=-1;StableState='';Last=0;Unknown=0;Actions=0;Commission=$false}}
# Live input is gated by two matching observations, separated in time. A loading
# frame or incomplete numeric read never authorizes an action. State transitions
# invalidate the gate; the spend itself remains non-repeatable in Next-Upgrade.
function Observation-Ready($session,$observation,[long]$now){
 $valid=$observation.State -ne 'UNKNOWN'
 if($observation.State -in @('DISC_EDIT','TREKKER_EDIT')){$valid=$valid -and $observation.Level -ge 1 -and $observation.Cost -ge 0}
 $key=('{0}|{1}|{2}|{3}|{4}' -f $session.UpgradeKind,$session.UpgradePhase,$observation.State,$observation.Level,$observation.Cost)
 if(!$valid){$session.SettleKey='';$session.SettleAt=$now;return $false}
 if(!$session.ContainsKey('SettleKey') -or $session.SettleKey -ne $key){$session.SettleKey=$key;$session.SettleAt=$now;return $false}
 return ($now-$session.SettleAt -ge 400)
}
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
  $session.TriedCards=@();$session.UpgradeOverlayCloses=0;$session.UpgradeKind=$kind;$session.UpgradePhase='LIST';$session.UpgradeAt=$now
  $session.TargetX=$x;$session.TargetY=$y;return ('ENTER_'+$kind)
 }
 $phase=$session.UpgradePhase
 if($phase -eq 'RETURN' -and $observation.State -eq 'HOME'){
  $session.UpgradeKind='';$session.UpgradePhase='';$session.Unknown=0;return 'WAIT'
 }
 if($now-$session.Last -lt 900){return 'WAIT'}
 if($phase -eq 'BACK_DETAILS' -and $observation.State -eq 'TREKKER_DETAILS'){
  $session.TargetX=.046;$session.TargetY=.061;$session.UpgradePhase='LIST';$session.UpgradeAt=$now;return 'BACK_LIST'
 }
 if($phase -eq 'LIST' -and $observation.State -eq ($kind+'_LIST')){
  # Inspect each visible reference cell at most once. Readable low levels
  # are preferred, but every selected card must pass the detail-level gate.
  $columns=5;if($kind -eq 'TREKKER'){$columns=6}
  $queue=@()
  foreach($card in $observation.Cards){
   if($card.Level -ge 90){continue}
   $row=0;if($card.Y -gt .5){$row=1}
   $col=[Math]::Max(0,[Math]::Min($columns-1,[int][Math]::Floor(($card.X-.05)/(.90/$columns))))
   $queue+=($row*$columns+$col)
  }
  $queue+=@(0..($columns*2-1))
  $chosen=-1
  foreach($index in $queue){if($session.TriedCards -notcontains $index){$chosen=$index;break}}
  if($chosen -lt 0){return 'ERR_LOW_LEVEL_NOT_FOUND'}
  $session.TriedCards+=@($chosen)
  $session.TargetX=.05+($chosen%$columns+.5)*(.90/$columns)
  $session.TargetY=.31;if($chosen -ge $columns){$session.TargetY=.68}
  $session.UpgradePhase='DETAILS';$session.UpgradeAt=$now;return 'SELECT_LOW'
 }
 if($phase -eq 'DETAILS' -and $observation.State -eq ($kind+'_DETAILS')){
  if($observation.Level -ge 90){
   $session.TargetX=.046;$session.TargetY=.061
   $session.UpgradePhase='LIST';$session.UpgradeAt=$now;return 'BACK_LIST'
  }
  $session.BeforeLevel=$observation.Level;$session.TargetX=$observation.X;$session.TargetY=$observation.Y
  $session.UpgradeOpenAttempts=1;$session.UpgradePhase='EDIT';$session.UpgradeAt=$now;return 'OPEN_UPGRADE'
 }
 if($phase -eq 'EDIT' -and $observation.State -eq ($kind+'_DETAILS')){
  # A navigation click can focus Unity without changing its tab. Retry this
  # harmless page transition once, only after re-recognizing the same detail
  # layout. Never apply this policy to PREPARE_LEVEL or COMMIT_UPGRADE.
  if($session.BeforeLevel -gt 0 -and $observation.Level -gt 0 -and $observation.Level -ne $session.BeforeLevel){return 'ERR_UPGRADE_SELECTION_CHANGED'}
  if($session.UpgradeOpenAttempts -lt 2 -and $now-$session.Last -ge $script:NavigationRetry){
   $session.UpgradeOpenAttempts++
   $session.TargetX=.435;$session.TargetY=.923
   if($kind -eq 'TREKKER'){$session.TargetX=$observation.X;$session.TargetY=$observation.Y}
   return 'REOPEN_UPGRADE'
  }
 }
 if($phase -eq 'EDIT' -and $observation.State -eq ($kind+'_EDIT')){
  # Opening the editor spends nothing. An unreadable detail counter may
  # reach this point, but the editor's level and empty selection are mandatory.
  if($observation.Level -lt 1){return 'ERR_UPGRADE_LEVEL_UNREADABLE'}
  if($observation.Level -ge 90){$session.TargetX=.046;$session.TargetY=.061;$session.UpgradePhase='LIST';if($kind -eq 'TREKKER'){$session.UpgradePhase='BACK_DETAILS'};$session.UpgradeAt=$now;return 'BACK_LIST'}
  if(!$session.BeforeLevel){$session.BeforeLevel=$observation.Level}
  if($observation.Cost -lt 0){return 'ERR_UPGRADE_COST_UNREADABLE'}
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
 if($phase -eq 'VERIFY' -and $observation.State -eq 'POWER_UP'){
  if($now-$session.UpgradeAt -ge $script:TransitionTimeout){return 'ERR_UPGRADE_OVERLAY_NOT_CLOSED'}
  if($session.UpgradeOverlayCloses -gt 0 -and $now-$session.Last -lt $script:OverlayRetry){return 'WAIT'}
  if($session.UpgradeOverlayCloses -ge 2){return 'WAIT'}
  $session.UpgradeOverlayCloses++
  $session.TargetX=$observation.X;$session.TargetY=$observation.Y
  return 'CLOSE_POWER_UP'
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
 if($now-$session.UpgradeAt -ge $script:TransitionTimeout){
  if($phase -eq 'PREPARED'){return 'ERR_UPGRADE_NOT_PREPARED'}
  if($phase -eq 'VERIFY'){return 'ERR_UPGRADE_NOT_CONFIRMED'}
  switch($phase){
   'LIST' {return 'ERR_UPGRADE_LIST_NOT_OPENED'}
   'DETAILS' {return 'ERR_UPGRADE_DETAILS_NOT_OPENED'}
   'BACK_DETAILS' {return 'ERR_UPGRADE_DETAILS_NOT_OPENED'}
   'EDIT' {return 'ERR_UPGRADE_TAB_NOT_OPENED'}
   'RETURN' {return 'ERR_UPGRADE_HOME_NOT_OPENED'}
  }
  return 'ERR_UPGRADE_SCREEN_NOT_CHANGED'
 }
 return 'WAIT'
}
function Next-Action($session,$observation,[long]$now){
 if(!$session.HomeEnabled -and !$session.DispatchEnabled -and !$session.DiscEnabled -and !$session.TrekkerEnabled){return 'ERR_NO_TASKS'}
 if($session.Actions -ge 80){return 'ERR_ACTION_LIMIT'}
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
   if($now-$session.Last -lt $script:DispatchTimeout){return 'WAIT'}
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
    if($session.Entry -gt 0){if($now-$session.Last -lt $script:EntryTimeout){return 'WAIT'};return 'ERR_ENTRY_NOT_CHANGED'}
    $session.Entry++;$action='OPEN'
   }
  }
  'CLAIM' {$session.Commission=$true;if($now-$session.Last -lt $script:ClaimRetry){return 'WAIT'};if($session.Claim -ge 2){return 'ERR_CLAIM_NOT_CHANGED'};$session.Claim++;$action='CLAIM'}
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
      if($kind -eq 'DISC' -and $screen -eq 'DETAILS'){
       $g.DrawString('Disc Level',$smallFont,[Drawing.Brushes]::White,800,30);$g.DrawString('1',$levelFont,[Drawing.Brushes]::White,990,5);$g.DrawString('/10',$smallFont,[Drawing.Brushes]::White,1045,35)
      }else{if($kind -eq 'TREKKER' -and $screen -eq 'DETAILS'){
        $g.FillRectangle([Drawing.Brushes]::White,985,145,40,65);$g.DrawString('1',$levelFont,[Drawing.Brushes]::White,1040,150);$g.DrawString('/10',$smallFont,[Drawing.Brushes]::White,1080,180)
       }else{$g.DrawString(($kind+' Level'),$smallFont,[Drawing.Brushes]::White,800,165);$g.DrawString('1',$levelFont,[Drawing.Brushes]::White,990,135);$g.DrawString('/10',$smallFont,[Drawing.Brushes]::White,1045,165)}}
      if($screen -eq 'DETAILS'){
       if($kind -eq 'DISC'){$g.DrawString('Melody Skill',$smallFont,[Drawing.Brushes]::White,900,150);$g.DrawString('Upgrade',$smallFont,[Drawing.Brushes]::White,500,655)}
       else{$g.FillRectangle([Drawing.Brushes]::Turquoise,950,510,240,65);$g.DrawString('Upgrade',$smallFont,[Drawing.Brushes]::White,990,525)}
      }else{
       $g.DrawString('Upgrade 1 level',$smallFont,[Drawing.Brushes]::White,860,460)
       $g.DrawString('Upgrade to Max',$smallFont,[Drawing.Brushes]::White,1070,460)
       $priceY=510;$priceX=1090;$buttonY=555;if($kind -eq 'TREKKER'){$priceY=550;$priceX=1110;$buttonY=600}
       $g.DrawString('0',$smallFont,[Drawing.Brushes]::White,$priceX,$priceY)
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
     if($kind -eq 'DISC' -and $screen -eq 'DETAILS'){
      $bg=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(30,50,100))
      try{
       $g.FillRectangle($bg,975,0,160,85);$g.DrawString('10',$levelFont,[Drawing.Brushes]::White,990,5);$g.DrawString('/10',$smallFont,[Drawing.Brushes]::White,1070,35)
       $g.FillRectangle($bg,450,630,220,70);$g.DrawString('Harmony Skill',$smallFont,[Drawing.Brushes]::White,900,390)
       $g.Flush([Drawing.Drawing2D.FlushIntention]::Sync)
       $capped=Read-Observation $bitmap
       Check ($capped.State -eq 'DISC_DETAILS' -and $capped.Level -eq 10) ('Actual-layout 10/10 without full-frame Upgrade tab was not recognized: '+$capped.State+'/'+$capped.Level)
      }finally{$bg.Dispose()}
     }
     if($screen -eq 'EDIT'){
      Check ($o.Cost -eq 0) ('Editor cost OCR missing: '+$o.Cost)
      foreach($costValue in @(100,0)){
       $bg=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(30,50,100))
       try{
        $g.FillRectangle($bg,920,90,180,140);$g.DrawString('2',$levelFont,[Drawing.Brushes]::White,990,135)
        $g.FillRectangle($bg,1050,($priceY-10),150,48)
        $ink=[Drawing.Brushes]::White
        if($costValue -eq 0){$g.FillRectangle([Drawing.Brushes]::White,($priceX-10),$priceY,90,42);$ink=[Drawing.Brushes]::DarkBlue}
        $g.DrawString([string]$costValue,$smallFont,$ink,$priceX,$priceY);$g.Flush([Drawing.Drawing2D.FlushIntention]::Sync)
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
  Check ((Next-Action $session ([pscustomobject]@{State='DISC_DETAILS';Level=90}) 3400) -eq 'BACK_LIST') 'High-level fallback candidate was upgraded'
  Check ($session.UpgradeCommits -eq 0) 'Unvalidated card spent currency'
  foreach($kind in @('DISC','TREKKER')){
   $mask=4;if($kind -eq 'TREKKER'){$mask=8}
   $session=New-Session $mask;$null=Next-Action $session $upgradeHome 1000
   $list=[pscustomobject]@{State=($kind+'_LIST');Cards=@()}
   $null=Next-Action $session $list 2200;$firstX=$session.TargetX
   Check ((Next-Action $session ([pscustomobject]@{State=($kind+'_DETAILS');Level=90}) 3400) -eq 'BACK_LIST') 'Level 90 was treated as eligible'
   Check ($session.UpgradeCommits -eq 0) 'Capped item spent materials'
   Check ((Next-Action $session $list 4600) -eq 'SELECT_LOW') 'Did not inspect the next card'
   Check ($session.TargetX -gt $firstX -and $session.TriedCards.Count -eq 2) 'Capped first card was selected repeatedly'
   Check ((Next-Action $session ([pscustomobject]@{State=($kind+'_DETAILS');Level=1;X=.43;Y=.92}) 5800) -eq 'OPEN_UPGRADE') 'Did not continue on low-level second card'
   $session.UpgradePhase='LIST';$session.TriedCards=@(0..9);if($kind -eq 'TREKKER'){$session.TriedCards=@(0..11)}
   Check ((Next-Action $session $list 8000) -eq 'ERR_LOW_LEVEL_NOT_FOUND') 'Visible inventory search did not stop at its bound'
  }
  foreach($kind in @('DISC','TREKKER')){
   $mask=4;if($kind -eq 'TREKKER'){$mask=8}
   $session=New-Session $mask;$null=Next-Action $session $upgradeHome 1000
   $list=[pscustomobject]@{State=($kind+'_LIST');Cards=@()};$null=Next-Action $session $list 2200
   $detail=[pscustomobject]@{State=($kind+'_DETAILS');Level=1;X=.435;Y=.923}
   Check ((Next-Action $session $detail 3400) -eq 'OPEN_UPGRADE') 'Initial upgrade navigation missing'
   Check ((Next-Action $session $detail 4500) -eq 'WAIT') 'Navigation retried too soon'
   Check ((Next-Action $session $detail 5400) -eq 'REOPEN_UPGRADE') 'Missed upgrade navigation was not retried'
   Check ($session.UpgradeOpenAttempts -eq 2 -and $session.UpgradeCommits -eq 0) 'Navigation retry spent resources'
   Check ((Next-Action $session $detail 7400) -eq 'WAIT') 'Navigation was retried a third time'
   $edit=[pscustomobject]@{State=($kind+'_EDIT');Level=1;Cost=0;OneX=.72;OneY=.67;CommitX=.8;CommitY=.8}
   Check ((Next-Action $session $edit 8500) -eq 'PREPARE_LEVEL') 'Navigation retry did not continue into material selection'
   $edit.Level=2;$edit.Cost=100
   Check ((Next-Action $session $edit 10000) -eq 'COMMIT_UPGRADE') 'Retry flow did not send its one upgrade'
   Check ((Next-Action $session $edit 12000) -eq 'WAIT') 'Navigation policy retried the actual spend'
   Check ($session.UpgradeCommits -eq 1) 'Actual spend was duplicated after navigation retry'
   $session=New-Session $mask;$session.Actions=3;$session.UpgradeKind=$kind;$session.UpgradePhase='EDIT';$session.BeforeLevel=1;$session.UpgradeAt=1000;$session.Last=1000;$session.UpgradeOpenAttempts=2
   Check ((Next-Action $session $detail 11000) -eq 'ERR_UPGRADE_TAB_NOT_OPENED') 'Missing tab did not identify its failed stage'
   $session.UpgradeAt=11000;$session.Last=11000;$session.UpgradeOpenAttempts=1;$detail.Level=10
   Check ((Next-Action $session $detail 13000) -eq 'ERR_UPGRADE_SELECTION_CHANGED') 'Navigation retry accepted a changed candidate'
  }
  $session=New-Session 4;$session.Actions=5;$session.UpgradeKind='DISC';$session.UpgradePhase='VERIFY';$session.UpgradeAt=1000;$session.BeforeLevel=1;$session.TargetLevel=2;$session.UpgradeCommits=1
  Check ((Next-Action $session ([pscustomobject]@{State='DISC_EDIT';Level=2;Cost=100}) 11000) -eq 'ERR_UPGRADE_NOT_CONFIRMED') 'Unconfirmed spend was retried or accepted'
  Check ($session.UpgradeCommits -eq 1) 'Failed upgrade spent twice'
  $session=New-Session 4;$session.Actions=3;$session.UpgradeKind='DISC';$session.UpgradePhase='EDIT';$session.BeforeLevel=1
  Check ((Next-Action $session ([pscustomobject]@{State='DISC_EDIT';Level=3;Cost=500}) 2200) -eq 'ERR_UPGRADE_EXISTING_SELECTION') 'Existing material selection was overwritten'
  $session=New-Session 588;$session.DiscDone=$true;$session.TrekkerDone=$true;$session.Interact=1;$session.Actions=15
  Check ((Next-Action $session $upgradeHome 20000) -eq 'OPEN') 'Completed upgrades did not hand off to Commission'
  $session=New-Session 12;$session.DiscDone=$true
  Check ((Next-Action $session $upgradeHome 2000) -eq 'ENTER_TREKKER') 'Disc did not hand off to Trekker'
  # Missing ALL OCR labels must still allow navigation in established Disc
  # context, using the actual control layout. It must never authorize a spend.
  function Paint-Region($brush,[double]$x,[double]$y,[double]$w,[double]$h){$g.FillRectangle($brush,[single]($x*$bitmap.Width),[single]($y*$bitmap.Height),[single]($w*$bitmap.Width),[single]($h*$bitmap.Height))}
  function Paint-DiscLayout([bool]$editor){
   $g.Clear([Drawing.Color]::FromArgb(30,50,100))
   Paint-Region ([Drawing.Brushes]::DarkBlue) .0 .03 .27 .06
   Paint-Region ([Drawing.Brushes]::White) .285 .045 .019 .03
   Paint-Region ([Drawing.Brushes]::WhiteSmoke) .19 .89 .62 .064
   if(!$editor){
    Paint-Region ([Drawing.Brushes]::DarkBlue) .20 .89 .17 .064
    Paint-Region ([Drawing.Brushes]::DarkTurquoise) .70 .20 .25 .035
    Paint-Region ([Drawing.Brushes]::DarkTurquoise) .70 .505 .25 .035
   }else{
    Paint-Region ([Drawing.Brushes]::DarkBlue) .36 .89 .15 .064
    Paint-Region ([Drawing.Brushes]::White) .667 .63 .105 .052
    Paint-Region ([Drawing.Brushes]::White) .80 .63 .105 .052
    Paint-Region ([Drawing.Brushes]::DarkTurquoise) .715 .73 .14 .066
   }
   $g.Flush([Drawing.Drawing2D.FlushIntention]::Sync)
  }
  Paint-DiscLayout $false
  $session=New-Session 4;$session.Actions=2;$session.UpgradeKind='DISC';$session.UpgradePhase='DETAILS'
  $o=Read-Observation $bitmap $session
  Check ($o.State -eq 'DISC_DETAILS' -and $o.Layout -and $o.Level -eq 0) 'Disc layout did not recover a text-free detail screen'
  Check ((Next-Action $session $o 3000) -eq 'OPEN_UPGRADE') 'Unknown detail counter blocked harmless editor navigation'
  Check ($session.UpgradeCommits -eq 0 -and $session.BeforeLevel -eq 0) 'Layout navigation authorized an upgrade spend'
  $wrong=New-Session 8;$wrong.UpgradeKind='TREKKER';$wrong.UpgradePhase='DETAILS'
  Check ($null -eq (Disc-Layout $bitmap $wrong)) 'Disc layout fallback ran in unrelated task context'
  Check ($null -eq (Disc-Layout $bitmap $null)) 'Disc layout fallback ran without an established candidate'
  Paint-Region ([Drawing.Brushes]::DarkBlue) .70 .50 .25 .06
  Check ($null -eq (Disc-Layout $bitmap $session)) 'Partial layout was accepted'
  # Regression uses the exact boolean vector reported by the owner on
  # Details: header/house and tabs agree, both skill colors fail, white skill
  # panels also trigger OnePill/MaxPill. Those do not make this an editor.
  $reported=[pscustomobject]@{Header=$true;House=$true;DetailsActive=$true;DetailsInactive=$false;UpgradeActive=$false;UpgradeInactive=$true;Melody=$false;Harmony=$false;OnePill=$true;MaxPill=$true;Commit=$false}
  $recovered=Disc-Layout-FromChecks $reported $session $true
  Check ($recovered.State -eq 'DISC_DETAILS') 'Owner-reported Details flags were rejected'
  Check ($null -eq (Disc-Layout-FromChecks $reported $session $false)) 'Missing caption and missing skill anchors were accepted'
  Check ($null -eq (Disc-Layout-FromChecks $reported $wrong $true)) 'Caption recovery escaped Disc task context'
  $reported.UpgradeActive=$true
  Check ($null -eq (Disc-Layout-FromChecks $reported $session $true)) 'Conflicting active tabs were accepted'
  $reported.UpgradeActive=$false;$reported.Commit=$true
  Check ($null -eq (Disc-Layout-FromChecks $reported $session $true)) 'Details/editor conflict was accepted'
  Paint-DiscLayout $false
  Paint-Region ([Drawing.Brushes]::DarkBlue) .70 .18 .25 .06
  Paint-Region ([Drawing.Brushes]::DarkBlue) .70 .50 .25 .06
  Paint-Region ([Drawing.Brushes]::White) .70 .62 .25 .07
  $g.DrawString('Disc',$font,[Drawing.Brushes]::White,[single](.12*$bitmap.Width),[single](.03*$bitmap.Height));$g.Flush([Drawing.Drawing2D.FlushIntention]::Sync)
  $session=New-Session 4;$session.Actions=2;$session.UpgradeKind='DISC';$session.UpgradePhase='DETAILS'
  $o=Read-Observation $bitmap $session
  Check ($o.State -eq 'DISC_DETAILS' -and $o.Layout) 'Real OCR caption plus owner-matching layout failed'
  Check ((Next-Action $session $o 3000) -eq 'OPEN_UPGRADE') 'Owner-matching Details did not advance into editor'
  Check ($session.UpgradeCommits -eq 0) 'Caption recovery sent a spend'
  Paint-DiscLayout $true
  $o=Read-Observation $bitmap $session
  Check ($o.State -eq 'DISC_EDIT' -and $o.Layout -and $o.Level -eq 0) 'Text-free editor layout was not identified'
  Check ((Next-Action $session $o 5000) -eq 'ERR_UPGRADE_LEVEL_UNREADABLE') 'Unreadable editor level allowed resource selection'
  Check ($session.UpgradeCommits -eq 0) 'Layout-only editor allowed spending'
  $counter=[Drawing.Font]::new('Segoe UI',56,[Drawing.FontStyle]::Bold,[Drawing.GraphicsUnit]::Pixel)
  $price=[Drawing.Font]::new('Segoe UI',24,[Drawing.FontStyle]::Bold,[Drawing.GraphicsUnit]::Pixel)
  try{
   $g.DrawString('1',$counter,[Drawing.Brushes]::Gold,[single](.74*$bitmap.Width),[single](.16*$bitmap.Height))
   $g.DrawString('0',$price,[Drawing.Brushes]::White,[single](.845*$bitmap.Width),[single](.69*$bitmap.Height));$g.Flush([Drawing.Drawing2D.FlushIntention]::Sync)
   $session=New-Session 4;$session.Actions=3;$session.UpgradeKind='DISC';$session.UpgradePhase='EDIT'
   $o=Read-Observation $bitmap $session
   Check ($o.State -eq 'DISC_EDIT' -and $o.Level -eq 1 -and $o.Cost -eq 0) ('Layout editor numeric validation failed: '+$o.State+'/'+$o.Level+'/'+$o.Cost)
   Check ((Next-Action $session $o 5000) -eq 'PREPARE_LEVEL') 'Validated editor did not continue'
   Check ($session.BeforeLevel -eq 1 -and $session.UpgradeCommits -eq 0) 'Editor baseline was not established before spending'
   $dim=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(170,0,0,0))
   try{$g.FillRectangle($dim,0,0,$bitmap.Width,$bitmap.Height);$g.Flush([Drawing.Drawing2D.FlushIntention]::Sync)}finally{$dim.Dispose()}
   Check ($null -eq (Disc-Layout $bitmap $session)) 'Dimmed/modal-covered layout was accepted'
  }finally{$counter.Dispose();$price.Dispose()}
  # Actual cost control: a taller currency icon precedes small digits on a
  # slightly tilted white card. Never confuse the icon or an empty crop with 0.
  $bubbleFont=[Drawing.Font]::new('Segoe UI',18,[Drawing.FontStyle]::Bold,[Drawing.GraphicsUnit]::Pixel)
  try{
   foreach($angle in @(-8,0,8)){
    foreach($item in @(@('0',0),@('100',100),@('250',250),@('2,860',2860))){
     $g.Clear([Drawing.Color]::FromArgb(60,95,155));$saved=$g.Save()
     try{
      $g.TranslateTransform([single](.817*$bitmap.Width),[single](.69*$bitmap.Height));$g.RotateTransform([single]$angle)
      $g.FillRectangle([Drawing.Brushes]::White,0,0,180,34)
      $g.FillRectangle([Drawing.Brushes]::DarkSlateBlue,8,1,18,29)
      $g.FillRectangle([Drawing.Brushes]::White,12,5,3,20);$g.FillRectangle([Drawing.Brushes]::White,17,10,3,10)
      $g.DrawString($item[0],$bubbleFont,[Drawing.Brushes]::DarkSlateBlue,38,4)
     }finally{$g.Restore($saved)}
     $g.Flush([Drawing.Drawing2D.FlushIntention]::Sync)
     $readCost=Read-Disc-Cost $bitmap
     Check ($readCost -eq $item[1] -or ($item[1] -eq 250 -and $readCost -eq -1)) ('Tilted icon-bearing cost failed at '+$angle+': '+$item[0]+' -> '+$readCost)
    }
   }
   $g.Clear([Drawing.Color]::FromArgb(60,95,155));$g.Flush([Drawing.Drawing2D.FlushIntention]::Sync)
   Check ((Read-Disc-Cost $bitmap) -eq -1) 'Missing cost pixels were treated as zero'
  }finally{$bubbleFont.Dispose()}
  # Post-upgrade Power Up result: close first, validate committed editor,
  # then return home. The same full-width result layout applies to both tasks.
  function Paint-PowerUp {
   $g.Clear([Drawing.Color]::FromArgb(30,50,100))
   Paint-Region ([Drawing.Brushes]::DarkTurquoise) 0 .35 1 .06
   Paint-Region ([Drawing.Brushes]::WhiteSmoke) 0 .41 1 .25
   $g.DrawString('Power Up!',$font,[Drawing.Brushes]::White,[single](.43*$bitmap.Width),[single](.33*$bitmap.Height))
   $g.Flush([Drawing.Drawing2D.FlushIntention]::Sync)
  }
  Paint-PowerUp
  foreach($kind in @('DISC','TREKKER')){
   $mask=4;if($kind -eq 'TREKKER'){$mask=8}
   $session=New-Session $mask;$session.Actions=5;$session.UpgradeKind=$kind;$session.UpgradePhase='VERIFY';$session.UpgradeCommits=1;$session.TargetLevel=2;$session.BeforeLevel=1;$session.Last=1000;$session.UpgradeAt=1000
   $o=Read-Observation $bitmap $session
   Check ($o.State -eq 'POWER_UP') 'Committed upgrade result was not recognized'
   Check ((Next-Action $session $o 2200) -eq 'CLOSE_POWER_UP') 'Power Up dismissal missing'
   Check ([Math]::Abs($session.TargetX-.50) -lt .001 -and [Math]::Abs($session.TargetY-.85) -lt .001) 'Power Up click was not moved to the owner-confirmed dark backdrop'
   Check ($session.UpgradeCommits -eq 1 -and !$session.DiscDone -and !$session.TrekkerDone -and $session.UpgradePhase -eq 'VERIFY') 'Overlay alone proved completion or sent another spend'
   Check ((Next-Action $session $o 3000) -eq 'WAIT') 'Power Up dismissal did not settle'
   $editor=[pscustomobject]@{State=($kind+'_EDIT');Level=2;Cost=0}
   Check ((Next-Action $session $editor 4000) -eq 'RETURN_HOME') 'Closed result did not validate the editor and return home'
   Check ((Next-Action $session $upgradeHome 5200) -eq 'WAIT') 'Return-home transition was missed'
   Check ((Next-Action $session $upgradeHome 6400) -eq 'DONE') 'Upgrade-only result flow did not complete at home'
   Check ($session.UpgradeCommits -eq 1) 'Result cleanup repeated upgrade spending'
  }
  $session=New-Session 4;$session.Actions=5;$session.UpgradeKind='DISC';$session.UpgradePhase='VERIFY';$session.UpgradeCommits=1;$session.TargetLevel=2;$session.Last=1000;$session.UpgradeAt=1000
  $o=Read-Observation $bitmap $session;$null=Next-Action $session $o 2200
  Check ((Next-Action $session $o 4300) -eq 'CLOSE_POWER_UP') 'Still-recognized result did not receive its one dismissal retry'
  Check ((Next-Action $session $o 6500) -eq 'WAIT') 'Result dismissal was retried beyond its bound'
  Check ((Next-Action $session $o 12000) -eq 'ERR_UPGRADE_OVERLAY_NOT_CLOSED') 'Persistent result did not stop'
  Check ($session.UpgradeOverlayCloses -eq 2 -and $session.UpgradeCommits -eq 1) 'Result click or spend limits were violated'
  $session.UpgradePhase='PREPARED'
  Check ($null -eq (Upgrade-Result-Overlay $bitmap $session)) 'Result handler ran before a committed upgrade'
  $session.UpgradePhase='VERIFY';$session.UpgradeCommits=0
  Check ($null -eq (Upgrade-Result-Overlay $bitmap $session)) 'Unrelated overlay was dismissed without a spend from this run'
  Check ($null -eq (Upgrade-Result-Overlay $bitmap $null)) 'Result handler ran without task context'
  $session.UpgradeCommits=1
  Paint-Region ([Drawing.Brushes]::DarkBlue) .82 .35 .18 .06
  Check ($null -eq (Upgrade-Result-Overlay $bitmap $session)) 'Incomplete Power Up layout was accepted'
  # Trekker has distinct Details/Training headers, badge/counter regions,
  # a right-side entry button, lower price and no Disc bottom Upgrade tab.
  $tkFont=[Drawing.Font]::new('Segoe UI',56,[Drawing.FontStyle]::Bold,[Drawing.GraphicsUnit]::Pixel)
  $tkSmall=[Drawing.Font]::new('Segoe UI',18,[Drawing.FontStyle]::Bold,[Drawing.GraphicsUnit]::Pixel)
  $tkHeader=[Drawing.Font]::new('Segoe UI',24,[Drawing.FontStyle]::Regular,[Drawing.GraphicsUnit]::Pixel)
  function Paint-Trekker([bool]$training,[int]$level,[int]$cost){
   $g.Clear([Drawing.Color]::FromArgb(30,50,100))
   Paint-Region ([Drawing.Brushes]::DarkBlue) 0 .03 .27 .06
   Paint-Region ([Drawing.Brushes]::White) .285 .045 .019 .03
   if(!$training){
    $g.DrawString('Trekker Details',$tkHeader,[Drawing.Brushes]::White,[single](.12*$bitmap.Width),[single](.03*$bitmap.Height))
    Paint-Region ([Drawing.Brushes]::WhiteSmoke) .05 .89 .90 .064
    Paint-Region ([Drawing.Brushes]::DarkBlue) .055 .89 .15 .064
    foreach($y in @(.382,.423,.466)){Paint-Region ([Drawing.Brushes]::WhiteSmoke) .75 $y .225 .032}
    Paint-Region ([Drawing.Brushes]::DarkTurquoise) .796 .697 .137 .066
    Paint-Region ([Drawing.Brushes]::White) .772 .208 .030 .075
    $g.DrawString('8',$tkSmall,[Drawing.Brushes]::DarkBlue,[single](.776*$bitmap.Width),[single](.22*$bitmap.Height))
    $g.DrawString([string]$level,$tkFont,[Drawing.Brushes]::Gold,[single](.815*$bitmap.Width),[single](.21*$bitmap.Height))
   }else{
    $g.DrawString('Trekker Training',$tkHeader,[Drawing.Brushes]::White,[single](.12*$bitmap.Width),[single](.03*$bitmap.Height))
    Paint-Region ([Drawing.Brushes]::WhiteSmoke) .684 .689 .104 .051
    Paint-Region ([Drawing.Brushes]::WhiteSmoke) .818 .689 .104 .051
    Paint-Region ([Drawing.Brushes]::DarkTurquoise) .735 .794 .138 .064
    Paint-Region ([Drawing.Brushes]::White) .70 .20 .03 .08
    $g.DrawString('8',$tkSmall,[Drawing.Brushes]::DarkBlue,[single](.705*$bitmap.Width),[single](.22*$bitmap.Height))
    $g.DrawString([string]$level,$tkFont,[Drawing.Brushes]::Gold,[single](.760*$bitmap.Width),[single](.20*$bitmap.Height))
    Paint-Region ([Drawing.Brushes]::White) .838 .76 .09 .045
    Paint-Region ([Drawing.Brushes]::DarkSlateBlue) .842 .762 .014 .034
    $g.DrawString([string]$cost,$tkSmall,[Drawing.Brushes]::DarkSlateBlue,[single](.866*$bitmap.Width),[single](.764*$bitmap.Height))
   }
   $g.Flush([Drawing.Drawing2D.FlushIntention]::Sync)
  }
  try{
   $session=New-Session 8;$session.Actions=2;$session.UpgradeKind='TREKKER';$session.UpgradePhase='DETAILS'
   Paint-Trekker $false 1 0
   $o=Read-Observation $bitmap $session
   if($o.State -ne 'TREKKER_DETAILS' -or $o.Level -ne 1){
    $facts=Trekker-Layout-Checks $bitmap;foreach($property in $facts.PSObject.Properties){[Console]::Out.WriteLine(('TREKKER FIXTURE {0}: {1}' -f $property.Name,$property.Value))}
    $fixtureRead=Read-Image $bitmap
    try{foreach($line in $fixtureRead.Lines){[Console]::Out.WriteLine(('TREKKER FIXTURE TEXT: {0}' -f $line.Text))}}finally{$fixtureRead.Bitmap.Dispose()}
   }
   Check ($o.State -eq 'TREKKER_DETAILS' -and $o.Layout -and $o.Level -eq 1) ('Trekker Details layout/level failed: '+$o.State+'/'+$o.Level)
   Check ((Next-Action $session $o 2200) -eq 'OPEN_UPGRADE') 'Trekker Details did not open Training'
   Check ([Math]::Abs($session.TargetX-.865) -lt .001 -and [Math]::Abs($session.TargetY-.73) -lt .001) 'Disc tab coordinates were used for Trekker'
   $discContext=New-Session 4;$discContext.UpgradeKind='DISC';$discContext.UpgradePhase='DETAILS'
   $read=Read-Image $bitmap
   try{Check ($null -eq (Trekker-Layout $bitmap $discContext $read.Lines)) 'Trekker layout ran in Disc context';Check ($null -eq (Trekker-Layout $bitmap $null $read.Lines)) 'Trekker layout ran without context'}finally{$read.Bitmap.Dispose()}
   Paint-Trekker $true 1 0
   $o=Read-Observation $bitmap $session
   Check ($o.State -eq 'TREKKER_EDIT' -and $o.Layout -and $o.Level -eq 1 -and $o.Cost -eq 0) ('Trekker Training numeric layout failed: '+$o.State+'/'+$o.Level+'/'+$o.Cost)
   Check ((Next-Action $session $o 3400) -eq 'PREPARE_LEVEL') 'Training did not choose Upgrade 1 level'
   Check ([Math]::Abs($session.TargetY-.715) -lt .001 -and $session.UpgradeCommits -eq 0) 'Training material-selection position or spend guard failed'
   Paint-Trekker $true 2 100
   $o=Read-Observation $bitmap $session
   Check ($o.Level -eq 2 -and $o.Cost -eq 100) 'Training preview was not read'
   Check ((Next-Action $session $o 4600) -eq 'COMMIT_UPGRADE') 'Training did not send its one confirmation'
   Check ([Math]::Abs($session.TargetY-.826) -lt .001) 'Disc confirmation position was reused for Training'
   Paint-PowerUp;$o=Read-Observation $bitmap $session
   Check ((Next-Action $session $o 5800) -eq 'CLOSE_POWER_UP') 'Trekker Power Up result was not closed'
   Paint-Trekker $true 2 0;$o=Read-Observation $bitmap $session
   Check ((Next-Action $session $o 7000) -eq 'RETURN_HOME') 'Trekker result did not verify level/cost before Home'
   Check ((Next-Action $session $upgradeHome 8200) -eq 'WAIT') 'Training Home transition was missed'
   Check ((Next-Action $session $upgradeHome 9400) -eq 'DONE') 'Trekker-only flow did not finish'
   Check ($session.TrekkerDone -and !$session.DiscDone -and $session.UpgradeCommits -eq 1) 'Trekker flow upgraded another task or repeated spending'
   # Capped only becomes readable in Training: back to Details, then List.
   $session=New-Session 8;$session.Actions=4;$session.UpgradeKind='TREKKER';$session.UpgradePhase='EDIT';$session.TriedCards=@(0)
   Check ((Next-Action $session ([pscustomobject]@{State='TREKKER_EDIT';Level=90;Cost=0}) 2000) -eq 'BACK_LIST') 'Capped Training was not rejected'
   Check ($session.UpgradePhase -eq 'BACK_DETAILS') 'Training skipped its intermediate Details screen'
   Check ((Next-Action $session ([pscustomobject]@{State='TREKKER_DETAILS';Level=90}) 3200) -eq 'BACK_LIST') 'Second back to List was missing'
   Check ((Next-Action $session ([pscustomobject]@{State='TREKKER_LIST';Cards=@()}) 4400) -eq 'SELECT_LOW') 'Training rejection did not continue candidate search'
   Check ($session.TargetX -gt .125 -and $session.UpgradeCommits -eq 0) 'Capped Training candidate repeated or spent materials'
   # Training prices have their own icon and vertical offset.
   foreach($angle in @(-8,0,8)){
    foreach($item in @(@('0',0),@('100',100),@('250',250),@('2,860',2860))){
     $g.Clear([Drawing.Color]::FromArgb(60,95,155));$saved=$g.Save()
     try{
      $g.TranslateTransform([single](.834*$bitmap.Width),[single](.755*$bitmap.Height));$g.RotateTransform([single]$angle)
      $g.FillRectangle([Drawing.Brushes]::White,0,0,180,34);$g.FillRectangle([Drawing.Brushes]::DarkSlateBlue,8,1,18,29)
      $g.FillRectangle([Drawing.Brushes]::White,12,5,3,20);$g.DrawString($item[0],$tkSmall,[Drawing.Brushes]::DarkSlateBlue,38,4)
     }finally{$g.Restore($saved)}
     $g.Flush([Drawing.Drawing2D.FlushIntention]::Sync)
     $cost=Read-Trekker-Cost $bitmap
     Check ($cost -eq $item[1] -or ($item[1] -eq 250 -and $cost -eq -1)) ('Training tilted cost failed: '+$angle+'/'+$item[0]+' -> '+$cost)
    }
   }
   $g.Clear([Drawing.Color]::FromArgb(60,95,155));$g.Flush([Drawing.Drawing2D.FlushIntention]::Sync)
   Check ((Read-Trekker-Cost $bitmap) -eq -1) 'Missing Training cost was treated as zero'
   Paint-Trekker $true 1 0
   $read=Read-Image $bitmap
   try{
    $dim=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(170,0,0,0))
    try{$g.FillRectangle($dim,0,0,$bitmap.Width,$bitmap.Height);$g.Flush([Drawing.Drawing2D.FlushIntention]::Sync)}finally{$dim.Dispose()}
    $session.UpgradePhase='EDIT'
    Check ($null -eq (Trekker-Layout $bitmap $session $read.Lines)) 'Dimmed Training layout was accepted'
   }finally{$read.Bitmap.Dispose()}
  }finally{$tkFont.Dispose();$tkSmall.Dispose();$tkHeader.Dispose()}
  # Exact owner observation: prepared Disc 2 -> green 3, cost 250 was read
  # correctly but the green counter returned 0. Do not bypass either guard.
  $previewFont=[Drawing.Font]::new('Segoe UI',56,[Drawing.FontStyle]::Bold,[Drawing.GraphicsUnit]::Pixel)
  $previewPrice=[Drawing.Font]::new('Segoe UI',18,[Drawing.FontStyle]::Bold,[Drawing.GraphicsUnit]::Pixel)
  $previewInk=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(145,185,75))
  try{
   Paint-DiscLayout $true
   $g.DrawString('3',$previewFont,$previewInk,[single](.74*$bitmap.Width),[single](.16*$bitmap.Height))
   Paint-Region $previewInk .62 .256 .27 .020
   $g.DrawString('910/1150',$previewPrice,[Drawing.Brushes]::White,[single](.76*$bitmap.Width),[single](.257*$bitmap.Height))
   $saved=$g.Save()
   try{
    $g.TranslateTransform([single](.817*$bitmap.Width),[single](.69*$bitmap.Height));$g.RotateTransform(8)
    $g.FillRectangle([Drawing.Brushes]::White,0,0,180,34);$g.FillRectangle([Drawing.Brushes]::DarkSlateBlue,8,1,18,29)
    $priceInk=[Drawing.SolidBrush]::new([Drawing.Color]::FromArgb(40,65,110));try{$g.DrawString('250',$previewPrice,$priceInk,38,4)}finally{$priceInk.Dispose()}
   }finally{$g.Restore($saved)}
   $g.Flush([Drawing.Drawing2D.FlushIntention]::Sync)
   Check (([CommissionDesktop]::Number($bitmap,.72,.12,.14,.20,$true,.032,.10)) -eq 3) 'Green preview counter was omitted or XP text/bar was parsed as the level'
   $session=New-Session 4;$session.Actions=4;$session.UpgradeKind='DISC';$session.UpgradePhase='PREPARED';$session.BeforeLevel=2;$session.Last=1000;$session.UpgradeAt=1000
   $o=Read-Observation $bitmap $session
   Check ($o.State -eq 'DISC_EDIT' -and $o.Level -eq 3 -and $o.Cost -eq 250) ('Owner-matching green/cost scene failed: '+$o.State+'/'+$o.Level+'/'+$o.Cost)
   Check ((Next-Action $session $o 2200) -eq 'COMMIT_UPGRADE') 'Verified green preview 2->3 / cost 250 did not continue'
   Check ((Next-Action $session $o 4000) -eq 'WAIT') 'Uncleared preview sent a repeated upgrade'
   Check ($session.UpgradeCommits -eq 1) 'Green preview caused duplicate confirmation'
   foreach($bad in @([pscustomobject]@{State='DISC_EDIT';Level=3;Cost=-1},[pscustomobject]@{State='DISC_EDIT';Level=0;Cost=250},[pscustomobject]@{State='DISC_EDIT';Level=3;Cost=0},[pscustomobject]@{State='DISC_EDIT';Level=2;Cost=250})){
    $session=New-Session 4;$session.Actions=4;$session.UpgradeKind='DISC';$session.UpgradePhase='PREPARED';$session.BeforeLevel=2;$session.Last=1000;$session.UpgradeAt=1000
    Check ((Next-Action $session $bad 2200) -eq 'WAIT') 'Prepared-level/cost guard was weakened'
    Check ($session.UpgradeCommits -eq 0) 'Unverified preparation spent resources'
   }
   $g.Clear([Drawing.Color]::FromArgb(30,50,100));Paint-Region $previewInk .62 .256 .27 .020
   $g.Flush([Drawing.Drawing2D.FlushIntention]::Sync)
   Check (([CommissionDesktop]::Number($bitmap,.72,.12,.14,.20,$true,.032,.10)) -eq -1) 'Green XP bar with no level invented a counter'
   $g.Clear([Drawing.Color]::FromArgb(30,50,100))
   $g.DrawString('3',$previewFont,$previewInk,[single](.760*$bitmap.Width),[single](.20*$bitmap.Height));$g.Flush([Drawing.Drawing2D.FlushIntention]::Sync)
   $r=Trekker-Level-Region 'TREKKER_EDIT'
   Check (([CommissionDesktop]::Number($bitmap,$r.X,$r.Y,$r.W,$r.H,$true,.032,.10)) -eq 3) 'Green Training preview did not use its own counter region'
  }finally{$previewFont.Dispose();$previewPrice.Dispose();$previewInk.Dispose()}
  # Owner-selected hard cap: all levels 1..89 are eligible; 90 is skipped.
  $cardWords=@([pscustomobject]@{Text='Lv.10';X=.12;Y=.44;W=.03;H=.02},[pscustomobject]@{Text='Lv.35';X=.30;Y=.44;W=.03;H=.02},[pscustomobject]@{Text='Lv.89';X=.48;Y=.44;W=.03;H=.02},[pscustomobject]@{Text='Lv.90';X=.66;Y=.44;W=.03;H=.02})
  $eligible=@(Low-Cards $cardWords)
  Check ($eligible.Count -eq 3 -and $eligible[0].Level -eq 10 -and $eligible[2].Level -eq 89) 'Visible card filter did not apply the requested level-90 boundary'
  foreach($kind in @('DISC','TREKKER')){
   $mask=4;if($kind -eq 'TREKKER'){$mask=8}
   foreach($level in @(10,35,89)){
    $session=New-Session $mask;$session.Actions=2;$session.UpgradeKind=$kind;$session.UpgradePhase='DETAILS'
    Check ((Next-Action $session ([pscustomobject]@{State=($kind+'_DETAILS');Level=$level;X=.8;Y=.7}) 2200) -eq 'OPEN_UPGRADE') 'A level below 90 was rejected at details'
    $editor=[pscustomobject]@{State=($kind+'_EDIT');Level=$level;Cost=0;OneX=.7;OneY=.7}
    Check ((Next-Action $session $editor 3400) -eq 'PREPARE_LEVEL') 'A level below 90 was rejected at the editor'
    Check ($session.UpgradeCommits -eq 0) 'Eligibility check spent resources'
   }
   $session=New-Session $mask;$session.Actions=2;$session.UpgradeKind=$kind;$session.UpgradePhase='DETAILS'
   Check ((Next-Action $session ([pscustomobject]@{State=($kind+'_DETAILS');Level=90}) 2200) -eq 'BACK_LIST') 'Level 90 was not skipped at details'
   $session=New-Session $mask;$session.Actions=3;$session.UpgradeKind=$kind;$session.UpgradePhase='EDIT'
   Check ((Next-Action $session ([pscustomobject]@{State=($kind+'_EDIT');Level=90;Cost=0}) 2200) -eq 'BACK_LIST') 'Level 90 was not skipped before material selection'
   Check ($session.UpgradeCommits -eq 0) 'A level-90 candidate spent resources'
   $session=New-Session $mask;$session.Actions=4;$session.UpgradeKind=$kind;$session.UpgradePhase='PREPARED';$session.BeforeLevel=89
   $preview=[pscustomobject]@{State=($kind+'_EDIT');Level=90;Cost=250;CommitX=.8;CommitY=.8}
   Check ((Next-Action $session $preview 2200) -eq 'COMMIT_UPGRADE') 'An authorized 89 -> 90 upgrade was blocked'
   $preview.Cost=0
   Check ((Next-Action $session $preview 4000) -eq 'RETURN_HOME') 'Committed level 90 was incorrectly treated as a new candidate'
   Check ($session.UpgradeCommits -eq 1) '89 -> 90 upgrade was repeated'
  }
  # Two-digit current levels with nearby cap, yellow diamond, XP and badge:
  # only the current yellow/green digits belong to the focused region.
  $counterFont=[Drawing.Font]::new('Segoe UI',[single]($bitmap.Height*.057),[Drawing.FontStyle]::Bold,[Drawing.GraphicsUnit]::Pixel)
  $counterSmall=[Drawing.Font]::new('Segoe UI',18,[Drawing.FontStyle]::Bold,[Drawing.GraphicsUnit]::Pixel)
  try{
   foreach($state in @('DISC_EDIT','TREKKER_EDIT')){
    $r=Focused-Level-Region $state
    foreach($level in @(13,35,89,90)){
     $g.Clear([Drawing.Color]::FromArgb(30,50,100))
     $g.DrawString([string]$level,$counterFont,[Drawing.Brushes]::Gold,[single](($r.X+.002)*$bitmap.Width),[single](($r.Y+.005)*$bitmap.Height))
     $g.DrawString('/20',$counterSmall,[Drawing.Brushes]::White,[single](($r.X+$r.W+.002)*$bitmap.Width),[single](($r.Y+.035)*$bitmap.Height))
     Paint-Region ([Drawing.Brushes]::Gold) ($r.X+$r.W+.035) ($r.Y+.02) .015 .05
     $g.DrawString('780/2700',$counterSmall,[Drawing.Brushes]::White,[single](.76*$bitmap.Width),[single](.258*$bitmap.Height))
     $g.Flush([Drawing.Drawing2D.FlushIntention]::Sync)
     $actual=Read-Focused-Level $bitmap $state
     Check ($actual -eq $level) ('Focused current counter failed: '+$state+'/'+$level+' -> '+$actual+' '+$script:FocusedFacts)
    }
    $g.Clear([Drawing.Color]::FromArgb(30,50,100))
    $g.DrawString('/20',$counterSmall,[Drawing.Brushes]::White,[single](($r.X+$r.W+.002)*$bitmap.Width),[single](($r.Y+.035)*$bitmap.Height))
    Paint-Region ([Drawing.Brushes]::Gold) ($r.X+$r.W+.035) ($r.Y+.02) .015 .05
    $g.Flush([Drawing.Drawing2D.FlushIntention]::Sync)
    Check ((Read-Focused-Level $bitmap $state) -eq 0) 'Cap or diamond invented a current level'
   }
  }finally{$counterFont.Dispose();$counterSmall.Dispose()}
  $settle=New-Session 8
  $sample=[pscustomobject]@{State='TREKKER_EDIT';Level=2;Cost=0}
  Check (!(Observation-Ready $settle $sample 0)) 'First frame authorized input'
  Check (!(Observation-Ready $settle $sample 200)) 'Unsettled frame authorized input'
  Check ((Observation-Ready $settle $sample 500)) 'Stable editor never became ready'
  $sample.Level=0;$sample.Cost=300
  Check (!(Observation-Ready $settle $sample 1000)) 'Unreadable preview authorized spending'
  $sample.Level=3
  Check (!(Observation-Ready $settle $sample 1500)) 'New preview bypassed settling'
  Check ((Observation-Ready $settle $sample 2000)) 'Stable preview was blocked'
  $settle.UpgradePhase='VERIFY'
  Check (!(Observation-Ready $settle $sample 2500)) 'New phase reused stale frame'
  Check (!(Observation-Ready $settle ([pscustomobject]@{State='UNKNOWN'}) 3000)) 'Loading frame authorized input'
  $script:TransitionTimeout=30000
  $delayed=New-Session 8;$delayed.Actions=4;$delayed.UpgradeKind='TREKKER';$delayed.UpgradePhase='PREPARED';$delayed.BeforeLevel=2;$delayed.UpgradeAt=1000
  $sample.Level=0
  Check ((Next-Action $delayed $sample 16000) -eq 'WAIT') 'Lagging preview stopped at old deadline'
  $sample=[pscustomobject]@{State='TREKKER_EDIT';Level=3;Cost=300;CommitX=.8;CommitY=.8}
  Check ((Next-Action $delayed $sample 20000) -eq 'COMMIT_UPGRADE') 'Delayed preview did not commit'
  Check ((Next-Action $delayed $sample 25000) -eq 'WAIT') 'Pending spend was repeated'
  Check ($delayed.UpgradeCommits -eq 1) 'Lag caused repeated spending'
  $script:TransitionTimeout=10000
  Signal 'SELF_TEST_PASS'
 }finally{$g.Dispose();$bitmap.Dispose();if($font){$font.Dispose()}}
 exit 0
}
if($Inspect){
 [Console]::Out.WriteLine('Close any tool/error dialog first. Keep the game unobscured. No input is sent.')
 [Console]::Out.WriteLine('READ-ONLY INSPECTION: switch to the game within 8 seconds. No click or key will be sent. Keep this window open.')
 Start-Sleep -Seconds 8
 $snapshot=$null;$read=$null
 try{
  [CommissionDesktop]::InspectForeground();$snapshot=[CommissionDesktop]::Capture()
  $read=Read-Image $snapshot
  [Console]::Out.WriteLine(('SoraVynx {0} inspection v2 - labels and layout only; no account text or screenshot saved' -f $InspectKind))
  foreach($name in @('disc','melodyskill','harmonyskill','upgrade','trekkerlist','trekkerdetails','trekkertraining')){
   [Console]::Out.WriteLine(('Known label {0}: {1}' -f $name,@(Label $read.Lines $name 0 1 0 1).Count))
  }
  [Console]::Out.WriteLine(('Disc header caption: {0}' -f @(Label $read.Lines 'disc' 0 .30 0 .12).Count))
  $facts=Disc-Layout-Checks $snapshot
  if($InspectKind -eq 'TREKKER'){$facts=Trekker-Layout-Checks $snapshot}
  foreach($property in $facts.PSObject.Properties){[Console]::Out.WriteLine(('Layout {0}: {1}' -f $property.Name,$property.Value))}
  $context=New-Session 4;$context.UpgradeKind=$InspectKind;$context.UpgradePhase='EDIT'
  $levelVisible=[CommissionDesktop]::VisibleRegion(.75,.18,.065,.115)
  if($InspectKind -eq 'DISC'){$levelVisible=[CommissionDesktop]::VisibleRegion(.73,.16,.07,.115)}
  [Console]::Out.WriteLine(('Editor level region unobscured: {0}' -f $levelVisible))
  $observed=Read-Observation $snapshot $context
  [Console]::Out.WriteLine(('Recognized state: {0}; editor level (0 = not validated): {1}' -f $observed.State,$observed.Level))
  $focused=Read-Focused-Level $snapshot ($InspectKind+'_EDIT');[Console]::Out.WriteLine(('Focused current level (0 = not validated): {0}' -f $focused));[Console]::Out.WriteLine(('Focused numeric validation: {0}' -f $script:FocusedFacts))
  $editorCost=-1;if($observed.State -in @('DISC_EDIT','TREKKER_EDIT')){$editorCost=$observed.Cost};[Console]::Out.WriteLine(('Editor cost (-1 = not validated): {0}' -f $editorCost))
 }catch{[Console]::Out.WriteLine('READ_ONLY_INSPECTION_FAILED: keep the unobscured Unity game foreground and try again.')}finally{if($read){$read.Bitmap.Dispose()};if($snapshot){$snapshot.Dispose()}}
 exit 0
}
try {
 [CommissionDesktop]::Init($GameWindow,$GamePid);$session=New-Session $Tasks;$clock=[Diagnostics.Stopwatch]::StartNew();$script:TransitionTimeout=30000;$script:DispatchTimeout=30000;$script:NavigationRetry=6000;$script:EntryTimeout=30000;$script:ClaimRetry=6000;$script:OverlayRetry=3000;$lastProgress=0;Signal 'READY'
 while($clock.ElapsedMilliseconds -lt 360000){
  $bitmap=[CommissionDesktop]::Capture()
  try{$observation=Read-Observation $bitmap $session}finally{$bitmap.Dispose()}
  $now=$clock.ElapsedMilliseconds
  if(!(Observation-Ready $session $observation $now)){
   if($now-$lastProgress -ge $script:TransitionTimeout){
    if($observation.State -in @('DISC_EDIT','TREKKER_EDIT') -and $observation.Level -lt 1){Signal 'ERR_UPGRADE_LEVEL_UNREADABLE'}
    elseif($observation.State -in @('DISC_EDIT','TREKKER_EDIT') -and $observation.Cost -lt 0){Signal 'ERR_UPGRADE_COST_UNREADABLE'}
    else{Signal 'ERR_UNKNOWN_SCREEN'};exit 3
   }
   Start-Sleep -Milliseconds 450;continue
  }
  $action=Next-Action $session $observation $now
  Signal ('STATE_'+$observation.State)
  if($action -eq 'DONE' -or $action -eq 'NO_REWARDS'){Signal $action;exit 0}
  if($action.StartsWith('ERR_')){Signal $action;exit 3}
  if($action -eq 'WAIT'){Start-Sleep -Milliseconds 450;continue}
  $lastProgress=$now;$session.SettleKey=''
  Signal ('ACTION_'+$action)
  # The owner confirmed one touch on the displayed home character completes
  # interaction. Its center point is based on the supplied home layout; this
  # is not character detection. Require recognized HOME before sending it.
  $targetX=$observation.X;$targetY=$observation.Y
  if($action -eq 'INTERACT'){$targetX=0.50;$targetY=0.50}
  if($action -in @('ENTER_DISC','ENTER_TREKKER','SELECT_LOW','BACK_LIST','OPEN_UPGRADE','REOPEN_UPGRADE','PREPARE_LEVEL','COMMIT_UPGRADE','CLOSE_POWER_UP','RETURN_HOME')){$targetX=$session.TargetX;$targetY=$session.TargetY}
  [CommissionDesktop]::Act(($action -eq 'ESC'),$targetX,$targetY)
  Start-Sleep -Milliseconds 500
 }
 Signal 'ERR_TIMEOUT';exit 3
} catch {Signal 'ERR_CONTEXT_OR_CAPTURE';exit 3}
