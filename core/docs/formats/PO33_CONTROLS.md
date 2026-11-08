# PO-33 control playback

[Sampler layout](PO33.md#pattern-parameters-and-controls) ·
[Control hypotheses](FIELD_HYPOTHESES.md#5-guess-control-roles-before-guessing-exact-scales) ·
[C API](../api/PO33.md#resonance)

A controlled PO-33 test measured a first-note pitch offset, a null bit-5
contrast, and coarse volume/filter responses. The generated bank returned
byte-for-byte before and after playback. These results constrain playback;
they do not complete the lower-field layout or establish universal numeric
conversions. This test did not independently validate a PO-133.

The names q0..q3 below are the descriptive six-bit coordinates defined in the
[format reference](PO33.md#pattern-parameters-and-controls), not four proven
field widths. This synthetic bank does not independently assign the lower
fields to physical knob writes.

## Test context and evidence

Sixteen patterns ran in an automatic nineteen-entry chain at 60 BPM, with
swing zero. Each pattern triggered Sound 1 at zero-based steps 0, 4, 8 and 12
using control `FC`. Every parameter row in that sound lane held the explicit
test word; other lanes were zero. All FX codes were `F`, and q3 was zero.
The shared source was an attenuated 250 ms multisine. The same baseline pattern
was reused after the low and high pitch primers and among level/filter cases.

The complete test frame contained 68,450 bytes and 423 packets. Every byte
matched the generated frame in both device readbacks, including the synthetic
bit-5 values. The original 110,278-byte, 593-packet bank was subsequently
restored and read back exactly.

The recording had no timestamp gaps, dropped frames or digital clipping.
An unexplained common level increase of about 32.08 dB occurred during
playback. Its cause remains unassigned. Musical pitches, durations and spectral
lines continued across it; the louder section was not a modem transmission.
The transition was excluded, and each level segment was independently aligned
to the observed cyclic pattern order and measured against its own references.
No gain or primary pitch pair crosses the level change.

Two complete quiet cycles and one complete louder cycle support the amplitude
comparisons. Four within-segment low/high pairs support the primary pitch
comparison; one pair comes from a partial final cycle. A transient reference
note was excluded from amplitude calibration. Channels and overlapping windows
are correlated measurements, not additional physical repetitions. Reported
ranges describe observed variation, not statistical confidence intervals.

## First-note pitch offset and settling

The same q0=13 baseline pattern followed q0=7 and q0=19 primer patterns. The
first target after the lower primer was slightly lower in pitch than the first
target after the higher primer:

| Comparison | Cents |
| --- | ---: |
| Frozen prediction, ratio `1020/1024` | -6.77588 |
| Median of four pairs, left channel | -6.77116 |
| Median of four pairs, right channel | -6.77587 |
| Individual pair/channel range | -6.78176 to -6.76339 |

The second, third and fourth q0=13 notes returned to the reference pitch:
repeat medians were within 0.0014 cents, and all measured channel/window
values within 0.059 cents. The offset is therefore associated with the first
target note in this transition, not indefinitely different q0=13 tuning.

This supports the [quantized-rate/context hypothesis](FIELD_HYPOTHESES.md#po-33-pitch-rate-model).
It does not identify a firmware algorithm or fixed-point representation.
The target pattern is held fixed, but the predecessor pattern identity still
co-varies. The source, q3 value and explicit inactive-row parameters differ
from the older recordings used to develop the prediction.

## Bit 5 survives without a resolved playback effect

At q1=3 and q1=7, paired patterns differed only in parameter-word bit 5.
The lower five pitch-coordinate bits stayed at 13; setting bit 5 made the
descriptive q0 extraction 45. Both values survived the device readbacks.

Neither pair produced a resolved systematic pitch or gain change. Per-cycle
median RMS contrasts stayed within 0.013 dB and fundamental-line contrasts
within 0.011 dB. Individual quiet pitch estimates were noisier, with observed
extrema around 0.27 cents; louder bit-5 comparisons stayed within 0.036 cents.
The neighboring q1 change from 3 to 4 remained measurable at about +0.194 dB
RMS, with per-cycle medians from +0.193 to +0.199 dB.

This rejects a simple extra +32-semitone interpretation in these cases and
does not support a simple audible half-volume step. It does not prove that
bit 5 is universally unused, distinguish every ignored/clamped interpretation,
or identify which knob writes it. Preserve the bit and the remaining raw word.

## Coarse volume response

At q0=13, control `FC`, q2=63 and q3=0, the source fundamental gave these
relative gains. Each level segment uses its own q1=32 reference:

| q1 | Quiet segment | Louder segment |
| ---: | ---: | ---: |
| 3 | -13.754 dB | -13.752 dB |
| 4 | -13.554 dB | -13.556 dB |
| 7 | -12.518 dB | -12.516 dB |
| 16 | -7.991 dB | -7.991 dB |
| 32 | 0 dB | 0 dB |
| 41 | +3.601 dB | +3.600 dB |
| 63 | +10.377 dB | +10.377 dB |

Signal-line amplitudes are used because quiet broadband RMS includes the
recording floor. Agreement between the two segments supports this relative
curve. These are sampled points, not an exact universal function. Earlier
two-point comparisons used different source/control/parameter-row contexts;
their results must not be silently pooled with this table.

With q1=0 following q1=7, output matched the preceding setting rather than
silence: per-cycle median RMS differences stayed within 0.0064 dB. Other
controls were unchanged, so the test does not distinguish per-control retention
from skipping the whole parameter row when that field is zero. Other
predecessors and zero-update contexts remain untested.

## Coarse filter response

At the same pitch, level and raw resonance, these measured signal-line gains
are relative to q2=63. Column frequencies are approximate:

| q2 | 110 Hz | 220 Hz | 440 Hz | 880 Hz | 1760 Hz | 3520 Hz |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 16 | +0.08 dB | +0.32 dB | +1.30 dB | +4.03 dB | -7.45 dB | -21.49 dB |
| 32 | +1.93 dB | +0.48 dB | +0.12 dB | +0.03 dB | +0.01 dB | +0.01 dB |
| 47 | -35.32 dB | -23.00 dB | -9.51 dB | +3.80 dB | +1.57 dB | +0.37 dB |

q2=16 has a low-pass-like shape and q2=47 a high-pass-like shape. q2=32 boosts
the lowest measured line relative to the chosen reference. Six frequencies
do not determine a unique filter family, cutoff-Hz formula or Q value, or
establish q2=63 as a universal bypass. The weakest lines are noise-limited.
Fixed reference-frequency projections of pitch-shifted patterns are excluded
from this filter comparison.

q2=0 following q2=16 reproduced the preceding spectral response, with per-cycle
median RMS differences within 0.0023 dB. As with volume zero, independent
retention versus whole-row skipping was not isolated. Do not turn these
observations into a blanket zero-as-inherit rule.

## Consequence for editors

Retain parameter words, including bit 5 and zeros, during unrelated edits.
Do not normalize them, infer a complete field layout, or implement a numeric
gain/cutoff conversion from these measurements alone. The existing C record
codecs and raw resonance helpers keep their contracts and layouts unchanged.
