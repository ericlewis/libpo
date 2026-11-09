#ifndef PO32_FX_H
#define PO32_FX_H

#include "po32.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Semantic patch transformations, not UI button numbers or serialized FX codes.
 * GRANULAR_LEVEL applies only the level change, not the repeated triggers.
 * REVERSE reshapes attacks; it does not reverse rendered audio. */
typedef enum po32_fx_patch_effect {
  PO32_FX_PATCH_NONE = 0,
  PO32_FX_PATCH_DISTORTION,
  PO32_FX_PATCH_SQUASH,
  PO32_FX_PATCH_PUNCH,
  PO32_FX_PATCH_FM,
  PO32_FX_PATCH_GRANULAR_LEVEL,
  PO32_FX_PATCH_REVERSE
} po32_fx_patch_effect_t;

/* All helpers evaluate a patch snapshot before synth voice initialization.
 * They do not update a running voice, sequence events, or edit backup FX bytes.
 * Use the original base for each evaluation to avoid accumulating adjustments.
 *
 * Every base parameter must be finite and in [0,1]. Modified parameters saturate
 * to [0,1]; other parameters are copied unchanged. Saturation is a host policy,
 * not a measured device overflow rule. Exact in-place use (base == out) is
 * supported; otherwise the structs must not overlap.
 * Null pointers return INVALID_ARG; invalid enum/control/patch values return
 * RANGE. Failure leaves out unchanged. No allocation or hosted runtime needed.
 */
po32_status_t po32_fx_apply_patch(const po32_patch_params_t *base, po32_fx_patch_effect_t effect,
                                  po32_patch_params_t *out);

/* Apply a normalized frequency-slider delta in [-1,1] to oscillator, noise and
 * EQ frequencies. Four envelope sliders move by -delta/4; ModRate by delta/2.
 * This is not a raw knob value or a semitone value. Under the host synth's
 * 20..20000 Hz mapping, one octave is log10(2)/3, approximately 0.10034333,
 * before saturation. Scheduling, phase and tempo remain caller-owned. */
po32_status_t po32_fx_pitch_patch(const po32_patch_params_t *base, float delta,
                                  po32_patch_params_t *out);

/* Evaluate the EQ sweep at caller-supplied position [0,1] through one 4/4 bar.
 * Frequency spans its full range. Gain rises from neutral to maximum over the
 * first half, then stays at maximum. No automatic wrapping or clock state. */
po32_status_t po32_fx_eq_sweep_patch(const po32_patch_params_t *base, float position,
                                     po32_patch_params_t *out);

#ifdef __cplusplus
}
#endif
#endif
