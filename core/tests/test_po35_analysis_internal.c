#ifdef NDEBUG
#undef NDEBUG
#endif
#include "../src/po35_analysis.c"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static po35_analysis_workspace_t workspace;

static void test_lar_quantization_bounds(void) {
  /* The clamped interval is monotonic: round(64/3 * log((1+k)/(1-k)))
   * ranges from -128 to 128. Only the positive endpoint needs saturation. */
  for (int i = -9950; i <= 9950; ++i) {
    double k = i / 10000.0;
    int expected = (int)round((64.0 / 3.0) * log((1 + k) / (1 - k)));
    int actual = nearest((64.0 / 3.0) * po_math_log((1 + k) / (1 - k)));
    assert(actual == expected && actual >= -128 && actual <= 128);
  }
}

static void test_gain_quantization(void) {
  static const double amplitudes[] = {0, 1.0 / 65536, .25, 1, 3.9, 4, 16, DBL_MAX};
  static const uint8_t expected[] = {0, 1, 64, 128, 253, 255, 255, 255};
  for (unsigned i = 0; i < sizeof(amplitudes) / sizeof(*amplitudes); ++i) {
    uint8_t actual = quantize_gain(amplitudes[i]);
    assert(actual == expected[i]);
  }
}

static void test_quiet_pitch_interpolation(void) {
  double periodicity, frequency;
  setup(&workspace, 24000);
  for (unsigned i = 0; i < 1024; ++i)
    workspace.high[i] = 1e-7 * sin(2 * PO_MATH_PI * 440 * i / 24000.0);
  frequency = pitch(&workspace, &periodicity);
  /* At tiny correlation magnitudes, use the peak's integer lag rather than
   * amplifying rounding error with a nearly-zero interpolation denominator. */
  assert(fabs(frequency - 24000.0 / 55.0) < 1e-12);
  assert(periodicity > .95 && periodicity <= 1.0);
  for (unsigned i = 0; i < 1024; ++i)
    workspace.high[i] = .2 * sin(2 * PO_MATH_PI * 440 * i / 24000.0);
  frequency = pitch(&workspace, &periodicity);
  assert(fabs(frequency - 440) < 1.0);
}

static double reference_power(const uint8_t *bytes, unsigned order, double rate, double f0,
                              double uv) {
  double coefficients[31] = {1}, temporary[31], residual = 1, sum = 0;
  unsigned harmonics = (unsigned)(rate * .5 / f0);
  for (unsigned i = 1; i <= order; ++i) {
    int q = bytes[i - 1] < 128 ? bytes[i - 1] : (int)bytes[i - 1] - 256;
    double reflection = tanh(q * 3.0 / 128.0);
    residual *= 1 - reflection * reflection;
    for (unsigned j = 1; j < i; ++j)
      temporary[j] = coefficients[j] + reflection * coefficients[i - j];
    for (unsigned j = 1; j < i; ++j)
      coefficients[j] = temporary[j];
    coefficients[i] = reflection;
  }
  if (!harmonics)
    return uv;
  for (unsigned h = 1; h <= harmonics; ++h) {
    double re = 1, im = 0;
    for (unsigned i = 1; i <= order; ++i) {
      double angle = 2 * PO_MATH_PI * f0 * h * i / rate;
      re += coefficients[i] * cos(angle);
      im += coefficients[i] * sin(angle);
    }
    sum += 1 / fmax(1e-20, re * re + im * im);
  }
  return uv + (1 - uv) * residual * sum / harmonics;
}

static void test_power_model(void) {
  uint8_t coefficients[30] = {0};
  static const unsigned orders[] = {10, 30};
  for (unsigned bank = 0; bank < 2; ++bank) {
    unsigned order = orders[bank];
    double rate = bank ? 24000 : 8000;
    /* No filter has unit power; frequencies above Nyquist have no voiced harmonics. */
    double actual = output_power(&workspace, coefficients, order, rate, 400, .25);
    assert(fabs(actual - 1) < 1e-14);
    actual = output_power(&workspace, coefficients, order, rate, rate, .25);
    assert(actual == .25);
    for (unsigned i = 0; i < order; ++i)
      coefficients[i] = (uint8_t)(3 * ((int)(i % 5) - 2));
    for (unsigned unvoiced = 0; unvoiced < 3; ++unvoiced) {
      double uv = .5 * unvoiced;
      actual = output_power(&workspace, coefficients, order, rate, 440, uv);
      double expected = reference_power(coefficients, order, rate, 440, uv);
      assert(fabs(actual - expected) < 1e-12);
    }
    memset(coefficients, 0, sizeof(coefficients));
  }
  /* A legal extreme LAR bank has a near-zero response denominator at Nyquist.
   * The power floor prevents division by cancellation noise. */
  memset(coefficients, 128, 10);
  double actual = output_power(&workspace, coefficients, 10, 8000, 4000, 0);
  double reflection = tanh(-3.0);
  double expected = pow(1 - reflection * reflection, 10) / 1e-20;
  assert(isfinite(actual) && fabs(actual / expected - 1) < 1e-12);
}

