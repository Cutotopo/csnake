# csnake
Simple Snake game written in C using GTK

## Flags
csnake supports the following flags:
| Flag             | Short flag | Description                                             | Expected input   |
|------------------|------------|---------------------------------------------------------|------------------|
| --refreshTimeout | -rt        | Set game tick interval in milliseconds (default is 200) | Positive integer |
| --help           | -h         | Show help message                                       |                  |

## Development
If GTK libraries are installed on your system, you may be able to build this software using:
```
gcc $(pkg-config --cflags gtk4) csnake.c -o csnake -lm $(pkg-config --libs gtk4)
```
