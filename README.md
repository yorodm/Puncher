# Puncher

**Puncher** is a transient shaping audio plugin built with the [iPlug2](https://github.com/iPlug2/iPlug2) framework. It provides precise control over the attack and sustain characteristics of audio signals, making it ideal for enhancing drums, percussion, and other transient-rich material.

This is a port of Reapers's JSFX Transient Controller.

## What It Does

Puncher uses a multi-stage envelope follower algorithm to analyze and shape the transient response of your audio:

- **Attack Control**: Emphasize or soften the initial hit of transients (-100% to +100%)
- **Sustain Control**: Adjust the body and tail of the sound (-100% to +100%)  
- **Output Gain**: Fine-tune the output level (-12 dB to +6 dB)

## Technical Details

- **Formats**: VST3, AU v2/v3, AAX (with AudioSuite), CLAP, Standalone App
- **Platforms**: macOS (Intel/Apple Silicon), Windows, iOS, Linux (CLAP)
- **Channels**: Stereo (2-in/2-out)
- **Latency**: 0 samples
- **UI**: Resizable interface (256x256 to 8192x8192)

## Building

Requires CMake 3.25+ and the iPlug2 framework cloned to `../iPlug2`:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Or use the provided IDE files (`Puncher.sln` for Visual Studio, `Puncher.xcworkspace` for Xcode).

## License

Copyright © 2025 OneKnobAudio. All rights reserved.
