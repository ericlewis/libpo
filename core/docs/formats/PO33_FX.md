# PO-33 FX mapping

[Pattern storage](PO33.md#record-layout) · [Hardware evidence](PO33_EFFECTS_STATE.md)

The [manufacturer's guide](https://teenage.engineering/guides/po-33/en) names
sixteen FX buttons. Each pattern stores sixteen four-bit codes, high nibble
first, in bytes 1024..1031. Storage order is verified at all sixteen positions.

The full identity table below proposes **stored code = button number minus one**.
Only button 7 writing code `6` is directly confirmed. Generated-code playback
now characterizes several other responses, but does not prove which button
writes them. `F` is the observed normal baseline. The remaining button
identities are ordering hypotheses, not a production enum.

| Button | UI choice | Candidate code | Evidence |
| ---: | --- | ---: | --- |
| 1 | Loop16 | `0` | Repeated source-start chirps observed; button write unconfirmed |
| 2 | Loop12 | `1` | Different repetition interval observed; button write unconfirmed |
| 3 | Short loop | `2` | Faster repetition observed; button write unconfirmed |
| 4 | Shorter loop | `3` | Faster repetition observed; button write unconfirmed |
| 5 | Unison | `4` | Multiple detuned components observed; button write unconfirmed |
| 6 | Low unison | `5` | Mixed lower and near-original components; button write unconfirmed |
| 7 | Octave up | **`6`** | **Button write and playback confirmed** |
| 8 | Octave down | `7` | Half-pitch playback; button write unconfirmed |
| 9 | Stutter4 | `8` | Amplitude gating observed; button write unconfirmed |
| 10 | Stutter3 | `9` | Different gating rate observed; button write unconfirmed |
| 11 | Scratch | `A` | Repeated source-start chirps observed; button write unconfirmed |
| 12 | Fast scratch | `B` | Alternating upward/downward chirps observed; button write unconfirmed |
| 13 | 6/8 quantization | `C` | No distinct response in the tested rhythm; identity unresolved |
| 14 | Pattern retrigger | `D` | Repeated source-start chirps while chain advances; identity unresolved |
| 15 | Reverse | `E` | Reversed source playback observed; button write unconfirmed |
| 16 | No effect | `F` | Normal baseline; button-16 clear-write untested |

Names such as Loop16 do not establish buffer lengths or timing units. These
results are specific to the tested PO-33; PO-133 behavior is not independently
established by this map. The raw C FX codecs preserve every nibble without
assigning the inferred names.

## Stronger evidence for code 7

A generated bank used identical sample, note and parameter data with code `6`
or `7` in matched early/late windows and untouched `F` references. Complete
pre-play and post-play readbacks matched all 68,450 bytes and 423 packets.
The selected playback had no timing gaps, clipping or level discontinuity.

Code `7` produced interior pitch ratios of approximately 0.499024–0.500000
against the same-step reference, with roughly 396 ms audible duration versus
198 ms. Nine notes crossing the window boundary started near half pitch and
returned toward normal pitch within the note. This supports a rate-changing,
octave-down-like response; it is not an exact universal tuning or timing law.

A subsequent manual FX7/FX8 procedure supplies no additional writer mapping:
the attributed readback contains only `FF` footers. Its non-FX edits do not
show which effect button was stored. Recording labels and intended gestures
must remain separate from the bytes actually recovered.

## Automatic raw-code comparison

A later PO-33 test compared fourteen raw codes with untouched `F` references
at 120 BPM. A time-asymmetric chirp followed by a harmonic tone distinguishes
source restarts and reversal from changes in volume. Reference-pattern timing
and identity determine chain position; no candidate effect identity is assumed.
Every target shares its note, sample and control data with its
matched reference and follows the same primer pattern.

Both loaded and post-playback readbacks match the complete prepared bank:
68,450 bytes and 423 validated packets. Two complete passes have no capture
timing gaps or clipping. All target bars span approximately two seconds at
20 ms analysis resolution; none persistently stalls the chain.

| Raw code | Observed response in both passes | What remains unresolved |
| --- | --- | --- |
| `0`, `1` | Repeated upward chirps about every 125 ms and 167 ms, respectively | Buffer source, exact loop schedule and boundary handling |
| `2`, `3` | Faster upward chirp repetitions, about every 62.5 ms and 41.7 ms, respectively | Buffer source, exact loop schedule and boundary handling |
| `4` | Multiple resolved frequency components around the reference fundamental, with corresponding harmonics | Implementation voice count, detuning and mixing law |
| `5` | A lower-frequency family together with components near the reference pitch | Voice structure and mixing law; this is not a simple half-speed response |
| `7` | Approximately half the reference pitch | Universal tuning law and physical button identity |
| `8` | Amplitude gating about every 125 ms; no extra source-start chirps | Duty cycle and behavior at other tempos |
| `9` | Amplitude gating about every 167 ms; no extra source-start chirps | Duty cycle and behavior at other tempos |
| `A` | Source-start chirps recur about every 250 ms | Full source-position trajectory and boundary behavior |
| `B` | Alternating upward/downward chirps, with about 125 ms per full cycle | Exact scan trajectory and boundary behavior |
| `C` | Closely matches the untreated reference | Quantization cannot be distinguished with these onsets |
| `D` | Source-start chirps recur about every 125 ms while the chain advances | Whether the operation resets the pattern, voice or another playback state |
| `E` | The onset chirp moves to the end and descends; envelope closely matches the reversed reference | Behavior across effect boundaries and physical button identity |

These timings describe one tempo and source, not general DSP constants. The
chirp recurrence is not necessarily the waveform's full period: code `3` has
chirps about 41.7 ms apart but a stronger waveform repetition around 83.3 ms.
Neither measurement alone establishes the internal loop-buffer length. The
`8`/`9` comparison distinguishes amplitude gating from the repeated source-start
signatures in `A`/`D`; it does not establish which internal state resets or the
entire effect implementation. Alternating chirp direction in `B` does not by
itself prove a particular sample-pointer trajectory. Similarly,
`C` matching the reference is a null result for this rhythm, not proof of a
no-op. Codes `6` and `F` retain their earlier evidence above.

## What would close the map

Use a longer, uniquely marked phrase to distinguish the `0`–`3` loop histories.
Uneven note positions are needed for `C`; different sounds across the pattern
would distinguish a pattern restart from a single-voice restart for `D`.
Repeat timing comparisons at another tempo before deriving tempo-scaling laws.
Short effect windows also need separate checks for entry, release and retained
state. Preserve matched primer history and identify chain position from
unaffected references, retaining missing or stalled cases as unresolved.

Generated-code playback can characterize these responses with few manual
actions. A known button write plus validated pattern readback is the separate
test for button-to-code identity. Neither unchanged readback nor an audible
impression alone establishes both.
