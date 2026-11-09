#include "po32_fx.h"

static int normalized(float value) {
  /* Ordered comparisons reject NaN and both infinities without libm. */
  return value >= 0.0f && value <= 1.0f;
}

static int valid_patch(const po32_patch_params_t *p) {
  return normalized(p->OscWave) && normalized(p->OscFreq) && normalized(p->OscAtk) &&
         normalized(p->OscDcy) && normalized(p->ModMode) && normalized(p->ModRate) &&
         normalized(p->ModAmt) && normalized(p->NFilMod) && normalized(p->NFilFrq) &&
         normalized(p->NFilQ) && normalized(p->NEnvMod) && normalized(p->NEnvAtk) &&
         normalized(p->NEnvDcy) && normalized(p->Mix) && normalized(p->DistAmt) &&
         normalized(p->EQFreq) && normalized(p->EQGain) && normalized(p->Level) &&
         normalized(p->OscVel) && normalized(p->NVel) && normalized(p->ModVel);
}

static float saturate(float value) {
  if (value < 0.0f)
    return 0.0f;
  if (value > 1.0f)
    return 1.0f;
  return value;
}

po32_status_t po32_fx_apply_patch(const po32_patch_params_t *base, po32_fx_patch_effect_t effect,
                                  po32_patch_params_t *out) {
  po32_patch_params_t p;
  if (!base || !out)
    return PO32_ERR_INVALID_ARG;
  if (!valid_patch(base))
    return PO32_ERR_RANGE;
  p = *base;
  switch (effect) {
  case PO32_FX_PATCH_NONE:
    break;
  case PO32_FX_PATCH_DISTORTION:
    p.DistAmt = 1.0f;
    p.Level = saturate(p.Level - 0.125f);
    break;
  case PO32_FX_PATCH_SQUASH:
    p.OscDcy = saturate(p.OscDcy + 0.25f);
    p.NEnvDcy = saturate(p.NEnvDcy + 0.25f);
    p.Level = saturate(p.Level - 0.0625f);
    break;
  case PO32_FX_PATCH_PUNCH:
    p.Mix = saturate(p.Mix + 0.25f);
    p.NEnvAtk = saturate(p.NEnvAtk - 0.25f);
    p.NEnvDcy = saturate(p.NEnvDcy - 0.25f);
    p.OscDcy = saturate(p.OscDcy + 0.125f);
    break;
  case PO32_FX_PATCH_FM:
    p.ModMode = 0.5f;
    p.ModAmt = 1.0f;
    p.ModRate = 0.75f;
    p.OscAtk = saturate(p.OscAtk + 0.0625f);
    p.NEnvAtk = saturate(p.NEnvAtk + 0.0625f);
    p.Level = saturate(p.Level - 0.0625f);
    break;
  case PO32_FX_PATCH_GRANULAR_LEVEL:
    p.Level = saturate(p.Level - 0.0625f);
    break;
  case PO32_FX_PATCH_REVERSE:
    p.NEnvMod = 0.0f;
    p.OscAtk = saturate(p.OscAtk + 0.5f);
    p.NEnvAtk = saturate(p.NEnvAtk + 0.5f);
    break;
  default:
    return PO32_ERR_RANGE;
  }
  *out = p;
  return PO32_OK;
}

po32_status_t po32_fx_pitch_patch(const po32_patch_params_t *base, float delta,
                                  po32_patch_params_t *out) {
  po32_patch_params_t p;
  if (!base || !out)
    return PO32_ERR_INVALID_ARG;
  if (!valid_patch(base) || !(delta >= -1.0f && delta <= 1.0f))
    return PO32_ERR_RANGE;
  p = *base;
  p.OscFreq = saturate(p.OscFreq + delta);
  p.NFilFrq = saturate(p.NFilFrq + delta);
  p.EQFreq = saturate(p.EQFreq + delta);
  p.OscAtk = saturate(p.OscAtk - delta * 0.25f);
  p.OscDcy = saturate(p.OscDcy - delta * 0.25f);
  p.NEnvAtk = saturate(p.NEnvAtk - delta * 0.25f);
  p.NEnvDcy = saturate(p.NEnvDcy - delta * 0.25f);
  p.ModRate = saturate(p.ModRate + delta * 0.5f);
  *out = p;
  return PO32_OK;
}

po32_status_t po32_fx_eq_sweep_patch(const po32_patch_params_t *base, float position,
                                     po32_patch_params_t *out) {
  po32_patch_params_t p;
  if (!base || !out)
    return PO32_ERR_INVALID_ARG;
  if (!valid_patch(base) || !normalized(position))
    return PO32_ERR_RANGE;
  p = *base;
  p.EQFreq = position;
  p.EQGain = saturate(0.5f + position);
  *out = p;
  return PO32_OK;
}
