using Google.Protobuf;
using JoltCSharp;
using System;
using UnityEngine;
using UnityEngine.AddressableAssets;

public class SFXSourceAnimController : AbstractCacheableAnimNode<IMessage, Enum, IMessage, string> {

    private float realtimeAttenuation = 0f;
    public void SetRealtimeAttenuation(in float val) {
        realtimeAttenuation = val;
    }

    public SFXSourceAnimController() {
        SetUd(0);
        SetCacheGroupId("");
    }

    protected new Animator getMainAnimator() {
        return null;
    }

    protected override bool lazyInit() {
        if (null != audioClip && null != audioSource) {
            return true;
        }

        audioSource = GetComponent<AudioSource>();
        if (null == audioSource) {
            audioSource = gameObject.AddComponent<AudioSource>();
        }
        audioSource.volume = PlayerSettingsManager.Instance.GetSfxVolume();

        var clipName = GetCacheGroupId();
        var handle = Addressables.LoadAssetAsync<AudioClip>($"SFX/{clipName}");
        audioClip = handle.WaitForCompletion();
        audioSource.clip = audioClip;

        return true;
    }

    protected override bool updateAnimUnderlying(in int currRdfId, in IMessage ins, in Enum newTargetState, in IMessage insConfig, in int frameIdxInAnim) {
        audioSource.volume = PlayerSettingsManager.Instance.GetSfxVolume() * (1.0f - realtimeAttenuation);
        float timeInSecond = frameIdxInAnim * PbPrimitivesOverride.Instance.getUnderlying().EstimatedSecondsPerRdf;
        if (audioSource.isPlaying) {
            if (timeInSecond > audioClip.length) {
                audioSource.Stop();
            } else {
                audioSource.time = timeInSecond;
            }
        } else {
            if (timeInSecond <= audioClip.length) {
               audioSource.time = timeInSecond;
               audioSource.Play();
            }
        }

        return true;
    }

    /////////////////////////////////////////////////////////////////////////////////////////
    protected override void Start() {
        base.Start();
    }

    protected AudioClip audioClip;
    protected AudioSource audioSource;
}
