#ifdef NDEBUG
#undef NDEBUG
#endif
#include "po32_fx.h"
#include "po32_synth.h"

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Member offsets avoid treating distinct struct members as an array. */
static const size_t fields[] = {
    offsetof(po32_patch_params_t, OscWave), offsetof(po32_patch_params_t, OscFreq),
    offsetof(po32_patch_params_t, OscAtk),  offsetof(po32_patch_params_t, OscDcy),
    offsetof(po32_patch_params_t, ModMode), offsetof(po32_patch_params_t, ModRate),
    offsetof(po32_patch_params_t, ModAmt),  offsetof(po32_patch_params_t, NFilMod),
    offsetof(po32_patch_params_t, NFilFrq), offsetof(po32_patch_params_t, NFilQ),
    offsetof(po32_patch_params_t, NEnvMod), offsetof(po32_patch_params_t, NEnvAtk),
    offsetof(po32_patch_params_t, NEnvDcy), offsetof(po32_patch_params_t, Mix),
    offsetof(po32_patch_params_t, DistAmt), offsetof(po32_patch_params_t, EQFreq),
    offsetof(po32_patch_params_t, EQGain),  offsetof(po32_patch_params_t, Level),
    offsetof(po32_patch_params_t, OscVel),  offsetof(po32_patch_params_t, NVel),
    offsetof(po32_patch_params_t, ModVel)};

static void set_field(po32_patch_params_t *p, size_t index, float value) {
  memcpy((unsigned char *)p + fields[index], &value, sizeof(value));
}

static float get_field(const po32_patch_params_t *p, size_t index) {
  float value;
  memcpy(&value, (const unsigned char *)p + fields[index], sizeof(value));
  return value;
}

static void fill(po32_patch_params_t *p, float value) {
  size_t i;
  memset(p, 0, sizeof(*p));
  for (i = 0; i < PO32_PARAM_COUNT; ++i)
    set_field(p, i, value);
}

static void same(const po32_patch_params_t *a, const po32_patch_params_t *b) {
  size_t i;
  for (i = 0; i < PO32_PARAM_COUNT; ++i)
    assert(get_field(a, i) == get_field(b, i));
}

static void fixed_effects(void) {
  po32_patch_params_t base, expected, out, original;
  int effect;
  fill(&base, 0.5f);
  base.OscWave = 0.25f;
  base.NEnvMod = 0.75f;
  base.ModMode = 0.25f;
  original = base;
  for (effect = PO32_FX_PATCH_NONE; effect <= PO32_FX_PATCH_REVERSE; ++effect) {
    expected = base;
    switch (effect) {
    case PO32_FX_PATCH_DISTORTION:
      expected.DistAmt = 1.0f;
      expected.Level = 0.375f;
      break;
    case PO32_FX_PATCH_SQUASH:
      expected.OscDcy = expected.NEnvDcy = 0.75f;
      expected.Level = 0.4375f;
      break;
    case PO32_FX_PATCH_PUNCH:
      expected.Mix = 0.75f;
      expected.NEnvAtk = expected.NEnvDcy = 0.25f;
      expected.OscDcy = 0.625f;
      break;
    case PO32_FX_PATCH_FM:
      expected.ModMode = 0.5f;
      expected.ModAmt = 1.0f;
      expected.ModRate = 0.75f;
      expected.OscAtk = expected.NEnvAtk = 0.5625f;
      expected.Level = 0.4375f;
      break;
    case PO32_FX_PATCH_GRANULAR_LEVEL:
      expected.Level = 0.4375f;
      break;
    case PO32_FX_PATCH_REVERSE:
      expected.NEnvMod = 0.0f;
      expected.OscAtk = expected.NEnvAtk = 1.0f;
      break;
    default:
      assert(effect == PO32_FX_PATCH_NONE);
    }
    assert(po32_fx_apply_patch(&base, (po32_fx_patch_effect_t)effect, &out) == PO32_OK);
    same(&out, &expected);
    same(&base, &original);
    out = base;
    assert(po32_fx_apply_patch(&out, (po32_fx_patch_effect_t)effect, &out) == PO32_OK);
    same(&out, &expected);
  }
}

