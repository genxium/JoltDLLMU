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
                UseKinematic = true,
                UseObstableInterfaceBody = true,
                DefaultBoxHalfSizeX = 100.0f,
                DefaultBoxHalfSizeY = 100.0f,
                DefaultLinearSpeed = 7.0f*BATTLE_DYNAMICS_FPS, 
                DefaultCooldownRdfCount = 60
            };

            TrapConfig RotatingPlatformTrap = new TrapConfig {
                Tpt = primitiveConsts.Tpts.RotatingPlatform,
                Name = "RotatingPlatform",
                NoXFlipRendering = true,
                UseKinematic = true,
                UseObstableInterfaceBody = false,
                DefaultBoxHalfSizeX = 100.0f,
                DefaultBoxHalfSizeY = 100.0f,
                DefaultLinearSpeed = 0, 
                DefaultCooldownRdfCount = 60
            };

            TrapConfig ConveyorBeltTrap = new TrapConfig {
                Tpt = primitiveConsts.Tpts.ConveyorBelt,
                Name = "ConveyorBelt",
                NoXFlipRendering = false,
                UseKinematic = true,
                UseObstableInterfaceBody = false,
                DefaultBoxHalfSizeX = 100.0f,
                DefaultBoxHalfSizeY = 100.0f,
                DefaultLinearSpeed = 0, 
                DefaultCooldownRdfCount = 60
            };

            TrapConfig FallingRockTrap = new TrapConfig {
                Tpt = primitiveConsts.Tpts.FallingRock,
                Name = "FallingRock",
                NoXFlipRendering = false,
                UseKinematic = false,
                DefaultBoxHalfSizeX = 100.0f,
                DefaultBoxHalfSizeY = 100.0f,
                DefaultLinearSpeed = 0, 
                DefaultCooldownRdfCount = 60
            };

            TrapConfig BrickTrap = new TrapConfig {
                Tpt = primitiveConsts.Tpts.Brick,
                Name = "Brick",
                NoXFlipRendering = true,
                UseKinematic = true,
                DefaultBoxHalfSizeX = 16.0f,
                DefaultBoxHalfSizeY = 16.0f,
                DefaultLinearSpeed = 0,
            };

            underlying = new MapField<uint, TrapConfig> {
                { SlidingPlatformTrap.Tpt, SlidingPlatformTrap },
                { RotatingPlatformTrap.Tpt, RotatingPlatformTrap },
                { ConveyorBeltTrap.Tpt, ConveyorBeltTrap },
                { FallingRockTrap.Tpt, FallingRockTrap },
                { BrickTrap.Tpt, BrickTrap }
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
