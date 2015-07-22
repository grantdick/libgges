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

#define NUM_STEPS 400
#define GRID_ROWS 32
#define GRID_COLS 32

enum orientation { NORTH, EAST, SOUTH, WEST };

static bool grid[GRID_ROWS][GRID_COLS];

static int remaining_steps = NUM_STEPS;
static int consumed = 0;
static enum orientation agent_orientation = EAST;
static int agent_row = 0;
static int agent_col = 0;

static bool food_ahead()
{
    int next_row, next_col;

    switch (agent_orientation) {
    case NORTH: default: next_row = agent_row + 1; next_col = agent_col; break;
    case EAST: next_row = agent_row; next_col = agent_col + 1; break;
    case SOUTH: next_row = agent_row - 1; next_col = agent_col; break;
    case WEST: next_row = agent_row; next_col = agent_col - 1; break;
    }

    if (next_row < 0) next_row = GRID_ROWS - 1;
    if (next_col < 0) next_col = GRID_COLS - 1;

    if (next_row >= GRID_ROWS) next_row = 0;
    if (next_col >= GRID_COLS) next_col = 0;

    return grid[next_row][next_col];
}

static void turn_left()
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

static void turn_right()
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

static void move_forward()
{
    int next_row, next_col;

    if (remaining_steps > 0) {
        remaining_steps--;

        switch (agent_orientation) {
        case NORTH: default: next_row = agent_row + 1; next_col = agent_col; break;
        case EAST: next_row = agent_row; next_col = agent_col + 1; break;
        case SOUTH: next_row = agent_row - 1; next_col = agent_col; break;
        case WEST: next_row = agent_row; next_col = agent_col - 1; break;
        }

        if (next_row < 0) next_row = GRID_ROWS - 1;
        if (next_col < 0) next_col = GRID_COLS - 1;

        if (next_row >= GRID_ROWS) next_row = 0;
        if (next_col >= GRID_COLS) next_col = 0;

        if (grid[next_row][next_col]) consumed++; /* food ahead */
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

    if (!ind->mapped) return DBL_MAX - 1.0;

    grid[ 1][ 0] = grid[ 2][ 0] = grid[ 3][ 0] = grid[ 3][ 1] = grid[ 3][ 2] =
        grid[ 3][ 3] = grid[ 3][ 4] = grid[ 3][ 5] = grid[ 4][ 5] = grid[ 5][ 5] =
        grid[ 6][ 5] = grid[ 8][ 5] = grid[ 9][ 5] = grid[10][ 5] = grid[11][ 5] =
        grid[12][ 5] = grid[12][ 6] = grid[12][ 7] = grid[12][ 8] = grid[12][ 9] =
        grid[12][11] = grid[12][12] = grid[12][13] = grid[12][14] = grid[12][17] =
        grid[12][18] = grid[12][19] = grid[12][20] = grid[12][21] = grid[12][22] =
        grid[12][23] = grid[11][24] = grid[10][24] = grid[ 9][24] = grid[ 8][24] =
        grid[ 7][24] = grid[ 4][24] = grid[ 3][24] = grid[ 1][25] = grid[ 1][26] =
        grid[ 1][27] = grid[ 1][28] = grid[ 2][30] = grid[ 3][30] = grid[ 4][30] =
        grid[ 5][30] = grid[ 7][29] = grid[ 7][28] = grid[ 8][27] = grid[ 9][27] =
        grid[10][27] = grid[11][27] = grid[12][27] = grid[13][27] = grid[14][27] =
        grid[16][26] = grid[16][25] = grid[16][24] = grid[16][21] = grid[16][20] =
        grid[16][19] = grid[16][18] = grid[17][15] = grid[20][14] = grid[20][13] =
        grid[20][10] = grid[20][ 9] = grid[20][ 8] = grid[20][ 7] = grid[21][ 5] =
        grid[22][ 5] = grid[24][ 4] = grid[24][ 3] = grid[25][ 2] = grid[26][ 2] =
        grid[27][ 2] = grid[29][ 3] = grid[29][ 4] = grid[29][ 6] = grid[29][ 9] =
        grid[29][11] = grid[28][13] = grid[27][13] = grid[26][13] = grid[23][14] =
        grid[24][17] = grid[27][18] = grid[26][21] = grid[23][22] = true;

    remaining_steps = NUM_STEPS;
    consumed = 0;
    agent_orientation = EAST;
    agent_row = 0;
    agent_col = 0;

    buffer = malloc(strlen(ind->mapping->buffer) + 1);
	while (remaining_steps > 0) {
        strcpy(buffer, ind->mapping->buffer);
		execute(strtok(buffer, " "));
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
    i = 2; while (i < argc) {
        if (strncmp(argv[i], "-p", 2) == 0) {
            process_parameter(argv[i + 1], params);
            i += 2;
        } else {
            parse_parameters(argv[i++], params);
        }
    }

    G = gges_load_bnf(argv[1]);

    pop = gges_run_system(params, G, eval, NULL, report, NULL);

    gges_release_population(pop);
    free(params);
    gges_release_grammar(G);

    return EXIT_SUCCESS;
}
