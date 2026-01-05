using Google.Protobuf;
using JoltCSharp;
using jtshared;

var outputDir = Environment.GetEnvironmentVariable("UNITY_RT_PLUGINS_OUTPATH");
Console.WriteLine($"outputDir={outputDir}");
if (null == outputDir) return;
Console.WriteLine($"DefaultBackendInputBufferSize={PbPrimitives.underlying.DefaultBackendInputBufferSize}");
using (var output = File.Create(Path.Combine(outputDir, "PrimitiveConsts.pb"))) {
    PbPrimitives.underlying.WriteTo(output);
}
var configConsts = new ConfigConsts {};
configConsts.CharacterConfigs.Add(PbCharacters.underlying);
configConsts.SkillConfigs.Add(PbSkills.underlying);
configConsts.TriggerConfigs.Add(PbTriggers.underlying);
configConsts.TrapConfigs.Add(PbTraps.underlying);

CharacterConfig bladeGirl;
configConsts.CharacterConfigs.TryGetValue(PbPrimitives.SPECIES_BLADEGIRL, out bladeGirl);
Console.WriteLine($"BladeGirl CapsuleRadius={bladeGirl.CapsuleRadius}");
using (var output = File.Create(Path.Combine(outputDir, "ConfigConsts.pb"))) {
    configConsts.WriteTo(output);
}
