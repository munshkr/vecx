vecx
====

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
  --laser-device DEV   Laser device path
  --laser-map MAP      Laser channel map string
  --help               Show this help and exit
```

Examples:
```
./vecx --rom rom.dat
./vecx --rom rom.dat --cart mygame.bin
./vecx --rom rom.dat --cart mygame.bin --overlay overlay.bmp
./vecx --rom rom.dat --laser-device /dev/ttyUSB0 --laser-map "0,1,2,3"
```

Authors
-------

* Valavan Manohararajah - original author
* [John Hawthorn](https://twitter.com/jhawthorn) - SDL port
* [Nikita Zimin](https://twitter.com/nzeemin) - audio

Contributors
-------
* [Simon Rodriguez](https://twitter.com/simonkosua) - SDL2 port