/* Independent Toeplitz evaluation by diagonals, rather than the fitter's
 * matrix traversal. The quadratic is the prediction error for this covariance. */
static double reference_loss(const double *polynomial, const double *covariance) {
  double result = 0.0;
  for (unsigned i = 0; i <= 30; ++i)
    result += polynomial[i] * polynomial[i] * covariance[0];
  for (unsigned lag = 1; lag <= 30; ++lag) {
    double cross = 0.0;
    for (unsigned i = lag; i <= 30; ++i)
      cross += polynomial[i] * polynomial[i - lag];
    result += 2.0 * covariance[lag] * cross;
  }
  return result;
}

static void test_target_covariance(void) {
  double target[31] = {0}, covariance[31], polynomial[31], temporary[31];
  double recovered[31], next[31];
  for (unsigned trial = 0; trial < 9; ++trial) {
    target[1] = trial ? (trial & 1 ? -.9985 : .9985) : 0.0;
    for (unsigned i = 2; i <= 30; ++i)
      target[i] = trial ? .24 * sin(i * 3.0 + trial * 7.0) / sqrt(i) : 0.0;
    assert(lpc_target_covariance(target, covariance, polynomial, temporary));
    double error = 1.0;
    recovered[0] = 1.0;
    /* Solve the recovered covariance independently with forward Levinson. */
    for (unsigned i = 1; i <= 30; ++i) {
      double numerator = covariance[i];
      for (unsigned j = 1; j < i; ++j)
        numerator += recovered[j] * covariance[i - j];
      double k = -numerator / error;
      assert(fabs(k - target[i]) < 2e-10);
      for (unsigned j = 1; j < i; ++j)
        next[j] = recovered[j] + k * recovered[i - j];
      for (unsigned j = 1; j < i; ++j)
        recovered[j] = next[j];
      recovered[i] = k;
      error *= 1 - k * k;
    }
    assert(fabs(reference_loss(polynomial, covariance) - error) < 2e-10);
    for (unsigned i = 1; i <= 30; ++i) {
      double gradient = 0.0;
      for (unsigned j = 0; j <= 30; ++j)
        gradient += polynomial[j] * covariance[i > j ? i - j : j - i];
      assert(fabs(gradient) < 2e-10);
    }
  }
  target[1] = 1.0;
  assert(!lpc_target_covariance(target, covariance, polynomial, temporary));
  target[1] = NAN;
  assert(!lpc_target_covariance(target, covariance, polynomial, temporary));
}

