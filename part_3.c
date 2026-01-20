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
#define P2 '&'
#define P3 '$'

//STRUCTURES 

typedef struct {
    int row, col;
    int lives;
    int intel;
    int active;
    int is_human;
    char symbol;
} Player;

typedef struct {
    char **grid;
    int size;
    Player players[3];
    int current_turn;
    int extraction_row;
    int extraction_col;
    FILE *log;
} Game;

//FUNCTION DECLARATIONS 

Game* init_game(int size, int human_mask);
void free_game(Game *game);
void place_items(Game *game);
void display_grid(Game *game);
int valid_move(Game *game, int r, int c);
void move_player(Game *game, int idx, char move);
void human_turn(Game *game, int idx);
void computer_turn(Game *game, int idx);
void check_elimination(Game *game);
int count_active(Game *game);
void log_state(Game *game, const char *action);
void play_game(Game *game);

//GAME INITIALIZATION 

Game* init_game(int size, int human_mask) {
    Game *game = malloc(sizeof(Game));
    game->size = size;
    game->current_turn = 0;

    /* Allocate grid */
    game->grid = malloc(size * sizeof(char*));
    for (int i = 0; i < size; i++) {
        game->grid[i] = malloc(size * sizeof(char));
        for (int j = 0; j < size; j++)
            game->grid[i][j] = EMPTY;
    }

    /* Players */
    char symbols[] = {P1, P2, P3};

    for (int i = 0; i < 3; i++) {
        Player *p = &game->players[i];
        p->lives = INITIAL_LIVES;
        p->intel = 0;
        p->active = 1;
        p->symbol = symbols[i];
        p->is_human = (human_mask >> i) & 1;

        do {
            p->row = rand() % size;
            p->col = rand() % size;
        } while (game->grid[p->row][p->col] != EMPTY);

        game->grid[p->row][p->col] = p->symbol;
    }

    place_items(game);

    game->log = fopen("game_log.txt", "w");
    if (game->log) {
        fprintf(game->log, "=== SpyNet Game Log - Part 3 ===\n\n");
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

    for (int i = 0; i < 3; i++)
        if (game->players[i].active &&
            game->players[i].row == r &&
            game->players[i].col == c)
            return 0;

    return 1;
}

void move_player(Game *game, int idx, char move) {
    Player *p = &game->players[idx];
    int nr = p->row, nc = p->col;

    move = toupper(move);

    if (move == 'W') nr--;
    else if (move == 'S') nr++;
    else if (move == 'A') nc--;
    else if (move == 'D') nc++;
    else if (move == 'Q') {
        p->active = 0;
        /* Restore extraction point if player was on it */
        if (p->row == game->extraction_row && p->col == game->extraction_col) {
            game->grid[p->row][p->col] = EXTRACTION;
        } else {
            game->grid[p->row][p->col] = EMPTY;
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
        printf("Player %c collected Intel! (%d/%d)\n", p->symbol, p->intel, REQUIRED_INTEL);
    }
    else if (cell == LIFE) {
        p->lives++;
        printf("Player %c gained a life!\n", p->symbol);
    }
    else if (cell == EXTRACTION) {
        if (p->intel >= REQUIRED_INTEL) {
            printf("\n*** Player %c WINS! Successfully extracted with all intel! ***\n", p->symbol);
            p->active = 0;
            game->grid[nr][nc] = p->symbol;
            log_state(game, "WIN");
            return;
        } else {
            printf("\n*** Player %c ELIMINATED! Reached extraction without enough intel! ***\n", p->symbol);
            printf("Player %c was eliminated by headquarters for compromising the mission.\n", p->symbol);
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

//TURNS 

void human_turn(Game *game, int idx) {
    char move;
    printf("Enter move (W=Up, A=Left, S=Down, D=Right, Q=Quit): ");
    scanf(" %c", &move);
    move_player(game, idx, move);
}

void computer_turn(Game *game, int idx) {
    char moves[] = {'W','A','S','D'};
    char selected = moves[rand() % 4];
    printf("Computer plays: %c\n", selected);
    move_player(game, idx, selected);
}

//GAME STATE 

void check_elimination(Game *game) {
    for (int i = 0; i < 3; i++) {
        if (game->players[i].active && game->players[i].lives <= 0) {
            printf("Player %c has been eliminated!\n", game->players[i].symbol);
            game->players[i].active = 0;
            /* Restore extraction point if player was on it */
            if (game->players[i].row == game->extraction_row && 
                game->players[i].col == game->extraction_col) {
                game->grid[game->players[i].row][game->players[i].col] = EXTRACTION;
            } else {
                game->grid[game->players[i].row][game->players[i].col] = EMPTY;
            }
        }
    }
}

int count_active(Game *game) {
    int c = 0;
    for (int i = 0; i < 3; i++)
        if (game->players[i].active) c++;
    return c;
}

void log_state(Game *game, const char *action) {
    if (game->log) {
        fprintf(game->log, "Player %c: %s\n",
                game->players[game->current_turn].symbol, action);
        fprintf(game->log, "Lives: %d | Intel: %d/%d | Position: (%d, %d)\n\n",
                game->players[game->current_turn].lives,
                game->players[game->current_turn].intel,
                REQUIRED_INTEL,
                game->players[game->current_turn].row + 1,
                game->players[game->current_turn].col + 1);
        fflush(game->log);
    }
}

//MAIN GAME LOOP 

void play_game(Game *game) {
    int winner_found = 0;
    
    while (1) {
        int active = count_active(game);
        
        if (active == 0) {
            printf("\nGame Over! All players eliminated.\n");
            break;
        }
        
        if (active == 1 && !winner_found) {
            for (int i = 0; i < 3; i++) {
                if (game->players[i].active) {
                    printf("\nGame Over! Player %c is the last survivor!\n", game->players[i].symbol);
                    break;
                }
            }
            break;
        }

        Player *p = &game->players[game->current_turn];
        if (!p->active) {
            game->current_turn = (game->current_turn + 1) % 3;
            continue;
        }

        display_grid(game);
        printf("\nPlayer %c | Lives: %d | Intel: %d/%d | Position: Row %d, Col %d\n",
               p->symbol, p->lives, p->intel, REQUIRED_INTEL, p->row + 1, p->col + 1);

        if (p->is_human)
            human_turn(game, game->current_turn);
        else
            computer_turn(game, game->current_turn);

        check_elimination(game);
        
        /* Check if someone won */
        if (!p->active && p->intel >= REQUIRED_INTEL) {
            winner_found = 1;
            break;
        }
        
        game->current_turn = (game->current_turn + 1) % 3;
    }
}

//MAIN 

int main() {
    srand(time(NULL));

    int size, opt;
    printf("=== SpyNet Game - Part 3: Three Player ===\n");
    
    printf("Enter grid size (%d-%d): ", MIN_SIZE, MAX_SIZE);
    scanf("%d", &size);

    if (size < MIN_SIZE || size > MAX_SIZE) {
        printf("Invalid grid size!\n");
        return 1;
    }

    printf("1. HHH (3 Humans)\n2. HHC (2 Humans, 1 Computer)\n3. HCC (1 Human, 2 Computers)\nChoice: ");
    scanf("%d", &opt);
    
    int mask = (opt == 1) ? 0b111 : (opt == 2) ? 0b011 : 0b001;
    Game *game = init_game(size, mask);
    play_game(game);
    free_game(game);

    return 0;
}
