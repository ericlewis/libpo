# PO-32 patch FX

[PO-32 API](../api/PO32.md) · [Synth signal path](../SYNTH.md)

Use [`po32_fx.h`](../../include/po32_fx.h) to evaluate a performance-effect
patch before starting a synth voice. The helpers implement normalized parameter
changes described by [Magnus Lidström](https://soniccharge.com/forum/topic/893-po-32-synth-engine-fx).
They are patch operations, not a complete effects sequencer or a hardware-accurate
playback model.

## Fixed patch operations

Pass a `po32_fx_patch_effect_t` to `po32_fx_apply_patch`:

| Operation | Patch change |
| --- | --- |
| `NONE` | Copy the patch |
| `DISTORTION` | Maximum distortion; reduce level by 1/8 |
| `SQUASH` | Increase both decays by 1/4; reduce level by 1/16 |
| `PUNCH` | Increase noise mix by 1/4 and oscillator decay by 1/8; reduce noise attack and decay by 1/4 |
| `FM` | Sine modulation, maximum depth, rate 3/4; increase attacks by 1/16 and reduce level by 1/16 |
| `GRANULAR_LEVEL` | Reduce level by 1/16; the caller must implement retriggers |
| `REVERSE` | Exponential noise envelope; increase both attacks by 1/2 |

Names in the table use the `PO32_FX_PATCH_` prefix in C. Fractions are of the
normalized slider range, not physical seconds, decibels or hertz. Reverse
changes the envelope; it does not reverse an audio buffer.

```c
#include <po.h>

po32_status_t start_fm_hit(po32_synth_voice_t *voice,
                         const po32_patch_params_t *base) {
    po32_patch_params_t effected;
    po32_status_t status;
    if (!voice)
        return PO32_ERR_INVALID_ARG;
    status = po32_fx_apply_patch(base, PO32_FX_PATCH_FM, &effected);
    if (status != PO32_OK)
        return status;
    po32_synth_voice_init(voice, 44100u, &effected, 100, 1.0f);
    return PO32_OK;
}
```

## Pitch and EQ snapshots

`po32_fx_pitch_patch(base, delta, out)` moves oscillator, noise and EQ frequency
sliders together. Envelope sliders move by `-delta/4`, and modulation rate by
`delta/2`. The host frequency map gives an octave delta of approximately
`0.10034333`; use its negative for an octave down. This does not change tempo
and is not a conversion from stored pitch bytes.

`po32_fx_eq_sweep_patch(base, position, out)` evaluates the sweep at a position
from 0 to 1 through a bar. Frequency spans its range; gain reaches maximum
halfway through. The caller owns the clock and decides whether to wrap or hold
the position.

## Contract and limits

All 21 input parameters must be finite and in `[0,1]`. Pitch delta must be in
`[-1,1]`; EQ position must be in `[0,1]`. Changed parameters saturate to `[0,1]`.
That boundary policy is defined for this API; device overflow behavior is not
established. Other parameters remain unchanged.

Exact in-place evaluation is supported. Failures leave output untouched.
Evaluate each snapshot from the original patch to avoid accumulating deltas.
For an effect across multiple sounds, evaluate each sound's base patch.

The synth snapshots coefficients when a voice starts. Evaluating another patch
does not change an already-running voice; reinitializing it restarts the hit.
Continuous sweeps require a future live-control path. Echo scheduling, triplet
selection, beat repeats, granular retriggers, morph interpolation and bouncing
ball timing are not implemented here.

These semantic enum values are **not serialized FX codes or UI button numbers**.
Keep raw pattern FX/reference bytes intact when editing backups. See the
[stored FX evidence](../formats/PO32_EFFECTS_STATE.md) for that separate question.
