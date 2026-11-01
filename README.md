# DS BEAT Master Suite

Two JUCE audio plugins built from one CMake project, each compiling to
**VST3** and **Standalone** from the same codebase.

## Plugins

### 1. DSBeatMaster — mastering suite
Full mastering chain (in signal order):
- **Input Gain** with auto-gain levelling (targets −18 dBFS RMS)
- **4-band Parametric EQ** (low shelf, 2× bell, high shelf) with live curve display
- **3-band Multiband Compressor** (per-band threshold, ratio, attack, release, makeup;
  4th-order Linkwitz-Riley crossovers)
- **Stereo Imager** (mid/side width per band: low/mid/high, 0–200%)
- **Harmonic Saturator** (asymmetric tube-style tanh shaper, drive + mix)
- **Brickwall Limiter** (1.5 ms lookahead, ceiling + release, GR meter)
- **Output section**: output gain, momentary LUFS meter (K-weighted),
  peak L/R meters with hold, input peak meters, limiter GR, per-band GR,
  stereo correlation meter
- Per-module bypass + master bypass; mono/stereo compatible

### 2. DSBeatGainStage — pre-mix gain staging tool
Automatically sets channel levels to pre-mix standards:
- **Instrument selector** (PRÉ-MIX standard peak targets):
  Kick −10 dB · Baixo −12 dB · Snare/Lead/Vocal −14 dB · Synth −16 dB ·
  Perc/Hi-Hat/Ride −18 dB · Pad −20 dB · FX −22 dB · Manual (custom target)
- **Target modes**: Peak / −18 LUFS / −23 LUFS (broadcast)
- Auto gain with smooth correction, per-channel trim (L/R),
  clip prevention (auto-trim above −0.26 dBFS)
- Before/after level meters with per-instrument accent colours,
  big instrument + target + applied-gain display
- Mono/stereo compatible

## UI
Dark DS BEAT style: near-black panels, silver/chrome rotary knobs and accents.

## Build

Requirements (Ubuntu): `cmake g++ make git` plus
`libx11-dev libxinerama-dev libxrandr-dev libxcursor-dev libxi-dev
libasound2-dev libfreetype6-dev libfontconfig1-dev mesa-common-dev libgl1-mesa-dev`

```sh
git clone --depth 1 https://github.com/juce-framework/JUCE.git juce
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
```

Binaries land in `build/`:
- `DSBeatMaster.vst3` / `DSBeatGainStage.vst3` (VST3 bundles)
- `DSBeatMaster` / `DSBeatGainStage` (standalone executables)

## Source layout
- `Source/PluginProcessor.*`, `Source/PluginEditor.*` — DSBeatMaster
- `Source/GainStage/` — DSBeatGainStage processor + editor
- `Source/DSP/` — MasterEQ, Crossover3, MultibandComp, StereoImager,
  Saturator, BrickwallLimiter, LufsMeter (one module per file pair)
- `Source/UI/` — DSBeatLookAndFeel, EQCurveDisplay, MeterBridge
