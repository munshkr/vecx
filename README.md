vecx
====

This is a fork of [jhawthorn/vecx](https://github.com/jhawthorn/vecx), a Vectrex
emulator. It adds real laser projector output via an audio-to-ILDA interface,
improved CLI parsing, and miscellaneous fixes.

![Vectrex Loading Screen](screenshot0.png) ![Star Trek](screenshot1.png)

Requirements
------------
* `libsdl`
* `sdl_gfx`

Usage
-----

```
vecx [OPTIONS]

Options:
  --rom FILE             ROM file to load (default: rom.dat)
  --cart FILE            Cartridge ROM file
  --overlay FILE         Overlay BMP image file
  --device DEV           Audio output device (default: system default)
  --audio-l NUM          PSG left  channel index (default: 0)
  --audio-r NUM          PSG right channel index (default: 1)
  --laser-x NUM          Laser X channel index (default: 2)
  --laser-y NUM          Laser Y channel index (default: 3)
  --laser-z NUM          Laser Z/blank channel index (default: 4)
  --laser-flip-x         Invert the laser X axis
  --laser-flip-y         Invert the laser Y axis
  --laser-optimized      Optimized scan: planner eliminates blank travel
  --laser-buf-samples N  SDL audio buffer size in frames (default: 256)
  --config FILE          Load configuration from FILE
  --help                 Show this help and exit
```

Examples:
```
./vecx --rom rom.dat
./vecx --rom rom.dat --cart mygame.bin
./vecx --rom rom.dat --cart mygame.bin --overlay overlay.bmp
./vecx --rom rom.dat --cart mygame.bin --device "MacBook Pro Speakers"
./vecx --rom rom.dat --cart mygame.bin --device "BlackHole 64ch" --laser-flip-y
./vecx --config mysetup.cfg
```

Configuration File
------------------

All options can be set in a `key=value` text file instead of (or in addition
to) the command line. CLI flags always override config file values.

**Discovery order:**
1. If `--config FILE` is given, that file is loaded (required to exist).
2. Otherwise, `vecx.cfg` in the current working directory is loaded if present.

**Format:**
- One `key=value` per line.
- Lines starting with `#` are comments; blank lines are ignored.
- Keys are the option names without the leading `--`.
- Boolean options (`laser-flip-x`, `laser-flip-y`) accept `true`/`false`,
  `1`/`0`, or `yes`/`no`.

Example `vecx.cfg`:

```
# vecx configuration
rom=rom.dat
cart=mygame.bin
overlay=overlay.bmp
device=BlackHole 64ch
audio-l=0
audio-r=1
laser-x=2
laser-y=3
laser-z=4
laser-flip-y=true
laser-optimized=true
laser-buf-samples=256
```

Audio Output
------------

Game audio (AY-3-8910 PSG) is always output through the selected device.
By default it is sent to channels 0 (left) and 1 (right). Override with
`--audio-l` and `--audio-r`.

Available audio devices are listed at startup.

Laser Output
------------

vecx drives a laser projector by outputting X, Y, and blanking signals as part
of a multi-channel audio stream to an audio-to-ILDA interface.

All signals — game audio and laser XYZ — are multiplexed onto a **single audio
device**. The device must appear to the OS as a standard multi-channel audio
output device. vecx uses the device's native sample rate automatically.

Default channel layout:

| Channel | Signal    | Description                     |
| ------- | --------- | ------------------------------- |
| 0       | Audio L   | PSG game audio left             |
| 1       | Audio R   | PSG game audio right            |
| 2       | X         | Horizontal beam position        |
| 3       | Y         | Vertical beam position          |
| 4       | Z / blank | +1.0 = beam on, −1.0 = beam off |

All signals are normalised to [−1.0, +1.0] (float32). Use the device name
exactly as reported by `--list-audio-devices`:

```
./vecx --rom rom.dat --cart mygame.bin --device "BlackHole 64ch"
```

Override any channel assignment:

```
./vecx --device "BlackHole 64ch" --audio-l 0 --audio-r 1 --laser-x 2 --laser-y 3 --laser-z 4
```

If the projected image is mirrored, use `--laser-flip-x` and/or `--laser-flip-y`:

```
./vecx --device "BlackHole 64ch" --laser-flip-y
./vecx --device "BlackHole 64ch" --laser-flip-x --laser-flip-y
```

### Optimized scan mode

By default vecx outputs every beam tick, including blank repositioning moves
between visible segments. In `--laser-optimized` mode a per-frame segment
planner is used instead: blank travel is eliminated and the beam jumps
directly to the start of each visible segment. All three channels (X, Y, Z)
are still output — visible segment samples carry Z = +1.0 (beam on) and the
inter-frame hold carries Z = −1.0 (beam off).

This reduces the number of wasted DAC samples spent on travel and can improve
scan quality when using an audio-to-ILDA interface:

```
./vecx --device "BlackHole 64ch" --laser-optimized
```

### Output latency

The SDL audio buffer size controls the trade-off between latency and
underrun risk. The default of 256 frames (≈5.8 ms at 44.1 kHz, ~12 ms
worst-case) is a good starting point. Reduce to 128 for lower latency on
hardware that supports it, or increase to 512 if you experience glitches:

```
./vecx --device "BlackHole 64ch" --laser-buf-samples 128
```

> **Note**: if the emulator stalls and the audio buffer empties, the laser
> output automatically parks at centre position with the beam off.

Releases
--------

Prebuilt binaries for Linux, macOS, and Windows are published automatically
when a version tag is pushed. On Windows the required SDL2 runtime DLLs are
bundled alongside the executable.

Download the latest release from the [Releases page](../../releases), or
publish one by pushing a version tag:

```
git tag v1.0.0
git push origin v1.0.0
```

Authors
-------

* Valavan Manohararajah - original author
* [John Hawthorn](https://twitter.com/jhawthorn) - SDL port
* [Nikita Zimin](https://twitter.com/nzeemin) - audio

Contributors
------------

* [Simon Rodriguez](https://twitter.com/simonkosua) - SDL2 port
* munshkr - laser output
