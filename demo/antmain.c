#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <float.h>
#include <math.h>
#include <string.h>

#include <sys/time.h>

#include "mt19937ar.h"

#include "gges.h"
#include "grammar.h"
#include "individual.h"

#include "parameters.h"

#define NUM_FOOD  89
#define GRID_ROWS 32
#define GRID_COLS 32

static int NUM_STEPS = 600;

enum orientation { NORTH, EAST, SOUTH, WEST };

static bool grid[GRID_ROWS][GRID_COLS];

static int total_steps = 0;
static int remaining_steps = 0;
static int consumed = 0;
static enum orientation agent_orientation = EAST;
static int agent_row = 0;
static int agent_col = 0;

static void reset_ant(int num_steps)
{
    grid[ 0][ 1] = grid[ 0][ 2] = grid[ 0][ 3] = grid[ 1][ 3] = grid[ 2][ 3] =
    grid[ 3][ 3] = grid[ 4][ 3] = grid[ 5][ 3] = grid[ 5][ 4] = grid[ 5][ 5] =
    grid[ 5][ 6] = grid[ 5][ 8] = grid[ 5][ 9] = grid[ 5][10] = grid[ 5][11] =
    grid[ 5][12] = grid[ 6][12] = grid[ 7][12] = grid[ 8][12] = grid[ 9][12] =
    grid[11][12] = grid[12][12] = grid[13][12] = grid[14][12] = grid[17][12] =
    grid[18][12] = grid[19][12] = grid[20][12] = grid[21][12] = grid[22][12] =
    grid[23][12] = grid[24][11] = grid[24][10] = grid[24][ 9] = grid[24][ 8] =
    grid[24][ 7] = grid[24][ 4] = grid[24][ 3] = grid[25][ 1] = grid[26][ 1] =
    grid[27][ 1] = grid[28][ 1] = grid[30][ 2] = grid[30][ 3] = grid[30][ 4] =
    grid[30][ 5] = grid[29][ 7] = grid[28][ 7] = grid[27][ 8] = grid[27][ 9] =
    grid[27][10] = grid[27][11] = grid[27][12] = grid[27][13] = grid[27][14] =
    grid[26][16] = grid[25][16] = grid[24][16] = grid[21][16] = grid[20][16] =
    grid[19][16] = grid[18][16] = grid[15][17] = grid[14][20] = grid[13][20] =
    grid[10][20] = grid[ 9][20] = grid[ 8][20] = grid[ 7][20] = grid[ 5][21] =
    grid[ 5][22] = grid[ 4][24] = grid[ 3][24] = grid[ 2][25] = grid[ 2][26] =
    grid[ 2][27] = grid[ 3][29] = grid[ 4][29] = grid[ 6][29] = grid[ 9][29] =
    grid[12][29] = grid[14][28] = grid[14][27] = grid[14][26] = grid[15][23] =
    grid[18][24] = grid[19][27] = grid[22][26] = grid[23][23] = true;

    total_steps = remaining_steps = num_steps;
    consumed = 0;
    agent_orientation = EAST;
    agent_row = 0;
    agent_col = 0;
}

static bool food_ahead(void)
{
    int next_row, next_col;

    switch (agent_orientation) {
    case NORTH: default: next_row = agent_row - 1; next_col = agent_col; break;
    case EAST: next_row = agent_row; next_col = agent_col + 1; break;
    case SOUTH: next_row = agent_row + 1; next_col = agent_col; break;
    case WEST: next_row = agent_row; next_col = agent_col - 1; break;
    }

    if (next_row < 0) next_row = GRID_ROWS - 1;
    if (next_col < 0) next_col = GRID_COLS - 1;

    if (next_row >= GRID_ROWS) next_row = 0;
    if (next_col >= GRID_COLS) next_col = 0;

    return grid[next_row][next_col];
}

static void turn_left(void)
{
    if (remaining_steps > 0) {
        remaining_steps--;

        switch (agent_orientation) {
        case NORTH: default: agent_orientation = WEST; break;
        case EAST: agent_orientation = NORTH; break;
        case SOUTH: agent_orientation = EAST; break;
        case WEST: agent_orientation = SOUTH; break;
        }
    }
}

static void turn_right(void)
{
    if (remaining_steps > 0) {
        remaining_steps--;

        switch (agent_orientation) {
        case NORTH: default: agent_orientation = EAST; break;
        case EAST: agent_orientation = SOUTH; break;
        case SOUTH: agent_orientation = WEST; break;
        case WEST: agent_orientation = NORTH; break;
        }
	}
}

