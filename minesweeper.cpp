#include <cstdio>
#include <ctime>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define FRAME_DELAY (1000/30)
#define CELL_SIZE 50
#define TILE_SIZE 16
#define TEXTURE_TILESET_PATH "res/tileset.png"

enum State {
    RunningState,
    QuitState,
    SuccessState,
    FailureState,
};

struct Cell {
    bool locked;
    bool flagged;
    bool trapped;
    int danger;

    void RaiseDanger() {
	if (danger < 8 && !trapped)
	    danger++;
    }
};

struct Field {
    int rows = 16;
    int columns = 12;
    int mineCount = 20;
    Cell **grid = nullptr;

    void Alloc() {
	if (rows < 2 || columns < 2 || mineCount < 2
	    || rows * columns <= mineCount) {
	    fprintf(stderr, "ERROR: Invalid MineField attributes.\n");
	    return;
	}
    
	grid = (Cell **) malloc(rows * sizeof(Cell *));
	for (int r = 0; r < rows; r++)
	    grid[r] = (Cell *) malloc(columns * sizeof(Cell));
    }

    void DeAlloc() {
	if (grid == NULL)
	    return;
    
	for (int r = 0; r < rows; r++)
	    free(grid[r]);
	free(grid);
    }

    bool InBounds(int r, int c) const {
	return r >= 0 && r < rows && c >= 0 && c < columns;
    }

    void PopulateMines() {
	srand(time(NULL));

	for (int r = 0; r < rows; r++) {
	    for (int c = 0; c < columns; c++) {
		grid[r][c] = {
		    .locked = true,
		    .flagged = false,
		    .trapped = false,
		    .danger = 0};
	    }
	}
    
	for (int m = 0; m < mineCount; m++) {
	    int r = rand() % rows;
	    int c = rand() % columns;

	    if (grid[r][c].trapped) {
		m--;
	    } else {
		grid[r][c].trapped = true;

		bool n = InBounds(r-1, c);
		bool s = InBounds(r+1, c);
		bool w = InBounds(r, c-1);
		bool e = InBounds(r, c+1);
	    
		if (w) grid[r][c-1].RaiseDanger();
		if (e) grid[r][c+1].RaiseDanger();
		if (n) {
		    grid[r-1][c].RaiseDanger();
		    if (w) grid[r-1][c-1].RaiseDanger();
		    if (e) grid[r-1][c+1].RaiseDanger();
		}
		if (s) {
		    grid[r+1][c].RaiseDanger();
		    if (w) grid[r+1][c-1].RaiseDanger();
		    if (e) grid[r+1][c+1].RaiseDanger();
		}
	    }
	}
    }

    void RevealMines() {
	for (int r = 0; r < rows; r++) {
	    for (int c = 0; c < columns; c++) {
		if (grid[r][c].trapped)
		    grid[r][c].locked = false;
	    }
	}
    }

    ~Field() { DeAlloc(); }
};

constexpr SDL_Rect TileRectFrom(int x, int y) {
    return SDL_Rect { x * TILE_SIZE, y * TILE_SIZE, TILE_SIZE, TILE_SIZE };
}
 
struct Textures {
    SDL_Texture *tileset;
    const SDL_Rect blank = TileRectFrom(0, 0);
    const SDL_Rect flag = TileRectFrom(1, 0);
    const SDL_Rect mine = TileRectFrom(2, 0);
    const SDL_Rect numbers[9] = {
	[0] = TileRectFrom(3, 0),
	[1] = TileRectFrom(0, 1),
	[2] = TileRectFrom(1, 1),
	[3] = TileRectFrom(2, 1),
	[4] = TileRectFrom(3, 1),
	[5] = TileRectFrom(0, 2),
	[6] = TileRectFrom(1, 2),
	[7] = TileRectFrom(2, 2),
	[8] = TileRectFrom(3, 2)};

    bool Load(SDL_Renderer *renderer, const char *filepath) {
	if ((tileset = IMG_LoadTexture(renderer, filepath)) == NULL) {
	    return false;
	}
	return true;
    }
};

const SDL_Color BackgroundColor = { .r = 40, .g = 45, .b = 42, .a = 255 };