static void pitch_and_sweep(void) {
  po32_patch_params_t base, expected, out;
  fill(&base, 0.5f);
  expected = base;
  expected.OscFreq = expected.NFilFrq = expected.EQFreq = 0.625f;
  expected.OscAtk = expected.OscDcy = expected.NEnvAtk = expected.NEnvDcy = 0.46875f;
  expected.ModRate = 0.5625f;
  assert(po32_fx_pitch_patch(&base, 0.125f, &out) == PO32_OK);
  same(&out, &expected);
  assert(po32_fx_pitch_patch(&out, -0.125f, &out) == PO32_OK);
  same(&out, &base);
  assert(po32_fx_pitch_patch(&base, 0.0f, &out) == PO32_OK);
  same(&out, &base);

  expected = base;
  expected.EQFreq = 0.0f;
  expected.EQGain = 0.5f;
  assert(po32_fx_eq_sweep_patch(&base, 0.0f, &out) == PO32_OK);
  same(&out, &expected);
  expected.EQFreq = 0.25f;
  expected.EQGain = 0.75f;
  assert(po32_fx_eq_sweep_patch(&base, 0.25f, &out) == PO32_OK);
  same(&out, &expected);
  expected.EQFreq = 0.5f;
  expected.EQGain = 1.0f;
  assert(po32_fx_eq_sweep_patch(&base, 0.5f, &out) == PO32_OK);
  same(&out, &expected);
  expected.EQFreq = 0.75f;
  assert(po32_fx_eq_sweep_patch(&base, 0.75f, &out) == PO32_OK);
  same(&out, &expected);
  expected.EQFreq = 1.0f;
  out = base;
  assert(po32_fx_eq_sweep_patch(&out, 1.0f, &out) == PO32_OK);
  same(&out, &expected);
}

static void saturation(void) {
  po32_patch_params_t base, out;
  int effect, edge;
  size_t i;
  for (edge = 0; edge <= 1; ++edge) {
    fill(&base, (float)edge);
    for (effect = PO32_FX_PATCH_NONE; effect <= PO32_FX_PATCH_REVERSE; ++effect) {
      assert(po32_fx_apply_patch(&base, (po32_fx_patch_effect_t)effect, &out) == PO32_OK);
      for (i = 0; i < PO32_PARAM_COUNT; ++i) {
        float v = get_field(&out, i);
        assert(v >= 0.0f && v <= 1.0f);
      }
    }
  }
  fill(&base, 0.5f);
  assert(po32_fx_pitch_patch(&base, 1.0f, &out) == PO32_OK);
  assert(out.OscFreq == 1.0f && out.NFilFrq == 1.0f && out.EQFreq == 1.0f);
  assert(out.OscAtk == 0.25f && out.ModRate == 1.0f);
  assert(po32_fx_pitch_patch(&base, -1.0f, &out) == PO32_OK);
  assert(out.OscFreq == 0.0f && out.NFilFrq == 0.0f && out.EQFreq == 0.0f);
  assert(out.OscAtk == 0.75f && out.ModRate == 0.0f);
}

