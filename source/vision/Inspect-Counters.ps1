# SPDX-License-Identifier: AGPL-3.0-or-later
# Standalone READ-ONLY numeric-patch diagnostic. No input or network APIs.
param([switch]$SelfTest)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -TypeDefinition @'
using System;using System.Drawing;using System.Drawing.Imaging;using System.IO;using System.Runtime.InteropServices;using System.Text;
public static class CounterReadOnly {
 [StructLayout(LayoutKind.Sequential)]struct Point{public int X,Y;}
 [StructLayout(LayoutKind.Sequential)]struct Rect{public int L,T,R,B;}
 [DllImport("user32.dll")]static extern IntPtr GetForegroundWindow();
 [DllImport("user32.dll")]static extern bool IsIconic(IntPtr h);
 [DllImport("user32.dll")]static extern bool GetClientRect(IntPtr h,out Rect r);
 [DllImport("user32.dll")]static extern bool ClientToScreen(IntPtr h,ref Point p);
 [DllImport("user32.dll",CharSet=CharSet.Unicode)]static extern int GetClassName(IntPtr h,StringBuilder name,int cap);
 [DllImport("user32.dll")]static extern IntPtr WindowFromPoint(Point p);
 [DllImport("user32.dll")]static extern IntPtr GetAncestor(IntPtr h,uint flags);
 [DllImport("user32.dll")]static extern IntPtr SetThreadDpiAwarenessContext(IntPtr c);
 static Rectangle Region(int width,int height){
  // Fixed upper-right numeric field only: current level, /cap and nearby badge.
  return new Rectangle((int)(width*.710),(int)(height*.155),Math.Max(1,(int)(width*.145)),Math.Max(1,(int)(height*.143)));
 }
 static string Encode(Bitmap bitmap){using(var stream=new MemoryStream()){bitmap.Save(stream,ImageFormat.Png);return Convert.ToBase64String(stream.ToArray());}}
 public static string Capture(){
  try{SetThreadDpiAwarenessContext(new IntPtr(-4));}catch(EntryPointNotFoundException){}
  IntPtr window=GetForegroundWindow();var name=new StringBuilder(64);Rect client;
  if(IsIconic(window)||GetClassName(window,name,64)==0||name.ToString()!="UnityWndClass"||!GetClientRect(window,out client))throw new Exception("UNITY_GAME_NOT_FOREGROUND");
  int width=client.R-client.L,height=client.B-client.T;if(width<640||height<360)throw new Exception("GAME_TOO_SMALL");
  Rectangle patch=Region(width,height);var origin=new Point{X=patch.X,Y=patch.Y};
  if(!ClientToScreen(window,ref origin))throw new Exception("CLIENT_COORDINATES");
  // Every sampled point must belong to the game, not an error/tool dialog.
  for(int row=0;row<5;row++)for(int col=0;col<7;col++){
   var point=new Point{X=origin.X+(int)(patch.Width*(col+.5)/7),Y=origin.Y+(int)(patch.Height*(row+.5)/5)};
   if(GetAncestor(WindowFromPoint(point),2)!=window)throw new Exception("NUMERIC_FIELD_OBSCURED");
  }
  using(var bitmap=new Bitmap(patch.Width,patch.Height,PixelFormat.Format32bppArgb)){
   using(var graphics=Graphics.FromImage(bitmap))graphics.CopyFromScreen(origin.X,origin.Y,0,0,bitmap.Size,CopyPixelOperation.SourceCopy);
   Rect after;if(GetForegroundWindow()!=window||!GetClientRect(window,out after)||after.R!=client.R||after.B!=client.B)throw new Exception("GAME_CONTEXT_CHANGED");
   return Encode(bitmap);
  }
 }
 public static void Test(){
  Rectangle patch=Region(1920,1080);if(patch.X!=1363||patch.Y!=167||patch.Width!=278||patch.Height!=154)throw new Exception("REGION");
  if(patch.X<1920*.7||patch.Right>1920*.86||patch.Y<1080*.15||patch.Bottom>1080*.31)throw new Exception("FIELD_BOUNDARY");
  using(var bitmap=new Bitmap(patch.Width,patch.Height)){
   using(var graphics=Graphics.FromImage(bitmap))using(var font=new Font("Segoe UI",44,FontStyle.Bold,GraphicsUnit.Pixel)){
    graphics.Clear(Color.DarkBlue);graphics.DrawString("13 /20",font,Brushes.Gold,20,10);
   }
   byte[] png=Convert.FromBase64String(Encode(bitmap));if(png.Length<16||png[0]!=137||png[1]!=80||png[2]!=78||png[3]!=71)throw new Exception("PNG");
   using(var stream=new MemoryStream(png))using(var roundTrip=new Bitmap(stream))if(roundTrip.Size!=bitmap.Size)throw new Exception("ROUND_TRIP");
  }
 }
}
'@ -ReferencedAssemblies System.Drawing
if($SelfTest){[CounterReadOnly]::Test();Write-Output 'PASS fixed numeric-field boundaries and in-memory PNG round trip; no desktop capture or input in this test.';exit 0}
Write-Output 'READ ONLY: close the error dialog and hide the tool. Open Disc or Trekker Upgrade.'
Write-Output 'Switch to the game within 8 seconds. No click/key/network request will be sent.'
Write-Output 'Only the small upper-right LEVEL / CAP field is copied into the text below.'
Write-Output 'No whole-screen image, account text, path or image/log file is collected.'
Start-Sleep -Seconds 8
try{
 $encoded=[CounterReadOnly]::Capture()
 Write-Output '=== BEGIN SV-COUNTER-PATCH-1 ==='
 Write-Output $encoded
 Write-Output '=== END SV-COUNTER-PATCH-1 ==='
 Write-Output 'Copy the BEGIN/END block into the conversation. It contains the numeric patch only.'
}catch{Write-Output ('STOPPED: '+$_.Exception.GetBaseException().Message);exit 1}
