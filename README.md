vecx
====

This is a fork of [jhawthorn/vecx](https://github.com/jhawthorn/vecx), a Vectrex
emulator. It mainly adds real laser projector output for an audio-to-ILDA
interface, improved CLI parsing and miscellaneous fixes.

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
  --rom FILE           ROM file to load (default: rom.dat)
  --cart FILE          Cartridge ROM file
  --overlay FILE       Overlay BMP image file
  --laser-device DEV   Laser DAC audio device name
  --laser-x NUM        Output channel index for X (default: 0)
  --laser-y NUM        Output channel index for Y (default: 1)
  --laser-z NUM        Output channel index for Z/blank (default: 2)
  --help               Show this help and exit
```

Examples:
```
./vecx --rom rom.dat
./vecx --rom rom.dat --cart mygame.bin
./vecx --rom rom.dat --cart mygame.bin --overlay overlay.bmp
./vecx --rom rom.dat --cart mygame.bin --laser-device "DAC-ILDA"
```

Laser Output
------------

vecx drives a laser projector by outputting X, Y, and blanking signals as a
3-channel audio stream to an audio-to-ILDA interface.

The interface must appear to the OS as a standard multi-channel audio output
device. vecx uses the device's native sample rate automatically (44.1 kHz or
96 kHz).

Channel assignment (default: X=0, Y=1, Z=2):

| Channel | Signal  | Description                     |
| ------- | ------- | ------------------------------- |
| 0       | X       | Horizontal beam position        |
| 1       | Y       | Vertical beam position          |
| 2       | Z/blank | +1.0 = beam on, −1.0 = beam off |

All signals are normalised to [−1.0, +1.0] (float32). Use the device name
exactly as it appears in your OS audio settings:

```
./vecx --rom rom.dat --cart mygame.bin --laser-device "DAC-ILDA"
./vecx --rom rom.dat --laser-device "DAC-ILDA" --laser-x 0 --laser-y 1 --laser-z 2
```

If your interface uses a different channel order, override with `--laser-x`,
`--laser-y`, and `--laser-z`. For example, if Z/blank is on channel 3:

```
./vecx --rom rom.dat --laser-device "DAC-ILDA" --laser-z 3
```

> **Note**: if the emulator stalls and the audio buffer empties, the output
> automatically parks at centre position with the beam off.

Authors
-------

* Valavan Manohararajah - original author
* [John Hawthorn](https://twitter.com/jhawthorn) - SDL port
* [Nikita Zimin](https://twitter.com/nzeemin) - audio

Contributors
------------

* [Simon Rodriguez](https://twitter.com/simonkosua) - SDL2 port
* munshkr - laser output
