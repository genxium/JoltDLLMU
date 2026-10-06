using Google.Protobuf.Collections;
using jtshared;
using System;
using static JoltCSharp.PbPrimitives;

namespace JoltCSharp {
    public class PbTraps {
        private PrimitiveConsts primitiveConsts;

        public PbTraps(in PrimitiveConsts primitiveConsts) {
            this.primitiveConsts = primitiveConsts;
        }

        protected MapField<uint, TrapConfig>? underlying;

        protected virtual bool lazyInit() {
            if (null != underlying) return true;

            TrapConfig SlidingPlatformTrap = new TrapConfig {
                Tpt = primitiveConsts.Tpts.SlidingPlatform,
                Name = "SlidingPlatform",
                NoXFlipRendering = true,
                UseKinematic = false,
                PseudoKinematicFactor = 1.0f,
                BlPushbackAttenuation = 1.0f,
                DefaultBoxHalfSizeX = 100.0f,
                DefaultBoxHalfSizeY = 100.0f,
                DefaultLinearSpeed = 7.0f*BATTLE_DYNAMICS_FPS, 
                DefaultCooldownRdfCount = 60
            };

            TrapConfig RotatingPlatformTrap = new TrapConfig {
                Tpt = primitiveConsts.Tpts.RotatingPlatform,
                Name = "RotatingPlatform",
                NoXFlipRendering = true,
                UseKinematic = false,
                PseudoKinematicFactor = 1.0f,
                BlPushbackAttenuation = 1.0f,
                DefaultBoxHalfSizeX = 100.0f,
                DefaultBoxHalfSizeY = 100.0f,
                DefaultLinearSpeed = 0, 
                DefaultCooldownRdfCount = 60,
                AllowsRotationFromPhySys = true,
            };

            TrapConfig ConveyorBeltTrap = new TrapConfig {
                Tpt = primitiveConsts.Tpts.ConveyorBelt,
                Name = "ConveyorBelt",
                NoXFlipRendering = false,
                UseKinematic = true,
                BlPushbackAttenuation = 1.0f,
                DefaultBoxHalfSizeX = 100.0f,
                DefaultBoxHalfSizeY = 100.0f,
                DefaultLinearSpeed = 0, 
                DefaultCooldownRdfCount = 60
            };

            TrapConfig BossDoorTrap = new TrapConfig {
                Tpt = primitiveConsts.Tpts.BossDoor,
                Name = "BossDoor",
                NoXFlipRendering = false,
                UseKinematic = true,
                BlPushbackAttenuation = 1.0f,
                DefaultBoxHalfSizeX = 10.0f,
                DefaultBoxHalfSizeY = 32.0f,
                DefaultLinearSpeed = 0, 
                DefaultCooldownRdfCount = 0
            };

            TrapConfig SpringTrap = new TrapConfig {
                Tpt = primitiveConsts.Tpts.Spring,
                Name = "Spring",
                NoXFlipRendering = false,
                UseKinematic = true,
                BlPushbackAttenuation = 1.0f,
                DefaultBoxHalfSizeX = 10.0f,
                DefaultBoxHalfSizeY = 4.0f,
                DefaultLinearSpeed = 0, 
                DefaultCooldownRdfCount = 30,
            };

            TrapConfig BrickTrap = new TrapConfig {
                Tpt = primitiveConsts.Tpts.Brick,
                Name = "Brick",
                NoXFlipRendering = true,
                UseKinematic = false,
                BlPushbackAttenuation = 1.0f,
                DefaultBoxHalfSizeX = 16.0f,
                DefaultBoxHalfSizeY = 16.0f,
                DefaultLinearSpeed = 0,
                Hp = 200,
                AllowsRotationFromPhySys = true,
                Destructible = true,
                TakesGravity = true,
            };

            underlying = new MapField<uint, TrapConfig> {
                { SlidingPlatformTrap.Tpt, SlidingPlatformTrap },
                { RotatingPlatformTrap.Tpt, RotatingPlatformTrap },
                { ConveyorBeltTrap.Tpt, ConveyorBeltTrap },
                { BossDoorTrap.Tpt, BossDoorTrap },
                { SpringTrap.Tpt, SpringTrap },
                { BrickTrap.Tpt, BrickTrap },
            };

            return true;
        }

        public MapField<uint, TrapConfig> getUnderlying() {
            if (!lazyInit() || null == underlying) {
                throw new ArgumentNullException("Failed to initialize the underlying of PbTraps");
            }
            return underlying;
        }
    }
}
