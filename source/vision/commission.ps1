# SPDX-License-Identifier: AGPL-3.0-or-later
param([long]$GameWindow=0,[int]$GamePid=0,[switch]$SelfTest)
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
  $lines=@(foreach($line in $task.Result.Lines){
   $left=1e9;$top=1e9;$right=0;$bottom=0
   foreach($word in $line.Words){$b=$word.BoundingRect;$left=[Math]::Min($left,$b.X);$top=[Math]::Min($top,$b.Y);$right=[Math]::Max($right,$b.X+$b.Width);$bottom=[Math]::Max($bottom,$b.Y+$b.Height)}
   if($right -gt $left){[pscustomobject]@{Text=$line.Text;X=$left/$copy.Width;Y=$top/$copy.Height;W=($right-$left)/$copy.Width;H=($bottom-$top)/$copy.Height}}
  })
  $result=[pscustomobject]@{Lines=$lines;Bitmap=$copy};$copy=$null;return $result
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
function Observe($lines,$bitmap,[string]$promptTokens=''){
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
 $commissionHome=@($lines | Where-Object {(Normal $_.Text) -eq 'commission' -and $_.X -gt .7 -and $_.Y -lt .3})
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
  return Observe $read.Lines $read.Bitmap $tokens
 }finally{$read.Bitmap.Dispose()}
}
function New-Session {return @{Entry=0;Claim=0;Skip=0;Items=0;Again=0;Last=0;Unknown=0;Actions=0;Commission=$false}}
function Next-Action($session,$observation,[long]$now){
 if($session.Actions -ge 12){return 'ERR_ACTION_LIMIT'}
 switch($observation.State){
  'HOME' {if($session.Entry -gt 0){if($now-$session.Last -lt 3000){return 'WAIT'};return 'ERR_ENTRY_NOT_CHANGED'};$session.Entry++;$action='OPEN'}
  'CLAIM' {$session.Commission=$true;if($session.Again -gt 0){return 'DONE'};if($now-$session.Last -lt 1500){return 'WAIT'};if($session.Claim -ge 2){return 'ERR_CLAIM_NOT_CHANGED'};$session.Claim++;$action='CLAIM'}
  'STORY' {if(!$session.Commission){return 'ERR_NOT_COMMISSION'};if($now-$session.Last -lt 800){return 'WAIT'};if($session.Skip -ge 4){return 'ERR_STORY_LIMIT'};$session.Skip++;$action='ESC'}
  'ITEMS' {if(!$session.Commission){return 'ERR_NOT_COMMISSION'};if($now-$session.Last -lt 800){return 'WAIT'};if($session.Items -ge 4){return 'ERR_ITEMS_LIMIT'};$session.Items++;$action='CONTINUE'}
  'RESULT' {$session.Commission=$true;if($session.Again -gt 0){return 'DONE'};if($now-$session.Last -lt 800){return 'WAIT'};$session.Again++;$action='AGAIN'}
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
    'HOME' {$g.DrawString('Commission',$font,[Drawing.Brushes]::White,1010,110)}
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
  $session=New-Session;$story=[pscustomobject]@{State='STORY'};$items=[pscustomobject]@{State='ITEMS'};$result=[pscustomobject]@{State='RESULT'}
  Check ((Next-Action $session ([pscustomobject]@{State='CLAIM'}) 2000) -eq 'CLAIM') 'Claim missing'
  Check ((Next-Action $session $story 2300) -eq 'WAIT') 'Cooldown ignored'
  Check ((Next-Action $session $story 3000) -eq 'ESC') 'Story skip missing'
  Check ((Next-Action $session $items 4000) -eq 'CONTINUE') 'Overlay dismissal missing'
  Check ((Next-Action $session $result 5000) -eq 'AGAIN') 'Repeat missing'
  Check ((Next-Action $session $result 6000) -eq 'DONE') 'Repeat was duplicated'
  $session=New-Session;Check ((Next-Action $session $result 1000) -eq 'AGAIN') 'Resume at results failed'
  $session=New-Session;Check ((Next-Action $session ([pscustomobject]@{State='NO_REWARDS'}) 2000) -eq 'NO_REWARDS') 'Disabled claim was clicked'
  for($i=0;$i -lt 8;$i++){$last=Next-Action $session ([pscustomobject]@{State='UNKNOWN'}) (3000+$i*1000)};Check ($last -eq 'ERR_UNKNOWN_SCREEN') 'Unknown screen did not stop'
  $session=New-Session;Check ((Next-Action $session $story 2000) -eq 'ERR_NOT_COMMISSION') 'Unrelated story was skipped'
  $session=New-Session;Check ((Next-Action $session $items 2000) -eq 'ERR_NOT_COMMISSION') 'Unrelated loot was dismissed'
  $session=New-Session;$commissionHome=[pscustomobject]@{State='HOME'};Check ((Next-Action $session $commissionHome 2000) -eq 'OPEN') 'Home navigation missing';Check ((Next-Action $session $commissionHome 2500) -eq 'WAIT') 'Navigation settle ignored'
  $session=New-Session;$null=Next-Action $session ([pscustomobject]@{State='CLAIM'}) 2000
  Check ((Next-Action $session ([pscustomobject]@{State='NO_REWARDS'}) 2500) -eq 'WAIT') 'Temporary disabled button ended the run'
  Check ((Next-Action $session ([pscustomobject]@{State='NO_REWARDS'}) 7000) -eq 'ERR_REWARD_RESULT_NOT_RECOGNIZED') 'Missing result was reported as no input'
  Signal 'SELF_TEST_PASS'
 }finally{$g.Dispose();$bitmap.Dispose();if($font){$font.Dispose()}}
 exit 0
}
try {
 [CommissionDesktop]::Init($GameWindow,$GamePid);$session=New-Session;$clock=[Diagnostics.Stopwatch]::StartNew();Signal 'READY'
 while($clock.ElapsedMilliseconds -lt 90000){
  $bitmap=[CommissionDesktop]::Capture()
  try{$observation=Read-Observation $bitmap}finally{$bitmap.Dispose()}
  $action=Next-Action $session $observation $clock.ElapsedMilliseconds
  Signal ('STATE_'+$observation.State)
  if($action -eq 'DONE' -or $action -eq 'NO_REWARDS'){Signal $action;exit 0}
  if($action.StartsWith('ERR_')){Signal $action;exit 3}
  if($action -eq 'WAIT'){Start-Sleep -Milliseconds 450;continue}
  Signal ('ACTION_'+$action)
  [CommissionDesktop]::Act(($action -eq 'ESC'),$observation.X,$observation.Y)
  if($action -eq 'AGAIN'){Signal 'DONE';exit 0}
  Start-Sleep -Milliseconds 500
 }
 Signal 'ERR_TIMEOUT';exit 3
} catch {Signal 'ERR_CONTEXT_OR_CAPTURE';exit 3}
