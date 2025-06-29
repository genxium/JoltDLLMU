using Google.Protobuf;
using JoltCSharp;

var outputDir = Environment.GetEnvironmentVariable("UNITY_RT_PLUGINS_OUTPATH");
Console.WriteLine($"outputDir={outputDir}");
if (null == outputDir) return;
var pbConsts = new PbConsts();
Console.WriteLine($"primitiveConsts.DefaultBackendInputBufferSize={pbConsts.primitiveConsts.DefaultBackendInputBufferSize}");
using (var output = File.Create(Path.Combine(outputDir, "PrimitiveConsts.pb"))) {
    pbConsts.primitiveConsts.WriteTo(output);
}
File.WriteAllBytes(Path.Combine(outputDir, "ConfigConsts.pb"), pbConsts.configConsts.ToByteArray());
