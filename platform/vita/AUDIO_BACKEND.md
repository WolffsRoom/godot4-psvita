# Vita audio backend

## Implemented routing

Godot 4 Vita uses Godot AudioServer for stream mixing. The platform adapter
emits native interleaved stereo 16-bit samples at 48 kHz.

The preceding implementation sent each 2048-frame block to the PS Vita
MAIN output port at maximum hardware volume. The new implementation
adapts the BGM-port technique seen in the local DeltaruneVita project:

- Open SCE_AUDIO_OUT_PORT_TYPE_BGM at 48000 Hz; fall back to MAIN if needed.
- Set both hardware channel volumes to 72% of SCE_AUDIO_OUT_MAX_VOL, separately
  from Godot's master, music and sound-effect bus gains.
- Request SCE_AUDIO_ALC_OFF for the BGM route to prevent dynamic volume
  normalization from pumping music.
- Use Godot's standard Q31 to interleaved PCM16 conversion.
- Use a high-priority audio thread and 2048-sample blocks; report only
  the first hardware output failure rather than logging on every block.
- Register sceAudioOutSetVolume and sceAudioOutSetAlcMode in the
  native loader's default_dynlib mapping. Without these entries the runtime
  cannot resolve its new imported symbols.

The DeltaruneVita reference uses OpenAL for game sound, buffered Vorbis music
and a BGM channel for direct PCM playback. This port deliberately does not
copy its OpenAL runtime or interfere with the original DeltaruneVita tree:
Godot still uses its own AudioServer and Bus architecture.

## Testing

- Native VitaSDK engine build, loader and VPK packaging verification.
- Static import resolution check for the two additional SceAudio functions.
- Host-testable PCM16 helper: compile and execute
  tools/tests/test_audio_pcm_vita.cpp with g++.
- On physical Vita test main BGM playback, effect overlap, end/start of
  music loops and transitions, volume changes and headset vs speakers.
  Sound quality cannot be verified by host import tests alone.

## Future diagnosis if clipping persists

1. Collect on-device ux0:data/godot_loader.log audio route line
   (BGM vs MAIN, volume_rc, any output failure).
2. Compare behavior on built-in speakers vs headphones and at 50% game
   volume versus 100%.
3. If audio still breaks, capture an output block or investigate
   SCE audio scheduling and Godot bus processing for clipping/underruns.
