#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

//CONSTANTS

#define MIN_SIZE 5
#define MAX_SIZE 15

#define INITIAL_LIVES 3
#define REQUIRED_INTEL 3
#define INTEL_COUNT 3
#define LIFE_COUNT 2

/* Grid symbols */
#define EMPTY '.'
#define WALL '#'
#define INTEL 'I'
#define LIFE 'L'
#define EXTRACTION 'X'
#define P1 '@'

//STRUCTURES

typedef struct {
    int row, col;
    int lives;
    int intel;
    int active;
    char symbol;
} Player;

typedef struct {
    char **grid;
    int size;
    Player player;
    int extraction_row;
    int extraction_col;
    FILE *log;
} Game;

//FUNCTION DECLARATIONS

Game* init_game(int size);
void free_game(Game *game);
void place_items(Game *game);
void display_grid(Game *game);
int valid_move(Game *game, int r, int c);
void move_player(Game *game, char move);
void log_state(Game *game, const char *action);
void play_game(Game *game);

//GAME INITIALIZATION

Game* init_game(int size) {
    Game *game = malloc(sizeof(Game));
    game->size = size;

    /* Allocate grid */
    game->grid = malloc(size * sizeof(char*));
    for (int i = 0; i < size; i++) {
        game->grid[i] = malloc(size * sizeof(char));
        for (int j = 0; j < size; j++)
            game->grid[i][j] = EMPTY;
    }

    //Initialize player
    game->player.lives = INITIAL_LIVES;
    game->player.intel = 0;
    game->player.active = 1;
    game->player.symbol = P1;

    do {
        game->player.row = rand() % size;
        game->player.col = rand() % size;
    } while (game->grid[game->player.row][game->player.col] != EMPTY);

    game->grid[game->player.row][game->player.col] = game->player.symbol;

    place_items(game);

    game->log = fopen("game_log.txt", "w");
    if (game->log) {
        fprintf(game->log, "=== SpyNet Game Log - Part 1 ===\n\n");
    }

    return game;
}

//CLEANUP

void free_game(Game *game) {
    if (!game) return;
    
    for (int i = 0; i < game->size; i++)
        free(game->grid[i]);

    free(game->grid);

    if (game->log)
        fclose(game->log);
    free(game);
}

//GRID & ITEMS

void place_items(Game *game) {
    int r, c;

    /* Intel */
    for (int i = 0; i < INTEL_COUNT; i++) {
        do {
            r = rand() % game->size;
            c = rand() % game->size;
        } while (game->grid[r][c] != EMPTY);
        game->grid[r][c] = INTEL;
    }

    /* Lives */
    for (int i = 0; i < LIFE_COUNT; i++) {
        do {
            r = rand() % game->size;
            c = rand() % game->size;
        } while (game->grid[r][c] != EMPTY);
        game->grid[r][c] = LIFE;
    }

    /* Walls */
    int walls = (game->size * game->size) / 8;
    for (int i = 0; i < walls; i++) {
        do {
            r = rand() % game->size;
            c = rand() % game->size;
        } while (game->grid[r][c] != EMPTY);
        game->grid[r][c] = WALL;
    }

    /* Extraction - Store location permanently */
    do {
        r = rand() % game->size;
        c = rand() % game->size;
    } while (game->grid[r][c] != EMPTY);
    game->grid[r][c] = EXTRACTION;
    game->extraction_row = r;
    game->extraction_col = c;
}

void display_grid(Game *game) {
    /* Print top border */
    printf("\n    ");
    for (int i = 0; i < game->size; i++) {
        printf("+---");
    }
    printf("+\n");

    /* Print column numbers starting from 1 */
    printf("    ");
    for (int i = 1; i <= game->size; i++)
        printf("  %2d", i);
    printf("\n");

    /* Print second border */
    printf("    ");
    for (int i = 0; i < game->size; i++) {
        printf("+---");
    }
    printf("+\n");

    /* Print grid rows with row numbers starting from 1 */
    for (int i = 0; i < game->size; i++) {
        printf(" %2d ", i + 1);
        for (int j = 0; j < game->size; j++) {
            printf("| %c ", game->grid[i][j]);
        }
        printf("|\n");
        
        /* Print row separator */
        printf("    ");
        for (int j = 0; j < game->size; j++) {
            printf("+---");
        }
        printf("+\n");
    }
}