static void rejection(void) {
  po32_patch_params_t base, out, before;
  const float invalid[] = {-0.01f, 1.01f, NAN, INFINITY, -INFINITY};
  size_t i, j;
  fill(&before, 0.375f);
  out = before;
  for (i = 0; i < PO32_PARAM_COUNT; ++i) {
    for (j = 0; j < sizeof(invalid) / sizeof(invalid[0]); ++j) {
      fill(&base, 0.5f);
      set_field(&base, i, invalid[j]);
      assert(po32_fx_apply_patch(&base, PO32_FX_PATCH_NONE, &out) == PO32_ERR_RANGE);
      assert(memcmp(&out, &before, sizeof(out)) == 0);
      assert(po32_fx_pitch_patch(&base, 0.125f, &out) == PO32_ERR_RANGE);
      assert(memcmp(&out, &before, sizeof(out)) == 0);
      assert(po32_fx_eq_sweep_patch(&base, 0.25f, &out) == PO32_ERR_RANGE);
      assert(memcmp(&out, &before, sizeof(out)) == 0);
    }
  }
  fill(&base, 0.5f);
  for (j = 0; j < sizeof(invalid) / sizeof(invalid[0]); ++j) {
    float bad_delta = invalid[j] < 0.0f ? invalid[j] - 1.0f : invalid[j];
    assert(po32_fx_pitch_patch(&base, bad_delta, &out) == PO32_ERR_RANGE);
    assert(memcmp(&out, &before, sizeof(out)) == 0);
    assert(po32_fx_eq_sweep_patch(&base, invalid[j], &out) == PO32_ERR_RANGE);
    assert(memcmp(&out, &before, sizeof(out)) == 0);
  }
  assert(po32_fx_apply_patch(&base, (po32_fx_patch_effect_t)-1, &out) == PO32_ERR_RANGE);
  assert(po32_fx_apply_patch(&base, (po32_fx_patch_effect_t)99, &out) == PO32_ERR_RANGE);
  assert(memcmp(&out, &before, sizeof(out)) == 0);
  assert(po32_fx_apply_patch(NULL, PO32_FX_PATCH_NONE, &out) == PO32_ERR_INVALID_ARG);
  assert(po32_fx_apply_patch(&base, PO32_FX_PATCH_NONE, NULL) == PO32_ERR_INVALID_ARG);
  assert(po32_fx_pitch_patch(NULL, 0.0f, &out) == PO32_ERR_INVALID_ARG);
  assert(po32_fx_pitch_patch(&base, 0.0f, NULL) == PO32_ERR_INVALID_ARG);
  assert(po32_fx_eq_sweep_patch(NULL, 0.0f, &out) == PO32_ERR_INVALID_ARG);
  assert(po32_fx_eq_sweep_patch(&base, 0.0f, NULL) == PO32_ERR_INVALID_ARG);
  assert(memcmp(&out, &before, sizeof(out)) == 0);
}

static size_t crossing_count(const float *audio, size_t count) {
  size_t i, crossings = 0;
  for (i = 1; i < count; ++i)
    if (audio[i - 1] <= 0 && audio[i] > 0)
      ++crossings;
  return crossings;
}

static void render_integration(void) {
  po32_patch_params_t base, changed;
  po32_synth_voice_t voice;
  float original[8000], shifted[8000], chunked[8000];
  size_t n, used, got, i;
  fill(&base, 0.0f);
  base.OscFreq = 0.5f;
  base.OscDcy = 0.75f;
  base.ModAmt = 0.5f;
  base.Level = 0.5f;
  base.EQGain = 0.5f;
  assert(po32_fx_pitch_patch(&base, -0.1003433319f, &changed) == PO32_OK);
  po32_synth_voice_init(&voice, 16000, &base, 100, 0.5f);
  assert(po32_synth_voice_render_f32(&voice, original, 8000, &n) == PO32_OK && n == 8000);
  po32_synth_voice_init(&voice, 16000, &changed, 100, 0.5f);
  assert(po32_synth_voice_render_f32(&voice, shifted, 8000, &n) == PO32_OK && n == 8000);
  assert(fabs((double)crossing_count(original, n) / (double)crossing_count(shifted, n) - 2) < 0.03);

  assert(po32_fx_apply_patch(&base, PO32_FX_PATCH_FM, &changed) == PO32_OK);
  po32_synth_voice_init(&voice, 16000, &changed, 100, 0.5f);
  assert(voice.mod_is_sine && voice.mod_pitch_scale == 4.0f);
  assert(fabsf(voice.mod_rot_c - cosf(2.0f * 3.14159265358979323846f * 200.028244f / 16000)) <
         1e-6f);
  assert(po32_synth_voice_render_f32(&voice, shifted, 8000, &n) == PO32_OK && n == 8000);
  po32_synth_voice_reset(&voice);
  for (used = 0; used < n; used += got) {
    size_t request = n - used < 113 ? n - used : 113;
    assert(po32_synth_voice_render_f32(&voice, chunked + used, request, &got) == PO32_OK);
    assert(got == request);
  }
  assert(memcmp(chunked, shifted, sizeof(shifted)) == 0);
  for (i = 0; i < n; ++i)
    assert(isfinite(shifted[i]) && shifted[i] >= -1 && shifted[i] <= 1);
}

int main(void) {
  fixed_effects();
  pitch_and_sweep();
  saturation();
  rejection();
  render_integration();
  puts("PO-32 semantic patch FX tests passed");
  return 0;
}
