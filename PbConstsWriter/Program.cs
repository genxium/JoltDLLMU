using Google.Protobuf;
using JoltCSharp;
using jtshared;

PbPrimitives pbPrimitives = new PbPrimitives();
PrimitiveConsts primitiveConsts = pbPrimitives.getUnderlying();

var configConsts = new ConfigConsts { };
configConsts.CharacterConfigs.Add(new PbCharacters(primitiveConsts).getUnderlying());
configConsts.SkillConfigs.Add(new PbSkills(primitiveConsts).getUnderlying());
configConsts.TriggerConfigs.Add(new PbTriggers(primitiveConsts).getUnderlying());
configConsts.TrapConfigs.Add(new PbTraps(primitiveConsts).getUnderlying());
configConsts.PickableConfigs.Add(new PbPickables(primitiveConsts).getUnderlying());

var outputDir = Environment.GetEnvironmentVariable("UNITY_RT_PLUGINS_OUTPATH");
Console.WriteLine($"outputDir={outputDir}");
if (null == outputDir) return;

Console.WriteLine($"DefaultBackendInputBufferSize={pbPrimitives.getUnderlying().DefaultBackendInputBufferSize}");
using (var output = File.Create(Path.Combine(outputDir, "PrimitiveConsts.pb"))) {
    pbPrimitives.getUnderlying().WriteTo(output);
}


CharacterConfig bladeGirl;
configConsts.CharacterConfigs.TryGetValue(PbPrimitives.SPECIES_BLADEGIRL, out bladeGirl);
Console.WriteLine($"BladeGirl CapsuleRadius={bladeGirl.CapsuleRadius}");
using (var output = File.Create(Path.Combine(outputDir, "ConfigConsts.pb"))) {
    configConsts.WriteTo(output);
}
