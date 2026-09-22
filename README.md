# csnake
Simple Snake game written in C using GTK and gstreamer.

The game can be controlled either using WASD or the arrow keys, and paused/resumed with either Escape or P.

## Flags
csnake supports the following flags:
| Flag             | Short flag | Description                                                              | Expected input                    |
|------------------|------------|--------------------------------------------------------------------------|-----------------------------------|
| --apples         | -a         | Number of apples to place on the field (default is `5`)                  | Positive integer                  |
| --debug          | -d         | Toggle debug features (can be repeated to toggle multiple features)      | Positive integer                  |
| --help           | -h         | Show help message and exit                                               | [See below](#debugging-flags)     |
| --refreshTimeout | -rt        | Set game tick interval in milliseconds (default is `200`)                | Positive integer                  |
| --size           | -s         | Set game field size (default is `25 25`)                                 | Two positive integers             |
| --squareSize     | -sz        | Set game square size (useful to make the window bigger, default is `25`) | Positive integer, greater than 25 |

## Development
If GTK & gstreamer libraries are installed on your system, you may be able to build this software using:
```
gcc $(pkg-config --cflags gtk4 gstreamer-1.0) csnake.c -o csnake -lm $(pkg-config --libs gtk4 gstreamer-1.0)
```

### Debugging flags
Debugging information can be toggled via the `--debug` flag. Valid options by category are:
 - debugging information on the grid (each flag in this category is mutually exclusive)
   - `print_field`: print field values on each cell
   - `print_snake`: print snake values on each cell
 - game behavior
   - `noclip`: the game does not end if the snake crosses itself
 - logging
   - `log_movement`: logs snake movement to stdout
   - `log_rng`: logs RNG-related events (e.g. apples placed) to stdout

## Credits
Background music has been contributed by [@touchinglie](https://github.com/touchinglie)!