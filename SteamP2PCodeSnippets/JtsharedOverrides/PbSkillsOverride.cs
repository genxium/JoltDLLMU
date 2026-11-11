using jtshared;
using static JoltCSharp.PbPrimitivesOverride;

namespace JoltCSharp {
    public class PbSkillsOverride : PbSkills {

        public static float cDeg2Rad = (System.MathF.PI / 180.0f);
        public static System.Numerics.Quaternion cTurn75DegsWrtZAxis = System.Numerics.Quaternion.CreateFromAxisAngle(System.Numerics.Vector3.UnitZ, cDeg2Rad * 75);

        /////////////////////////////////////////////////////////////////////////////////////////
        public PbSkillsOverride(PrimitiveConsts primitiveConsts) : base(primitiveConsts) {
        }

        public static PbSkillsOverride Instance {
            get {
                if (null != _instance) {
                    return _instance;
                }
                lock (_padLock) {
                    if (null == _instance) {
                        _instance = new PbSkillsOverride(PbPrimitivesOverride.Instance.getUnderlying());
                    }
                }
                return _instance;
            }
        }

        ////////////////////////////////////////////////////////////////////////////////////////
        private static PbSkillsOverride _instance = null;
        private static readonly object _padLock = new object();
        private bool _initialized = false;
        protected override bool lazyInit() {
            if (_initialized) {
                return true;
            }
            bool res = base.lazyInit();

            var BladeGirlGroundSlash1 = underlying[PbPrimitivesOverride.BladeGirlGroundSlash1Id];
            var BladeGirlGroundSlash1Hit1 = BladeGirlGroundSlash1.Hits[0];

            var BladeGirlDiverImpact = underlying[PbPrimitivesOverride.BladeGirlDiverImpactId];
            var BladeGirlDiverImpactHit1 = BladeGirlDiverImpact.Hits[0];

            // New skill definition

            // Fine tune
            BladeGirlDiverImpactHit1.Damage = 4;
            BladeGirlDiverImpactHit1.HitboxOffsetX = 10.0f;
            BladeGirlDiverImpactHit1.HitboxOffsetY = 0.0f;
            BladeGirlDiverImpactHit1.HitboxHalfSizeX = 12f;
            BladeGirlDiverImpactHit1.HitboxHalfSizeY = 8f;
            BladeGirlDiverImpactHit1.SelfLockVelY = -32.0f* PbPrimitivesOverride.BATTLE_DYNAMICS_FPS;
            BladeGirlDiverImpactHit1.HitSfxName = "SlashHit2";
            BladeGirlDiverImpactHit1.CharacterEmitSfxName = "Whoosh1";
            BladeGirlDiverImpactHit1.RemainsUponHit = false;
            BladeGirlDiverImpactHit1.MeleeHitSelfStunFrames = PbPrimitivesOverride.DEFAULT_MELEE_HIT_SELF_STUN_FRAMES;

            BladeGirlGroundSlash1Hit1.Damage = 8;
            BladeGirlGroundSlash1Hit1.HitboxOffsetX = 18f;
            BladeGirlGroundSlash1Hit1.HitboxOffsetY = 20f;
            BladeGirlGroundSlash1Hit1.HitboxHalfSizeX = 16f;
            BladeGirlGroundSlash1Hit1.HitboxHalfSizeY = 16f;
            BladeGirlGroundSlash1Hit1.HitSfxName = "SlashHit1";
            BladeGirlGroundSlash1Hit1.CharacterEmitSfxName = "Whoosh1";
            BladeGirlGroundSlash1Hit1.MeleeHitSelfStunFrames = 0;

            var BladeGirlGroundSlash2 = underlying[PbPrimitivesOverride.BladeGirlGroundSlash2Id];
            var BladeGirlGroundSlash2Hit1 = BladeGirlGroundSlash2.Hits[0];
            BladeGirlGroundSlash2Hit1.Damage = 10;
            BladeGirlGroundSlash2Hit1.HitboxOffsetX = 16f;
            BladeGirlGroundSlash2Hit1.HitboxOffsetY = 18f;
            BladeGirlGroundSlash2Hit1.HitboxHalfSizeX = 16f;
            BladeGirlGroundSlash2Hit1.HitboxHalfSizeY = 16f;
            BladeGirlGroundSlash2Hit1.HitSfxName = "SlashHit1";
            BladeGirlGroundSlash2Hit1.CharacterEmitSfxName = "Whoosh1";
            BladeGirlGroundSlash2Hit1.StartupFrames = 6;
            BladeGirlGroundSlash2Hit1.ActiveFrames = 8;
            BladeGirlGroundSlash2Hit1.CooldownFrames = 8;
            BladeGirlGroundSlash2.RecoveryFrames = BladeGirlGroundSlash2Hit1.StartupFrames+BladeGirlGroundSlash2Hit1.ActiveFrames+BladeGirlGroundSlash2Hit1.CooldownFrames;
            BladeGirlGroundSlash2.RecoveryFramesOnBlock = BladeGirlGroundSlash2Hit1.StartupFrames+BladeGirlGroundSlash2Hit1.ActiveFrames+BladeGirlGroundSlash2Hit1.CooldownFrames;
            BladeGirlGroundSlash2.RecoveryFramesOnHit = BladeGirlGroundSlash2Hit1.StartupFrames+BladeGirlGroundSlash2Hit1.ActiveFrames+BladeGirlGroundSlash2Hit1.CooldownFrames;

            var BladeGirlGroundSlash3 = underlying[PbPrimitivesOverride.BladeGirlGroundSlash3Id];
            var BladeGirlGroundSlash3Hit1 = BladeGirlGroundSlash3.Hits[0];
            BladeGirlGroundSlash3Hit1.HitboxOffsetX = 20f;
            BladeGirlGroundSlash3Hit1.HitboxOffsetY = 18f;
            BladeGirlGroundSlash3Hit1.HitboxHalfSizeX = 23f;
            BladeGirlGroundSlash3Hit1.HitboxHalfSizeY = 18f;
            BladeGirlGroundSlash3Hit1.HitSfxName = "SlashHit1";
            BladeGirlGroundSlash3Hit1.CharacterEmitSfxName = "Whoosh2";
            BladeGirlGroundSlash3Hit1.MeleeHitSelfStunFrames = (PbPrimitivesOverride.BATTLE_DYNAMICS_FPS/12);
            BladeGirlGroundSlash3Hit1.StartupFrames = 18;
            BladeGirlGroundSlash3Hit1.ActiveFrames = 16;
            BladeGirlGroundSlash3.RecoveryFrames = BladeGirlGroundSlash3Hit1.StartupFrames+BladeGirlGroundSlash3Hit1.ActiveFrames+BladeGirlGroundSlash3Hit1.CooldownFrames;
            BladeGirlGroundSlash3.RecoveryFramesOnBlock = BladeGirlGroundSlash3Hit1.StartupFrames+BladeGirlGroundSlash3Hit1.ActiveFrames+BladeGirlGroundSlash3Hit1.CooldownFrames;
            BladeGirlGroundSlash3.RecoveryFramesOnHit = BladeGirlGroundSlash3Hit1.StartupFrames+BladeGirlGroundSlash3Hit1.ActiveFrames+BladeGirlGroundSlash3Hit1.CooldownFrames;

            var BladeGirlAirSlash1 = underlying[PbPrimitivesOverride.BladeGirlAirSlash1Id];
            var BladeGirlAirSlash1Hit1 = BladeGirlAirSlash1.Hits[0];
            BladeGirlAirSlash1Hit1.Damage = 8;
            BladeGirlAirSlash1Hit1.HitboxOffsetX = 17f;
            BladeGirlAirSlash1Hit1.HitboxOffsetY = 30f;
            BladeGirlAirSlash1Hit1.HitboxHalfSizeX = 16f;
            BladeGirlAirSlash1Hit1.HitboxHalfSizeY = 18f;
            BladeGirlAirSlash1Hit1.HitSfxName = "SlashHit2";
            BladeGirlAirSlash1Hit1.CharacterEmitSfxName = "Whoosh2";
            BladeGirlAirSlash1Hit1.RemainsUponHit = false;
            BladeGirlAirSlash1Hit1.MeleeHitSelfStunFrames = PbPrimitivesOverride.DEFAULT_MELEE_HIT_SELF_STUN_FRAMES;

            var BlackSaber1GroundSlash1 = underlying[PbPrimitivesOverride.BlackSaber1GroundSlash1Id];
            var BlackSaber1GroundSlash1Hit1 = BlackSaber1GroundSlash1.Hits[0];
            BlackSaber1GroundSlash1Hit1.Damage = 4;
            BlackSaber1GroundSlash1Hit1.HitboxOffsetX = 11f;
            BlackSaber1GroundSlash1Hit1.HitboxOffsetY = 17f;
            BlackSaber1GroundSlash1Hit1.HitboxHalfSizeX = 12f;
            BlackSaber1GroundSlash1Hit1.HitboxHalfSizeY = 8f;
            BlackSaber1GroundSlash1Hit1.StartupFrames = 48;
            BlackSaber1GroundSlash1Hit1.ActiveFrames = 6;
            BlackSaber1GroundSlash1Hit1.CooldownFrames = 18;
            BlackSaber1GroundSlash1.RecoveryFrames = BlackSaber1GroundSlash1Hit1.StartupFrames + BlackSaber1GroundSlash1Hit1.ActiveFrames + BlackSaber1GroundSlash1Hit1.CooldownFrames;
            BlackSaber1GroundSlash1.RecoveryFramesOnBlock = BlackSaber1GroundSlash1Hit1.StartupFrames + BlackSaber1GroundSlash1Hit1.ActiveFrames + BlackSaber1GroundSlash1Hit1.CooldownFrames;
            BlackSaber1GroundSlash1.RecoveryFramesOnHit = BlackSaber1GroundSlash1Hit1.StartupFrames + BlackSaber1GroundSlash1Hit1.ActiveFrames + BlackSaber1GroundSlash1Hit1.CooldownFrames;
            BlackSaber1GroundSlash1Hit1.CharacterEmitSfxName = "SlowLaserBladeWhoosh1";

            var WolverineGroundSlash1 = underlying[PbPrimitivesOverride.Wolverine1GroundSlash1Id];
            var WolverineGroundSlash1Hit1 = WolverineGroundSlash1.Hits[0];
            WolverineGroundSlash1Hit1.StartupInvinsibleFrames = 0;
            WolverineGroundSlash1Hit1.HitboxOffsetX = 20f;
            WolverineGroundSlash1Hit1.HitboxOffsetY = 34f;
            WolverineGroundSlash1Hit1.HitboxHalfSizeX = 20f;
            WolverineGroundSlash1Hit1.HitboxHalfSizeY = 8f;
            WolverineGroundSlash1Hit1.StartupFrames = 50;
            WolverineGroundSlash1Hit1.CharacterEmitSfxName = "SlowClawWhoosh1";

            var BlackThrower1TimedBomb = underlying[PbPrimitivesOverride.BlackThrower1TimedBombId];
            var BasicTimedBombHit1 = BlackThrower1TimedBomb.Hits[0];
            //BasicTimedBombHit1.Speed = 4.5f * PbPrimitivesOverride.BATTLE_DYNAMICS_FPS;
            BasicTimedBombHit1.Hardness = 7;
            BasicTimedBombHit1.HitAnimRdfCnt = 45;
            BasicTimedBombHit1.VanishingAnimRdfCnt = 25;
            BasicTimedBombHit1.HitboxOffsetX = 8f;
            BasicTimedBombHit1.HitboxOffsetY = 56f;
            BasicTimedBombHit1.HitboxHalfSizeX = 5.0f;
            BasicTimedBombHit1.HitboxHalfSizeY = 5.0f;
            BasicTimedBombHit1.Friction = 0;
            BasicTimedBombHit1.GravityFactor = 1.0f;
            BasicTimedBombHit1.Speed *= 1.2f;

            var BlackShooter1RapidFire = underlying[PbPrimitivesOverride.BlackShooter1RapidFireId];
            var BasicRapidFireHit1 = BlackShooter1RapidFire.Hits[0];
            var BasicRapidFireHit2 = BlackShooter1RapidFire.Hits[1];
            var BasicRapidFireHit3 = BlackShooter1RapidFire.Hits[2];
            var BasicRapidFireHit4 = BlackShooter1RapidFire.Hits[3];
            BasicRapidFireHit1.Damage = BasicRapidFireHit2.Damage = BasicRapidFireHit3.Damage = BasicRapidFireHit4.Damage = 4;
            BasicRapidFireHit1.Hardness = BasicRapidFireHit2.Hardness = BasicRapidFireHit3.Hardness = BasicRapidFireHit4.Hardness = 4;

            rectifyByPPU();

            _initialized = true;
            return _initialized;
        }