static void move_forward(void)
{
    int next_row, next_col;

    if (remaining_steps > 0) {
        remaining_steps--;

        switch (agent_orientation) {
        case NORTH: default: next_row = agent_row - 1; next_col = agent_col; break;
        case EAST: next_row = agent_row; next_col = agent_col + 1; break;
        case SOUTH: next_row = agent_row + 1; next_col = agent_col; break;
        case WEST: next_row = agent_row; next_col = agent_col - 1; break;
        }

        if (next_row < 0) next_row = GRID_ROWS - 1;
        if (next_col < 0) next_col = GRID_COLS - 1;

        if (next_row >= GRID_ROWS) next_row = 0;
        if (next_col >= GRID_COLS) next_col = 0;

        if (grid[next_row][next_col]) consumed++;
        grid[next_row][next_col] = false;

        agent_row = next_row;
        agent_col = next_col;
    }
}



static void jump(char *symbol)
{
    if (symbol == NULL) {
        fprintf(stderr, "Premature end to expression!\n");
		exit(-1);
    }

    if (strncmp(symbol, "ifa", 3) == 0) {
		jump(strtok(NULL, " "));
		jump(strtok(NULL, " "));
    } else if (strncmp(symbol, "tl", 2) == 0) {
        return;
    } else if (strncmp(symbol, "tr", 2) == 0) {
        return;
    } else if (strncmp(symbol, "mv", 2) == 0) {
        return;
    } else if (strncmp(symbol, "prog2", 5) == 0) {
        jump(strtok(NULL, " "));
        jump(strtok(NULL, " "));
    } else if (strncmp(symbol, "prog3", 5) == 0) {
        jump(strtok(NULL, " "));
        jump(strtok(NULL, " "));
        jump(strtok(NULL, " "));
    } else if (strncmp(symbol, "begin", 5) == 0) {
        do {
            symbol = strtok(NULL, " ");
            jump(symbol);
        } while (strncmp(symbol, "end", 3) != 0);
    }
}

static void execute(char *symbol)
{
    if (symbol == NULL) {
        fprintf(stderr, "Premature end to expression!\n");
		exit(-1);
    }

    if (strncmp(symbol, "ifa", 3) == 0) {
		if (food_ahead()) {
			execute(strtok(NULL, " "));
			jump(strtok(NULL, " "));
		} else {
			jump(strtok(NULL, " "));
			execute(strtok(NULL, " "));
		}
    } else if (strncmp(symbol, "tl", 2) == 0) {
        turn_left();
    } else if (strncmp(symbol, "tr", 2) == 0) {
        turn_right();
    } else if (strncmp(symbol, "mv", 2) == 0) {
        move_forward();
    } else if (strncmp(symbol, "prog2", 5) == 0) {
        execute(strtok(NULL, " "));
        execute(strtok(NULL, " "));
    } else if (strncmp(symbol, "prog3", 5) == 0) {
        execute(strtok(NULL, " "));
        execute(strtok(NULL, " "));
        execute(strtok(NULL, " "));
    } else if (strncmp(symbol, "begin", 5) == 0) {
        do {
            symbol = strtok(NULL, " ");
            execute(symbol);
        } while (strncmp(symbol, "end", 3) != 0);
    }
}

static double eval(struct gges_parameters *params, struct gges_individual *ind, void *args)
{
	char *buffer;
    int used_steps;

    if (!ind->mapped) return DBL_MAX - 1.0;

    reset_ant(NUM_STEPS);

    buffer = malloc(strlen(ind->mapping->buffer) + 1);
	while (remaining_steps > 0) {
        strcpy(buffer, ind->mapping->buffer);

        used_steps = remaining_steps;
		execute(strtok(buffer, " "));
        used_steps -= remaining_steps;
        if (used_steps == 0) break; /* for an infinite loop */
	}

	free(buffer);

    ind->objective = 89 - consumed;

    return consumed;
}

/* function that gets run at the end of each generation, simply prints
 * out the best individual's evaluation score, and the number of
 * invalid individuals in the population */
static void report(struct gges_parameters *params, int G,
                   struct gges_individual **members, int N,
                   void *args)
{
    int i, invalid;
    double best_objective;

    invalid = 0;
    for (i = 0; i < N; ++i) if (!members[i]->mapped) invalid++;

    best_objective = (members[0]->mapped) ? members[0]->objective : NAN;
    fprintf(stdout, "%3d %9.6f %3d\n", G, best_objective, invalid);
    fflush(stdout);
}

int main(int argc, char **argv)
{
    int i;

    struct gges_population *pop;
    struct gges_parameters *params;
    struct gges_bnf_grammar *G;

    struct timeval t;
    gettimeofday(&t, NULL);
    init_genrand(t.tv_usec);

    params = gges_default_parameters();
    params->rnd = genrand_real2;
    i = 3; while (i < argc) {
        if (strncmp(argv[i], "-p", 2) == 0) {
            process_parameter(argv[i + 1], params);
            i += 2;
        } else {
            parse_parameters(argv[i++], params);
        }
    }

    NUM_STEPS = atoi(argv[2]);

    G = gges_load_bnf(argv[1]);

    pop = gges_run_system(params, G, eval, NULL, report, NULL);

    gges_release_population(pop);
    free(params);
    gges_release_grammar(G);

    return EXIT_SUCCESS;
}
