namespace jtshared {
    public sealed partial class BulletConfig {

        public BulletConfig UpsertCancelTransit(int patternId, uint skillId) {
            if (this.CancelTransit.ContainsKey(patternId)) {
                this.CancelTransit.Remove(patternId);
            }
            this.CancelTransit.Add(patternId, skillId);
            return this;
        }

        public BulletConfig SetAllowsWalking(bool val) {
            this.AllowsWalking = val;
            return this;
        }

        public BulletConfig SetAllowsCrouching(bool val) {
            this.AllowsCrouching = val;
            return this;
        }

        public BulletConfig SetStartupVfxSpeciesId(int val) {
            this.StartupVfxSpeciesId = val;
            return this;
        }

        public BulletConfig SetStartupFrames(int val) {
            this.StartupFrames = val;
            return this;
        }

        public BulletConfig SetStartupInvinsibleFrames(int val) {
            this.StartupInvinsibleFrames = val;
            return this;
        }

        public BulletConfig SetActiveFrames(int val) {
            this.ActiveFrames = val;
            return this;
        }

        public BulletConfig SetHitStunFrames(int val) {
            this.HitStunFrames = val;
            return this;
        }

        public BulletConfig SetHitInvinsibleFrames(int val) {
            this.HitInvinsibleFrames = val;
            return this;
        }

        public BulletConfig SetFinishingFrames(int val) {
            this.FinishingFrames = val;
            return this;
        }

        public BulletConfig SetSpeed(int val) {
            this.Speed = val;
            return this;
        }

        public BulletConfig SetSpeedIfNotHit(int val) {
            this.SpeedIfNotHit = val;
            return this;
        }

        public BulletConfig SetHitboxOffsets(float hitboxOffsetX, float hitboxOffsetY) {
            this.HitboxOffsetX = hitboxOffsetX;
            this.HitboxOffsetY = hitboxOffsetY;
            return this;
        }

        public BulletConfig SetHitboxHalfSizes(float hitboxHalfSizeX, float hitboxHalfSizeY) {
            this.HitboxHalfSizeX = hitboxHalfSizeX;
            this.HitboxHalfSizeY = hitboxHalfSizeY;
            return this;
        }

        public BulletConfig SetSelfLockVel(float x, float y, int yWhenFlying) {
            this.SelfLockVelX = x;
            this.SelfLockVelY = y;
            this.SelfLockVelYWhenFlying = yWhenFlying;
            return this;
        }

        public BulletConfig SetDamage(int val) {
            this.Damage = val;
            return this;
        }

        public BulletConfig SetCancellableFrames(int st, int ed) {
            this.CancellableStFrame = st;
            this.CancellableEdFrame = ed;
            return this;
        }

        public BulletConfig SetPushbacks(float pushbackVelX, float pushbackVelY) {
            this.PushbackVelX = pushbackVelX;
            this.PushbackVelY = pushbackVelY;
            return this;
        }

        public BulletConfig SetSimultaneousMultiHitCnt(uint simultaneousMultiHitCnt) {
            this.SimultaneousMultiHitCnt = simultaneousMultiHitCnt;
            return this;
        }

        public BulletConfig SetTakesGravity(bool yesOrNo) {
            this.TakesGravity = yesOrNo;
            return this;
        }

        public BulletConfig SetRotateAlongVelocity(bool yesOrNo) {
            RotatesAlongVelocity = yesOrNo;
            return this;
        }

        public BulletConfig SetMhType(MultiHitType mhType) {
            MhType = mhType;
            return this;
        }

        public BulletConfig SetCollisionTypeMask(ulong val) {
            CollisionTypeMask = val;
            return this;
        }

        public BulletConfig SetOmitSoftPushback(bool val) {
            OmitSoftPushback = val;
            return this;
        }

        public BulletConfig SetRemainsUponHit(bool val) {
            RemainsUponHit = val;
            return this;
        }

        public BulletConfig SetIsPixelatedActiveVfx(bool val) {
            IsPixelatedActiveVfx = val;
            return this;
        }

        public BulletConfig SetActiveVfxSpeciesId(int val) {
            ActiveVfxSpeciesId = val;
            return this;
        }

        public bool isEmissionInducedMultiHit() {
            return (MultiHitType.FromEmission == MhType);
        }

        public BulletConfig SetBlowUp(bool val) {
            BlowUp = val;
            return this;
        }

        public BulletConfig SetAngularVel(float cosVal, float sinVal) {
            AngularFrameVelCos = cosVal;
            AngularFrameVelSin = sinVal;
            return this;
        }

        public BulletConfig SetMhInheritsSpin(bool val) {
            MhInheritsSpin = val;
            return this;
        }

        public BulletConfig SetElementalAttrs(uint val) {
            ElementalAttrs = val;
            return this;
        }
    }

    public sealed partial class Skill {
        
        public Skill AddHit(BulletConfig val) {
            Hits.Add(val);
            return this;
        }
    }
}