static void test_coordinate_minimization(void) {
  double target[31] = {0}, covariance[31], polynomial[31], temporary[31];
  double reflection[31], work[31], scratch[124];
  uint8_t baseline[30], codes[30];
  for (int code = -128; code <= 127; ++code) {
    double k = po35_lar_reflection[(uint8_t)code];
    assert(isfinite(k) && fabs(k) < 1.0);
    if (code > -128)
      assert(k > po35_lar_reflection[(uint8_t)(code - 1)]);
  }
  for (unsigned trial = 0; trial < 8; ++trial) {
    target[1] = trial & 1 ? -.9985 : .9985;
    for (unsigned i = 2; i <= 30; ++i)
      target[i] = .24 * sin(i * 3.0 + trial * 7.0) / sqrt(i);
    assert(lpc_target_covariance(target, covariance, polynomial, temporary));
    baseline[0] = (uint8_t)(trial & 1 ? -128 : 127);
    reflection[0] = 0.0;
    for (unsigned i = 1; i < 30; ++i)
      baseline[i] = (uint8_t)((int)((i * 11 + trial * 3) % 41) - 20);
    for (unsigned i = 0; i < 30; ++i)
      reflection[i + 1] = po35_lar_reflection[baseline[i]];
    for (unsigned coordinate = 2; coordinate <= 30; coordinate += 4) {
      memcpy(work, reflection, sizeof(work));
      memcpy(codes, baseline, sizeof(codes));
      lpc_step_up(work, polynomial, temporary);
      double original = lpc_bilinear(polynomial, polynomial, covariance), selected = original;
      int result =
          lpc_fit_coordinate(covariance, work, codes, coordinate, &selected, scratch, temporary);
      assert(result >= 0);
      assert(selected <= original && codes[0] == baseline[0]);
      /* Scratch A/B also independently predict the polynomial and quadratic
       * away from both endpoints used to derive the coordinate slice. */
      static const double points[] = {-.73, .21, .993};
      for (unsigned p = 0; p < 3; ++p) {
        work[coordinate] = points[p];
        lpc_step_up(work, polynomial, temporary);
        for (unsigned i = 0; i <= 30; ++i)
          assert(fabs(polynomial[i] - (scratch[i] + points[p] * scratch[31 + i])) < 2e-12);
        double aa = lpc_bilinear(scratch, scratch, covariance);
        double ba = lpc_bilinear(scratch + 31, scratch, covariance);
        double bb = lpc_bilinear(scratch + 31, scratch + 31, covariance);
        assert(bb > 0.0);
        assert(fabs(reference_loss(polynomial, covariance) -
                    (aa + 2 * points[p] * ba + points[p] * points[p] * bb)) < 2e-10);
      }
      for (int code = -128; code <= 127; ++code) {
        work[coordinate] = po35_lar_reflection[(uint8_t)code];
        lpc_step_up(work, polynomial, temporary);
        double exhaustive = reference_loss(polynomial, covariance);
        assert(selected <= exhaustive + 2e-10 * (1 + fabs(exhaustive)));
      }
    }
  }
  /* Invalid curvature must not mutate either the bytes or working reflection. */
  for (unsigned invalid = 0; invalid < 2; ++invalid) {
    memset(covariance, 0, sizeof(covariance));
    if (invalid)
      covariance[0] = NAN;
    memcpy(work, reflection, sizeof(work));
    memcpy(codes, baseline, sizeof(codes));
    double loss = 1.0;
    int result = lpc_fit_coordinate(covariance, work, codes, 2, &loss, scratch, temporary);
    assert(result == -1);
    assert(memcmp(work, reflection, sizeof(work)) == 0);
    assert(memcmp(codes, baseline, sizeof(codes)) == 0 && loss == 1.0);
  }
}

static void test_fit_preservation(void) {
  double target[31] = {0}, expanded[31], temporary[31], covariance[31], polynomial[31];
  uint8_t baseline[30] = {128}, fitted[30];
  target[1] = -.9985;
  for (unsigned i = 2; i <= 30; ++i)
    target[i] = .1 * sin(i * 3.0) / sqrt(i);
  lpc_step_up(target, expanded, temporary);
  assert(lpc_target_covariance(target, covariance, polynomial, temporary));
  double decoded[31] = {0};
  decoded[1] = po35_lar_reflection[128];
  lpc_step_up(decoded, polynomial, temporary);
  double before = reference_loss(polynomial, covariance);
  memcpy(fitted, baseline, sizeof(fitted));
  fit_high_bank(&workspace, expanded, fitted);
  assert(fitted[0] == baseline[0] && memcmp(fitted, baseline, sizeof(fitted)) != 0);
  for (unsigned i = 1; i <= 30; ++i)
    decoded[i] = po35_lar_reflection[fitted[i - 1]];
  lpc_step_up(decoded, polynomial, temporary);
  assert(reference_loss(polynomial, covariance) < before);
  /* Valid unselected target, inconsistent boundary, unstable target, NaN,
   * and a finite overflow-producing target all retain the entire bank. */
  for (unsigned invalid = 0; invalid < 5; ++invalid) {
    target[1] = invalid == 0 ? -.4 : -.9985;
    lpc_step_up(target, expanded, temporary);
    memcpy(fitted, baseline, sizeof(fitted));
    if (invalid == 1)
      fitted[0] = 0;
    if (invalid == 2)
      expanded[30] = 1.0;
    if (invalid == 3)
      expanded[20] = NAN;
    if (invalid == 4)
      expanded[15] = DBL_MAX;
    uint8_t unchanged[30];
    memcpy(unchanged, fitted, sizeof(unchanged));
    fit_high_bank(&workspace, expanded, fitted);
    assert(memcmp(fitted, unchanged, sizeof(fitted)) == 0);
  }
}

int main(void) {
  test_lar_quantization_bounds();
  test_gain_quantization();
  test_quiet_pitch_interpolation();
  test_power_model();
  test_target_covariance();
  test_coordinate_minimization();
  test_fit_preservation();
  puts("private PO-35 analysis tests passed");
  return 0;
}
