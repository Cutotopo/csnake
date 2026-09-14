# csnake
Simple Snake game written in C using GTK and gstreamer.

The game can be controlled either using WASD or the arrow keys, and paused/resumed with either Escape or P.

## Flags
csnake supports the following flags:
| Flag             | Short flag | Description                                             | Expected input        |
|------------------|------------|---------------------------------------------------------|-----------------------|
| --apples         | -a         | Number of apples to place on the field                  | Positive integer      |
| --help           | -h         | Show help message                                       |                       |
| --refreshTimeout | -rt        | Set game tick interval in milliseconds (default is 200) | Positive integer      |
| --size           | -s         | Set game field size                                     | Two positive integers |
## Development
If GTK & gstreamer libraries are installed on your system, you may be able to build this software using:
```
gcc $(pkg-config --cflags gtk4 gstreamer-1.0) csnake.c -o csnake -lm $(pkg-config --libs gtk4 gstreamer-1.0)
```

Debugging information can be toggled via the `--debug` flag. Valid options are:
 - `print_field`: print field values on each cell
 - `print_snake`: print snake values on each cell

## Credits
Background music has been contributed by [@touchinglie](https://github.com/touchinglie)!