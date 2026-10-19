using System;
using System.IO;
using System.IO.Compression;
using System.Text;
using System.Collections.Generic;
using System.Runtime.InteropServices;

// Offscreen shader diagnostics using Windows' installed compiler and D3D11 WARP.
// No game window, SDK, downloads, native compiler or machine configuration needed.
public static class SceneShaderCheck {
 [DllImport("d3dcompiler_47.dll", CharSet=CharSet.Ansi)]
 static extern int D3DCompile(byte[] source,UIntPtr length,string name,IntPtr defines,IntPtr include,
   string entry,string target,uint flags,uint flags2,out IntPtr code,out IntPtr errors);
 [DllImport("d3d11.dll")]
 static extern int D3D11CreateDevice(IntPtr adapter,uint driver,IntPtr software,uint flags,
   uint[] levels,uint count,uint sdk,out IntPtr device,out uint level,out IntPtr context);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate IntPtr BlobPointer(IntPtr self);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate UIntPtr BlobSize(IntPtr self);
 static T Method<T>(IntPtr obj,int index) where T:Delegate {
  return Marshal.GetDelegateForFunctionPointer<T>(Marshal.ReadIntPtr(Marshal.ReadIntPtr(obj),index*IntPtr.Size));
 }
 static void Check(int hr) { Marshal.ThrowExceptionForHR(hr); }
 public static byte[] Compile(string source,string entry,string profile) {
  byte[] text=Encoding.UTF8.GetBytes(source); IntPtr code=IntPtr.Zero,errors=IntPtr.Zero;
  try {
   int hr=D3DCompile(text,(UIntPtr)text.Length,"BuiltInScene",IntPtr.Zero,IntPtr.Zero,entry,profile,2048,0,out code,out errors);
   string diagnostic=errors==IntPtr.Zero?"":Marshal.PtrToStringAnsi(Method<BlobPointer>(errors,3)(errors));
   if(hr<0) throw new InvalidOperationException(entry+": "+diagnostic+" (0x"+hr.ToString("X")+")");
   if(!String.IsNullOrWhiteSpace(diagnostic)) throw new InvalidOperationException(entry+" compiler diagnostic: "+diagnostic);
   int length=checked((int)Method<BlobSize>(code,4)(code).ToUInt64());
   byte[] bytes=new byte[length]; Marshal.Copy(Method<BlobPointer>(code,3)(code),bytes,0,length); return bytes;
  } finally { if(errors!=IntPtr.Zero) Marshal.Release(errors); if(code!=IntPtr.Zero) Marshal.Release(code); }
 }
 [StructLayout(LayoutKind.Sequential)] struct TextureDesc {
  public uint Width,Height,Mips,ArraySize,Format,SampleCount,SampleQuality,Usage,Bind,Cpu,Misc;
 }
 [StructLayout(LayoutKind.Sequential)] struct BufferDesc { public uint Bytes,Usage,Bind,Cpu,Misc,Stride; }
 [StructLayout(LayoutKind.Sequential)] struct Subresource { public IntPtr Data; public uint RowPitch,SlicePitch; }
 [StructLayout(LayoutKind.Sequential)] struct Mapped { public IntPtr Data; public uint RowPitch,DepthPitch; }
 [StructLayout(LayoutKind.Sequential)] struct Viewport { public float X,Y,Width,Height,Min,Max; }
 [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate int TextureCreate(IntPtr self,ref TextureDesc desc,IntPtr data,out IntPtr resource);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate int BufferCreate(IntPtr self,ref BufferDesc desc,ref Subresource data,out IntPtr resource);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate int ViewCreate(IntPtr self,IntPtr resource,IntPtr desc,out IntPtr view);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate int ShaderCreate(IntPtr self,byte[] data,UIntPtr size,IntPtr linkage,out IntPtr shader);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate void SetShader(IntPtr self,IntPtr shader,IntPtr classes,uint count);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate void SetBuffer(IntPtr self,uint start,uint count,ref IntPtr buffer);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate void SetTarget(IntPtr self,uint count,ref IntPtr target,IntPtr depth);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate void SetViewport(IntPtr self,uint count,ref Viewport viewport);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate void SetTopology(IntPtr self,uint topology);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate void Draw(IntPtr self,uint count,uint start);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate void CopyResource(IntPtr self,IntPtr dest,IntPtr source);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate int MapResource(IntPtr self,IntPtr resource,uint sub,uint type,uint flags,out Mapped mapped);
 [UnmanagedFunctionPointer(CallingConvention.StdCall)] delegate void UnmapResource(IntPtr self,IntPtr resource,uint sub);
 static float[] Normalize(float x,float y,float z) {
  float length=(float)Math.Sqrt(x*x+y*y+z*z); return new float[]{x/length,y/length,z/length};
 }
 public static int Difference(byte[] a,byte[] b) {
  if(a.Length!=b.Length) throw new ArgumentException("Image dimensions differ.");
  int maximum=0; for(int i=0;i<a.Length;i++) maximum=Math.Max(maximum,Math.Abs(a[i]-b[i])); return maximum;
 }
 public static double MeanDifference(byte[] a,byte[] b) {
  if(a.Length!=b.Length) throw new ArgumentException("Image dimensions differ.");
  double sum=0;
  for(int i=0;i<a.Length;i++) if(i%4!=3) sum+=Math.Abs(a[i]-b[i]);
  return sum/(a.Length/4*3);
 }
 public static byte[] Sky(byte[] vs,byte[] ps,int width,int height,float yaw,float elevation,float seconds,string path,float cloudThreshold=.57f) {
  var objects=new List<IntPtr>(); IntPtr memory=IntPtr.Zero;
  try {
   IntPtr device,context; uint level;
   Check(D3D11CreateDevice(IntPtr.Zero,5,IntPtr.Zero,0,new uint[]{0xb000},1,7,out device,out level,out context));
   objects.Add(device); objects.Add(context);
   var desc=new TextureDesc{Width=(uint)width,Height=(uint)height,Mips=1,ArraySize=1,Format=28,SampleCount=1,Bind=32};
   IntPtr target,view,readback,vertex,pixel,buffer;
   Check(Method<TextureCreate>(device,5)(device,ref desc,IntPtr.Zero,out target)); objects.Add(target);
   Check(Method<ViewCreate>(device,9)(device,target,IntPtr.Zero,out view)); objects.Add(view);
   desc.Usage=3; desc.Bind=0; desc.Cpu=0x20000;
   Check(Method<TextureCreate>(device,5)(device,ref desc,IntPtr.Zero,out readback)); objects.Add(readback);
   Check(Method<ShaderCreate>(device,12)(device,vs,(UIntPtr)vs.Length,IntPtr.Zero,out vertex)); objects.Add(vertex);
   Check(Method<ShaderCreate>(device,15)(device,ps,(UIntPtr)ps.Length,IntPtr.Zero,out pixel)); objects.Add(pixel);
   var forward=Normalize((float)(Math.Sin(yaw)*Math.Cos(elevation)),(float)Math.Sin(elevation),(float)(Math.Cos(yaw)*Math.Cos(elevation)));
   var right=Normalize(forward[2],0,-forward[0]);
   float[] up={forward[1]*right[2]-forward[2]*right[1],forward[2]*right[0]-forward[0]*right[2],forward[0]*right[1]-forward[1]*right[0]};
   float v=(float)Math.Tan(Math.PI/8),h=v*width/height;
   // Row-vector far-plane reconstruction, stored transposed for HLSL columns.
   float[] constants=new float[96];
   for(int i=0;i<3;i++) { constants[i*4]=right[i]*h; constants[i*4+1]=up[i]*v; constants[i*4+3]=forward[i]; }
   constants[15]=1;
   constants[88]=-30; constants[89]=51; constants[90]=-24; constants[91]=.00465f;
   constants[92]=(float)((seconds%1800)*Math.PI*2/1800); constants[93]=.85f; constants[94]=.006f; constants[95]=cloudThreshold;
   memory=Marshal.AllocHGlobal(constants.Length*4); Marshal.Copy(constants,0,memory,constants.Length);
   var initial=new Subresource{Data=memory}; var bd=new BufferDesc{Bytes=(uint)(constants.Length*4),Bind=4};
   Check(Method<BufferCreate>(device,3)(device,ref bd,ref initial,out buffer)); objects.Add(buffer);
   var viewport=new Viewport{Width=width,Height=height,Max=1};
   Method<SetTarget>(context,33)(context,1,ref view,IntPtr.Zero);
   Method<SetViewport>(context,44)(context,1,ref viewport);
   Method<SetTopology>(context,24)(context,4);
   Method<SetBuffer>(context,7)(context,0,1,ref buffer); Method<SetBuffer>(context,16)(context,0,1,ref buffer);
   Method<SetShader>(context,11)(context,vertex,IntPtr.Zero,0); Method<SetShader>(context,9)(context,pixel,IntPtr.Zero,0);
   Method<Draw>(context,13)(context,3,0);
   Method<CopyResource>(context,47)(context,readback,target);
   Mapped mapped; Check(Method<MapResource>(context,14)(context,readback,0,1,0,out mapped));
   byte[] rgba=new byte[width*height*4];
   try { for(int y=0;y<height;y++) Marshal.Copy(IntPtr.Add(mapped.Data,checked(y*(int)mapped.RowPitch)),rgba,y*width*4,width*4); }
   finally { Method<UnmapResource>(context,15)(context,readback,0); }
   WritePng(path,width,height,rgba); return rgba;
  } finally {
   if(memory!=IntPtr.Zero) Marshal.FreeHGlobal(memory);
   for(int i=objects.Count-1;i>=0;i--) Marshal.Release(objects[i]);
  }
 }
 static void BigEndian(Stream output,uint value) {
  output.WriteByte((byte)(value>>24)); output.WriteByte((byte)(value>>16)); output.WriteByte((byte)(value>>8)); output.WriteByte((byte)value);
 }
 static void Chunk(Stream output,string name,byte[] data) {
  byte[] type=Encoding.ASCII.GetBytes(name); BigEndian(output,(uint)data.Length);
  output.Write(type,0,4); output.Write(data,0,data.Length);
  uint crc=0xffffffff;
  foreach(byte b in type) { crc^=b; for(int j=0;j<8;j++) crc=(crc>>1)^((crc&1)!=0?0xedb88320u:0); }
  foreach(byte b in data) { crc^=b; for(int j=0;j<8;j++) crc=(crc>>1)^((crc&1)!=0?0xedb88320u:0); }
  BigEndian(output,crc^0xffffffff);
 }
 static void WritePng(string path,int width,int height,byte[] rgba) {
  using(var output=File.Create(path)) {
   output.Write(new byte[]{137,80,78,71,13,10,26,10},0,8);
   using(var header=new MemoryStream()) {
    BigEndian(header,(uint)width); BigEndian(header,(uint)height); header.Write(new byte[]{8,6,0,0,0},0,5); Chunk(output,"IHDR",header.ToArray());
   }
   using(var compressed=new MemoryStream()) {
    using(var zip=new ZLibStream(compressed,CompressionLevel.Optimal,true)) {
     for(int y=0;y<height;y++) { zip.WriteByte(0); zip.Write(rgba,y*width*4,width*4); }
    }
    Chunk(output,"IDAT",compressed.ToArray());
   }
   Chunk(output,"IEND",new byte[0]);
  }
 }
}
