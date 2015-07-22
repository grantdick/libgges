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

static double result = 0.0;

static void execute(char *token)
{
    if (!token) return;
    result += atof(token);
    execute(strtok(NULL, " "));
}

static double eval(struct gges_parameters *params, struct gges_individual *ind, void *args)
{
	char *buffer;

    if (ind->mapped) {
        result = 0.0;

        buffer = malloc(strlen(ind->mapping->buffer) + 1);
        strcpy(buffer, ind->mapping->buffer);

        execute(strtok(buffer, " "));
        free(buffer);
    } else {
        result = DBL_MAX - 1.0;
    }

    ind->objective = result;

    return 1 / (1 + result);
}

/* function that gets run at the end of each generation, simply prints
 * out the best individual's evaluation score, the number of invalid
 * individuals in the population, and the best individual's mapping */
static void report(struct gges_parameters *params, int G,
                   struct gges_individual **members, int N,
                   void *args)
{
    int i, invalid;
    double best_objective;

    invalid = 0;
    for (i = 0; i < N; ++i) if (!members[i]->mapped) invalid++;

    best_objective = (members[0]->mapped) ? members[0]->objective : NAN;
    fprintf(stdout, "%3d %9.6f %3d [[ %s ]]\n", G, best_objective, invalid, members[0]->mapping->buffer);
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
