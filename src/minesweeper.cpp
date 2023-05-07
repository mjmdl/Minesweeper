#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include "minesweeper.hpp"

void FieldCell::RaiseDanger()
{
    if (danger < 8 && !trapped)
	danger++;
}

void MineField::Alloc()
{
    if (rows < 2 || columns < 2 || mineCount < 2
	|| rows * columns <= mineCount) {
	fprintf(stderr, "ERROR: Invalid MineField attributes.\n");
	return;
    }
    
    grid = (FieldCell **) malloc(rows * sizeof(FieldCell *));
    for (int r = 0; r < rows; r++) {
	grid[r] = (FieldCell *) malloc(columns * sizeof(FieldCell));
	for (int c = 0; c < columns; c++) {
	    grid[r][c] = {
		.locked = false,
		.flagged = false,
		.trapped = false,
		.danger = 0};
	}
    }
}

void MineField::DeAlloc()
{
    if (grid == NULL)
	return;
    
    for (int r = 0; r < rows; r++)
	free(grid[r]);
    free(grid);
}

bool MineField::InBounds(int r, int c) const
{
    return r >= 0 && r < rows && c >= 0 && c < columns;
}

void MineField::PopulateMines()