        private void rectifyByPPU() {
            foreach (var s in underlying.Values) {
                foreach (var b in s.Hits) {
                    b.BeamVisualSizeY /= DEFAULT_PIXELS_PER_UNIT;
                    b.HitboxHalfSizeX /= DEFAULT_PIXELS_PER_UNIT;
                    b.HitboxHalfSizeY /= DEFAULT_PIXELS_PER_UNIT;
                    b.HitboxOffsetX /= DEFAULT_PIXELS_PER_UNIT;
                    b.HitboxOffsetY /= DEFAULT_PIXELS_PER_UNIT;
                    b.HitboxSizeIncX /= DEFAULT_PIXELS_PER_UNIT;
                    b.HitboxSizeIncY /= DEFAULT_PIXELS_PER_UNIT;
                    b.Speed /= DEFAULT_PIXELS_PER_UNIT;
                    if (PbPrimitivesOverride.Instance.getUnderlying().NoLockVel != b.SelfLockVelX) {
                        b.SelfLockVelX /= DEFAULT_PIXELS_PER_UNIT;
                    }
                    if (PbPrimitivesOverride.Instance.getUnderlying().NoLockVel != b.SelfLockVelY) {
                        b.SelfLockVelY /= DEFAULT_PIXELS_PER_UNIT;
                    }
                    if (PbPrimitivesOverride.Instance.getUnderlying().NoLockVel != b.SelfLockVelYWhenFlying) {
                        b.SelfLockVelYWhenFlying /= DEFAULT_PIXELS_PER_UNIT;
                    }
                    if (PbPrimitivesOverride.Instance.getUnderlying().NoLockVel != b.PushbackVelX) {
                        b.PushbackVelX /= DEFAULT_PIXELS_PER_UNIT;
                    }
                    if (PbPrimitivesOverride.Instance.getUnderlying().NoLockVel != b.PushbackVelY) {
                        b.PushbackVelY /= DEFAULT_PIXELS_PER_UNIT;
                    }
                }
            }
        }
    }
}