//MOVEMENT

int valid_move(Game *game, int r, int c) {
    if (r < 0 || c < 0 || r >= game->size || c >= game->size)
        return 0;
    if (game->grid[r][c] == WALL)
        return 0;
    return 1;
}

void move_player(Game *game, char move) {
    Player *p = &game->player;
    int nr = p->row, nc = p->col;

    move = toupper(move);

    if (move == 'W') nr--;
    else if (move == 'S') nr++;
    else if (move == 'A') nc--;
    else if (move == 'D') nc++;
    else if (move == 'Q') {
        p->active = 0;
        game->grid[p->row][p->col] = EMPTY;
        /* Restore extraction point if player was on it */
        if (p->row == game->extraction_row && p->col == game->extraction_col) {
            game->grid[p->row][p->col] = EXTRACTION;
        }
        log_state(game, "Player Quit");
        return;
    } else {
        p->lives--;
        log_state(game, "Invalid Input");
        return;
    }

    if (!valid_move(game, nr, nc)) {
        p->lives--;
        log_state(game, "Invalid Move");
        return;
    }

    /* Restore extraction point if player is leaving it */
    if (p->row == game->extraction_row && p->col == game->extraction_col) {
        game->grid[p->row][p->col] = EXTRACTION;
    } else {
        game->grid[p->row][p->col] = EMPTY;
    }

    char cell = game->grid[nr][nc];
    if (cell == INTEL) {
        p->intel++;
        printf("Collected Intel! (%d/%d)\n", p->intel, REQUIRED_INTEL);
    }
    else if (cell == LIFE) {
        p->lives++;
        printf("Gained a life!\n");
    }
    else if (cell == EXTRACTION) {
        if (p->intel >= REQUIRED_INTEL) {
            printf("\n*** YOU WIN! Successfully extracted with all intel! ***\n");
            p->active = 0;
            game->grid[nr][nc] = p->symbol;
            log_state(game, "WIN");
            return;
        } else {
            printf("\n*** MISSION FAILED! Reached extraction without enough intel! ***\n");
            printf("You were eliminated by headquarters for compromising the mission.\n");
            p->active = 0;
            p->lives = 0;
            log_state(game, "ELIMINATED - Insufficient Intel at Extraction");
            return;
        }
    }

    p->row = nr;
    p->col = nc;
    game->grid[nr][nc] = p->symbol;

    log_state(game, "Move");
}

//GAME STATE

void log_state(Game *game, const char *action) {
    if (game->log) {
        fprintf(game->log, "Action: %s\n", action);
        fprintf(game->log, "Lives: %d | Intel: %d/%d | Position: (%d, %d)\n\n",
                game->player.lives, game->player.intel, REQUIRED_INTEL,
                game->player.row + 1, game->player.col + 1);
        fflush(game->log);
    }
}

//MAIN GAME LOOP

void play_game(Game *game) {
    while (game->player.active && game->player.lives > 0) {
        display_grid(game);
        printf("\nLives: %d | Intel: %d/%d\n",
               game->player.lives, game->player.intel, REQUIRED_INTEL);
        printf("Position: Row %d, Col %d\n", 
               game->player.row + 1, game->player.col + 1);
        
        char move;
        printf("Enter move (W=Up, A=Left, S=Down, D=Right, Q=Quit): ");
        scanf(" %c", &move);
        
        move_player(game, move);
        
        if (game->player.lives <= 0) {
            printf("\nGame Over! You ran out of lives.\n");
            break;
        }
    }
}

// MAIN

int main() {
    srand(time(NULL));

    int size;
    printf("=== SpyNet Game - Part 1: Single Player ===\n");
    printf("Enter grid size (%d-%d): ", MIN_SIZE, MAX_SIZE);
    scanf("%d", &size);

    if (size < MIN_SIZE || size > MAX_SIZE) {
        printf("Invalid grid size!\n");
        return 1;
    }

    Game *game = init_game(size);
    play_game(game);
    free_game(game);

    return 0;
}
