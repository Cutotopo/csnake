# csnake
Simple Snake game written in C using GTK

## Development
If GTK libraries are installed on your system, you may be able to build this software using:
```
gcc $(pkg-config --cflags gtk4) csnake.c -o csnake -lm $(pkg-config --libs gtk4)
```
