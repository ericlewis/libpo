# libpo

[![Tests](https://github.com/ericlewis/libpo/actions/workflows/tests.yml/badge.svg?branch=main&event=push)](https://github.com/ericlewis/libpo/actions/workflows/tests.yml)
[Releases](https://github.com/ericlewis/libpo/releases) · [CLI tools](core/examples/README.md) · [C API](core/docs/C_API.md) · [Format reference](core/docs/PO_FORMATS.md)

**Turn Pocket Operator backup audio into files you can inspect, edit, and rebuild.**
libpo is a C99 library and a set of offline command-line tools for Teenage
Engineering Pocket Operators. Record a transfer as a WAV, decode it, and use the
device-specific tools to work with patterns, patches, samples, or speech.

## What works on my Pocket Operator?

| Devices | What you can do | Start here |
| --- | --- | --- |
| **PO-32 Tonic** | Decode mono transfers, edit patterns, and render drum patches. | [PO-32 examples](core/examples/README.md#build) |
| **PO-33 K.O! / PO-133 Street Fighter** | Decode stereo backups, export samples, and rebuild frames with sample or pattern edits. | [Sampler workflow](core/docs/guides/PO33.md) |
| **PO-35 Speak / PO-137 Rick and Morty** | Decode stereo backups, preview speech, and build transfers with replacement speech. Drum patches share the PO-32 codec. | [Speech workflow](core/docs/guides/PO35.md) |

The paired models share record formats, not necessarily hardware behavior.
Selected PO-33 edits and a PO-35 pattern-scale edit have been verified by device
readback. Speech previews are approximate, and a successful software round trip
doesn't guarantee that hardware will accept an edit. See the
[validation limits](core/docs/PO_FORMATS.md#limits) before sending a modified backup.

## Try it without hardware

Requires CMake 3.16+ and a C99 compiler. From the repository root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
(cd build && ./core/po32_demo)
./build/core/po32_decode_capture build/demo_modem.wav build/decoded-demo
```

The demo generates and decodes a PO-32 transfer and renders a drum hit. Open
`build/demo_kick.wav` to hear the synth, or look in `build/decoded-demo/` for the
decoded packets. Use a fresh output directory if you run the decoder again.

## Decode your own backup

Record the transfer in your audio software and keep the original WAV. For
PO-33/133 and PO-35/137, **keep both channels**: mixing the recording to mono
loses transfer data. From the repository root, run the command for your model
with your WAV and a new output directory:

```sh
./build/core/po32_decode_capture tonic.wav decoded-tonic       # PO-32
./build/core/po33_decode_capture sampler.wav decoded-sampler   # PO-33 / PO-133
./build/core/po35_decode_capture speak.wav decoded-speak       # PO-35 / PO-137
```

For the stereo families, keep the exported `frame.bin` as your raw backup and
use a copy for edits. To replace samples or speech and produce a new transfer
WAV, follow the model-specific guide above. If a stereo recording won't decode,
try [`po_capture_doctor`](core/examples/README.md#capture-diagnosis) to see which
channel orders and frame parsers recovered data.

## Use libpo in your project

The freestanding C99 core uses caller-owned buffers, doesn't allocate heap
memory, and has no OS dependencies. The hosted CLI tools handle WAV files and
allocation separately. Start with the [C API guide](core/docs/C_API.md) for CMake
integration, buffer sizing, and frame completion. Use the
[architecture and protocol index](core/docs/README.md) if you're working on the
wire format or adding tooling.

### Projects built with libpo

- [Libpo32 for Schwung](https://github.com/mestela/schwung-libpo32) brings the drum synth to Ableton Move.
- [PO-PatchStudio](https://squarewave.studio/po-patchstudio) edits PO-32 sounds and patterns and sends them over audio.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for setup, checks, and commit conventions.
Thanks to [Joseph Holland](https://github.com/joseph-holland) for contributing to libpo.

## Acknowledgments

Thanks to [Magnus Lidström](https://github.com/malstrom72) of Sonic Charge,
creator of Microtonic and co-developer of the PO-32 Tonic, PO-35 Speak, and PO-137.

## License

[MIT license](LICENSE)
