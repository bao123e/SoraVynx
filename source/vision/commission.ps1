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
  $writer=[Windows.Storage.Streams.DataWriter]::new();$writer.WriteBytes($bytes)
  $software=[Windows.Graphics.Imaging.SoftwareBitmap]::CreateCopyFromBuffer($writer.DetachBuffer(),[Windows.Graphics.Imaging.BitmapPixelFormat]::Bgra8,$copy.Width,$copy.Height,[Windows.Graphics.Imaging.BitmapAlphaMode]::Ignore)
  $task=$asTask.MakeGenericMethod([Windows.Media.Ocr.OcrResult]).Invoke($null,@($engine.RecognizeAsync($software)))
  if(!$task.Wait(8000)){throw 'OCR_TIMEOUT'}
  $lines=@(foreach($line in $task.Result.Lines){
   $left=1e9;$top=1e9;$right=0;$bottom=0
   foreach($word in $line.Words){$b=$word.BoundingRect;$left=[Math]::Min($left,$b.X);$top=[Math]::Min($top,$b.Y);$right=[Math]::Max($right,$b.X+$b.Width);$bottom=[Math]::Max($bottom,$b.Y+$b.Height)}
   if($right -gt $left){[pscustomobject]@{Text=$line.Text;X=$left/$copy.Width;Y=$top/$copy.Height;W=($right-$left)/$copy.Width;H=($bottom-$top)/$copy.Height}}
  })
  return [pscustomobject]@{Lines=$lines;Bitmap=$copy};$copy=$null
 }finally{if($software){$software.Dispose()};if($writer){$writer.Dispose()};if($copy){$copy.Dispose()}}
}
function Normal([string]$text){return [regex]::Replace($text.ToLowerInvariant(),'[^a-z0-9]','')}
function Cyan-Button($bitmap,$line){
 $hits=0
 foreach($point in @(@($line.X-.008,$line.Y+$line.H/2),@($line.X+$line.W+.008,$line.Y+$line.H/2),@($line.X+$line.W/2,$line.Y-.01),@($line.X+$line.W/2,$line.Y+$line.H+.01))){
  $x=[int]($point[0]*$bitmap.Width);$y=[int]($point[1]*$bitmap.Height)
  if($x -ge 0 -and $x -lt $bitmap.Width -and $y -ge 0 -and $y -lt $bitmap.Height){$c=$bitmap.GetPixel($x,$y);if($c.G -gt 130 -and $c.B -gt 120 -and $c.G-$c.R -gt 30 -and $c.B-$c.R -gt 20){$hits++}}
 }
 return $hits -ge 2
}
function Observe($lines,$bitmap){
 $top=@($lines | Where-Object {$_.X -lt .25 -and $_.Y -lt .18} | ForEach-Object {Normal $_.Text}) -join ''
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
 $home=@($lines | Where-Object {(Normal $_.Text) -eq 'commission' -and $_.X -gt .7 -and $_.Y -lt .3})
 if($home.Count -eq 1){$p=$home[0];return [pscustomobject]@{State='HOME';X=$p.X+$p.W/2;Y=$p.Y+$p.H/2}}
 return [pscustomobject]@{State='UNKNOWN';X=0.0;Y=0.0}
}
function New-Session {return @{Entry=0;Claim=0;Skip=0;Items=0;Again=0;Last=0;Unknown=0;Actions=0}}
function Next-Action($session,$observation,[long]$now){
 if($session.Actions -ge 12){return 'ERR_ACTION_LIMIT'}
 switch($observation.State){
  'HOME' {if($session.Entry -gt 0){return 'ERR_ENTRY_NOT_CHANGED'};$session.Entry++;$action='OPEN'}
  'CLAIM' {if($session.Again -gt 0){return 'DONE'};if($now-$session.Last -lt 1500){return 'WAIT'};if($session.Claim -ge 2){return 'ERR_CLAIM_NOT_CHANGED'};$session.Claim++;$action='CLAIM'}
  'STORY' {if($now-$session.Last -lt 800){return 'WAIT'};if($session.Skip -ge 4){return 'ERR_STORY_LIMIT'};$session.Skip++;$action='ESC'}
  'ITEMS' {if($now-$session.Last -lt 800){return 'WAIT'};if($session.Items -ge 4){return 'ERR_ITEMS_LIMIT'};$session.Items++;$action='CONTINUE'}
  'RESULT' {if($session.Again -gt 0){return 'DONE'};if($now-$session.Last -lt 800){return 'WAIT'};$session.Again++;$action='AGAIN'}
  'NO_REWARDS' {return 'NO_REWARDS'}
  default {$session.Unknown++;if($session.Unknown -ge 8){return 'ERR_UNKNOWN_SCREEN'};return 'WAIT'}
 }
 $session.Unknown=0;$session.Last=$now;$session.Actions++;return $action
}
if($SelfTest){
 function Check($condition,[string]$message){if(!$condition){throw $message}}
 $bitmap=[Drawing.Bitmap]::new(1280,720);$g=[Drawing.Graphics]::FromImage($bitmap)
 try{
  $g.Clear([Drawing.Color]::FromArgb(25,35,48));$font=[Drawing.Font]::new('Segoe UI',30)
  $g.DrawString('Commission Complete!',$font,[Drawing.Brushes]::White,320,70)
  $g.FillRectangle([Drawing.Brushes]::Turquoise,660,550,390,90);$g.DrawString('Dispatch Again',$font,[Drawing.Brushes]::White,675,565)
  $read=Read-Image $bitmap
  try{$o=Observe $read.Lines $read.Bitmap;Check ($o.State -eq 'RESULT') ('Real OCR recognition failed: '+$o.State)}finally{$read.Bitmap.Dispose()}
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
  Signal 'SELF_TEST_PASS'
 }finally{$g.Dispose();$bitmap.Dispose();if($font){$font.Dispose()}}
 exit 0
}
try {
 [CommissionDesktop]::Init($GameWindow,$GamePid);$session=New-Session;$clock=[Diagnostics.Stopwatch]::StartNew();Signal 'READY'
 while($clock.ElapsedMilliseconds -lt 90000){
  $bitmap=[CommissionDesktop]::Capture();$read=$null
  try{$read=Read-Image $bitmap;$observation=Observe $read.Lines $read.Bitmap}finally{$bitmap.Dispose();if($read){$read.Bitmap.Dispose()}}
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
