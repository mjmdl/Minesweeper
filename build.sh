CC="g++"
CFLAGS="-Wall -Wextra -std=c++17 -ggdb"
LIBS="-lSDL2 -lSDL2_image -lm"

$CC -o Minesweeper src/minesweeper.cpp $CFLAGS $LIBS
