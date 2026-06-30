using jtshared;
using UnityEngine;
using JoltCSharp;

public class JoltInplaceHpBarAnimController : AbstractCacheableAnimNode<CharacterDownsync, CharacterState, CharacterConfig, uint> {

    protected Vector2 newSizeHolder = new Vector2();

    protected const int HP_PER_SECTION = 100;
    protected float HP_PER_SECTION_F = (float)HP_PER_SECTION;

    protected float DEFAULT_HP100_WIDTH = 32.0f;
    protected float DEFAULT_HP100_HEIGHT = 5.0f;
    protected float DEFAULT_HOLDER_PADDING = 5.0f;

    protected static Color[] hpColors = new Color[] {
        new Color(0x77 / 255f, 0xE9 / 255f, 0x35 / 255f),
        new Color(0x25 / 255f, 0x56 / 255f, 0x26 / 255f),
        new Color(0x4f / 255f, 0x57 / 255f, 0x18 / 255f),
        new Color(0x93 / 255f, 0x90 / 255f, 0x25 / 255f),
        new Color(0x8f / 255f, 0xC4 / 255f, 0x62 / 255f),
        new Color(0xf0 / 255f, 0xA6 / 255f, 0x08 / 255f),
        new Color(0x6E / 255f, 0xC2 / 255f, 0xBD / 255f),
    };

    public JoltInplaceHpBarAnimController() {
        SetUd(PbPrimitivesOverride.Instance.getUnderlying().TerminatingCharacterId);
        SetCacheGroupId(PbPrimitivesOverride.Instance.getUnderlying().ChSpecies.None);
    }

    public SpriteRenderer hpFiller;
    public SpriteRenderer overflowHpFiller;
    public SpriteRenderer hpHolder;

    public void updateHpByValsAndCaps(int hp, int hpCap) {
        float newHolderWidth = (HP_PER_SECTION >= hpCap ? DEFAULT_HP100_WIDTH * (hpCap / HP_PER_SECTION_F) : DEFAULT_HP100_WIDTH);
        newSizeHolder.Set(newHolderWidth, DEFAULT_HP100_HEIGHT);
        hpFiller.size = newSizeHolder;
        newSizeHolder.Set(newHolderWidth+DEFAULT_HOLDER_PADDING, DEFAULT_HP100_HEIGHT);
        hpHolder.size = newSizeHolder;

        float newOverflowFillerWidth = (HP_PER_SECTION >= hpCap ? 0 : DEFAULT_HP100_WIDTH);
        newSizeHolder.Set(newOverflowFillerWidth, DEFAULT_HP100_HEIGHT);
        overflowHpFiller.size = newSizeHolder;

        if (HP_PER_SECTION >= hp) {
            int baseHpSectionIdx = 0;
            var baseColor = hpColors[baseHpSectionIdx];

            float hpNewScaleX = (float)hp / (HP_PER_SECTION < hpCap ? HP_PER_SECTION : hpCap);
            scaleHolder.Set(hpNewScaleX, hpFiller.transform.localScale.y, hpFiller.transform.localScale.z);

            hpFiller.transform.localScale = scaleHolder;
            hpFiller.color = baseColor;

            newSizeHolder.Set(0, DEFAULT_HP100_HEIGHT);
            overflowHpFiller.size = newSizeHolder;
        } else {
            int overwhelmedHpSectionIdx = (hp / HP_PER_SECTION);
            var overwhelmedColor = hpColors[overwhelmedHpSectionIdx];

            int baseHpSectionIdx = overwhelmedHpSectionIdx - 1;
            var baseColor = hpColors[baseHpSectionIdx];

            scaleHolder.Set(1.0f, hpFiller.transform.localScale.y, hpFiller.transform.localScale.z);
            hpFiller.transform.localScale = scaleHolder;
            hpFiller.color = baseColor;

            int overwhelmedHp = hp - (overwhelmedHpSectionIdx * HP_PER_SECTION);
            float overwhelmedHpNewScaleX = (float)overwhelmedHp / HP_PER_SECTION_F;
            scaleHolder.Set(overwhelmedHpNewScaleX, overflowHpFiller.transform.localScale.y, overflowHpFiller.transform.localScale.z);
            overflowHpFiller.transform.localScale = scaleHolder;
            overflowHpFiller.color = overwhelmedColor;
        }
    }

    protected override bool updateAnimUnderlying(in int rdfId, in CharacterDownsync rdfCharacter, in CharacterState newCharacterState, in CharacterConfig chConfig, in int framesInNewState) {
        SetCacheGroupId(chConfig.SpeciesId);
        updateHpByValsAndCaps(rdfCharacter.Hp, chConfig.Hp);
        return true;
    }

    protected override bool lazyInit() {
        // All should be handled in prefab binding.
        return true;
    }
}