void RenderGame(SDL_Renderer *renderer, const Textures &textures,
		const Field &field, const State &state) {
    (void) state;
    
    SDL_Rect dest = { 0, 0, CELL_SIZE, CELL_SIZE };
    const SDL_Rect *src = &textures.blank;
    
    for (int r = 0; r < field.rows; r++) {
	for (int c = 0; c < field.columns; c++) {
	    Cell &cell = field.grid[r][c];

	    dest.x = c * CELL_SIZE;
	    dest.y = r * CELL_SIZE;
	    
	    if (cell.locked)
		src = &textures.blank;
	    else if (cell.trapped)
		src = &textures.mine;
	    else
		src = &textures.numbers[cell.danger];
	    
	    SDL_RenderCopy(renderer,
			   textures.tileset,
			   src, &dest);
	    
	    if (cell.flagged)
		SDL_RenderCopy(renderer,
			       textures.tileset,
			       &textures.flag, &dest);
	}
    }
}

void ClickOnCell(Field &field, State &state, int button,
		 int r, int c) {
    static int unlocks = 0;
    
    Cell &cell = field.grid[r][c];

    switch (button) {
    case SDL_BUTTON_LEFT:
	unlocks++;
	if (cell.locked && !cell.flagged) {
	    cell.locked = false;

	    if (cell.trapped) {
		if (unlocks < 2) {
		    unlocks = 0;
		    field.PopulateMines();
		    ClickOnCell(field, state, button, r, c);
		} else {
		    state = FailureState;
		    field.RevealMines();
		}
	    }
	}
	break;
	
    case SDL_BUTTON_RIGHT:
	if (cell.locked)
	    cell.flagged = !cell.flagged;
	break;
    }
}

void HandleEvents(Field &field, State &state) {
    static SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
	switch (event.type) {
	case SDL_QUIT:
	    state = QuitState;
	    break;

	case SDL_MOUSEBUTTONUP:
	    if (state == RunningState)
		ClickOnCell(field, state, event.button.button,
			    event.button.y / CELL_SIZE,
			    event.button.x / CELL_SIZE);
	    break;
	}
    }
}

void BeginGameLoop(SDL_Renderer *renderer, Field &field) {
    State state = RunningState;
    field.Alloc();
    field.PopulateMines();

    Textures textures;
    if (!textures.Load(renderer, TEXTURE_TILESET_PATH)) {
	fprintf(stderr, "ERROR: failed to load texture %s\n",
		TEXTURE_TILESET_PATH);
	state = QuitState;
    }

    while (state != QuitState) {
	SDL_SetRenderDrawColor(
	    renderer,
	    BackgroundColor.r, BackgroundColor.g, BackgroundColor.b,
	    BackgroundColor.a);
	SDL_RenderClear(renderer);
	RenderGame(renderer, textures, field, state);
	SDL_RenderPresent(renderer);

	HandleEvents(field, state);

	SDL_Delay(FRAME_DELAY);
    }
}

void QuitSdl(SDL_Window *&window, SDL_Renderer *&renderer) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
}

bool InitSdl(int windowWidth, int windowHeight,
	     SDL_Window *&window, SDL_Renderer *&renderer) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
	fprintf(stderr, "ERROR: SDL_Init failed: %s\n",
		SDL_GetError());
	goto failure;
    }

    if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
	fprintf(stderr, "ERROR: IMG_Init failed: %s\n",
		IMG_GetError());
	goto failure;
    }

    window = SDL_CreateWindow(
	"Minesweeper",
	SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	windowWidth, windowHeight, 0);
    if (window == NULL) {
	fprintf(stderr, "ERROR: SDL_CreateWindow failed: %s\n",
		SDL_GetError());
	goto failure;
    }

    renderer = SDL_CreateRenderer(
	window, -1,
	SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
	fprintf(stderr, "ERROR: SDL_CreateRenderer() failed: %s\n",
		SDL_GetError());
	goto failure;
    }

    return true;

failure:
    QuitSdl(window, renderer);
    return false;
}

int main() {
    SDL_Window *window;
    SDL_Renderer *renderer;

    Field field = {
	.rows = 12,
	.columns = 10,
	.mineCount = 20};
    
    InitSdl(field.columns * CELL_SIZE, field.rows * CELL_SIZE,
	    window, renderer);
    BeginGameLoop(renderer, field);
    QuitSdl(window, renderer);
    
    return 0;
}
