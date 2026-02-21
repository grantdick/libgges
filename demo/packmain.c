#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <math.h>
#include <string.h>

#include <sys/time.h>

#include "mt19937ar.h"

/* imports the local gges system */
#include "gges.h"
#include "grammar.h"
#include "individual.h"

#include "pack.h"
#include "parameters.h"

static struct packing_instance **instances = NULL;
static int num_instances = 0;

static enum gap_lessthan_mode extract_threshold(char *symbol)
{
    if (strncmp(symbol, "average", 7) == 0) {
        return AVERAGE;
    } else if (strncmp(symbol, "minimum", 7) == 0) {
        return MINIMUM;
    } else {
        return MAXIMUM;
    }
}
static void execute(char *symbol, struct packing_instance *instance, bool *remove)
{
    char *num, *ignore, *remove_all, *threshold, *num_pieces;
    if (symbol == NULL) {
        return;
    }

    if (strncmp(symbol, "highest_filled", 14) == 0) {
        num        = strtok(NULL, " ");
        ignore     = strtok(NULL, " ");
        remove_all = strtok(NULL, " ");

        choose_highest_filled(instance, remove,
                              atoi(num),
                              atof(ignore),
                              strncmp(remove_all, "ALL", 3) == 0);
        execute(strtok(NULL, " "), instance, remove);
    } else if (strncmp(symbol, "lowest_filled", 13) == 0) {
        num        = strtok(NULL, " ");
        ignore     = strtok(NULL, " ");
        remove_all = strtok(NULL, " ");

        choose_lowest_filled(instance, remove,
                             atoi(num),
                             atof(ignore),
                             strncmp(remove_all, "ALL", 3) == 0);
        execute(strtok(NULL, " "), instance, remove);
    } else if (strncmp(symbol, "random_bins", 11) == 0) {
        num        = strtok(NULL, " ");
        ignore     = strtok(NULL, " ");
        remove_all = strtok(NULL, " ");

        choose_random_bins(instance, remove,
                           atoi(num),
                           atof(ignore),
                           strncmp(remove_all, "ALL", 3) == 0);
        execute(strtok(NULL, " "), instance, remove);
    } else if (strncmp(symbol, "gap_lessthan", 12) == 0) {
        num        = strtok(NULL, " ");
        threshold  = strtok(NULL, " ");
        ignore     = strtok(NULL, " ");
        remove_all = strtok(NULL, " ");

        choose_gap_lessthan(instance, remove,
                           atoi(num),
                           extract_threshold(threshold),
                           atof(ignore),
                           strncmp(remove_all, "ALL", 3) == 0);
        execute(strtok(NULL, " "), instance, remove);
    } else if (strncmp(symbol, "num_of_pieces", 13) == 0) {
        num        = strtok(NULL, " ");
        num_pieces = strtok(NULL, " ");
        ignore     = strtok(NULL, " ");
        remove_all = strtok(NULL, " ");

        choose_num_of_pieces(instance, remove,
                           atoi(num),
                           atoi(num_pieces),
                           atof(ignore),
                           strncmp(remove_all, "ALL", 3) == 0);
        execute(strtok(NULL, " "), instance, remove);
    } else if (strncmp(symbol, "remove_pieces_from_bins", 23) == 0) {
        remove_pieces_from_bins(instance, remove);
        execute(strtok(NULL, " "), instance, remove);
    } else if (strncmp(symbol, "best_fit_decreasing", 19) == 0) {
        repack_best_fit_decreasing(instance);
    } else if (strncmp(symbol, "worst_fit_decreasing", 20) == 0) {
        repack_worst_fit_decreasing(instance);
    } else if (strncmp(symbol, "first_fit_decreasing", 20) == 0) {
        repack_first_fit_decreasing(instance);
    }
}

static int local_search(struct packing_instance *instance, char *source)
{
    int i;
    struct packing_instance *solution;
	char *buffer;
    bool *remove;

    remove = malloc(instance->N * sizeof(bool));

    reset_packing_instance(instance);
    initial_packing(instance);
    solution = copy_packing_instance(instance);

    buffer = malloc(strlen(source) + 1);
	for (i = 0; i < 100; ++i) {
        replace_packing_instance(solution, instance);
        memset(remove, false, instance->N * sizeof(bool));

        strcpy(buffer, source);
		execute(strtok(buffer, " "), instance, remove);

        if (measure_instance_quality(solution) > measure_instance_quality(instance)) {
            replace_packing_instance(instance, solution);
        }
  	}

    delete_packing_instance(solution);

	free(buffer);
    free(remove);

    return number_of_bins_used(instance);
}



static double eval(struct gges_parameters *params __attribute__((unused)),
                   struct gges_individual *ind,
                   void *args __attribute__((unused)))
{
    double res;

    if (!ind->mapped) return DBL_MAX - 1.0;

    local_search(instances[0], ind->mapping->buffer);

    res = measure_instance_quality(instances[0]);

    ind->objective = 1 - res;

    return res;
}



/* function that gets run at the end of each generation, simply prints
 * out the best individual's evaluation score, and the number of
 * invalid individuals in the population */
static void reset(struct gges_parameters *params __attribute__((unused)),
                  int G __attribute__((unused)),
                  struct gges_individual **members __attribute__((unused)),
                  int N __attribute__((unused)),
                  void *args __attribute__((unused)))
{
    shuffle_packing_list(instances[0]);
}



/* function that gets run at the end of each generation, simply prints
 * out the best individual's evaluation score, and the number of
 * invalid individuals in the population */
static void report(struct gges_parameters *params __attribute__((unused)),
                   int G,
                   struct gges_individual **members, int N,
                   void *args __attribute__((unused)))
{
    int i, j, invalid, res, cnt;
    double best_objective, best_deviation, mean_test_deviation;

    invalid = 0;
    for (i = 0; i < N; ++i) if (!members[i]->mapped) invalid++;

    best_objective = (members[0]->mapped) ? members[0]->objective : NAN;

    if (members[0]->mapped) {
        mean_test_deviation = 0;
        best_deviation = local_search(instances[0], members[0]->mapping->buffer) - instances[0]->lower_bound;

        cnt = 0;
        for (i = 1; i < num_instances; ++i) {
            for (j = 0; j < 10; ++j) {
                cnt++;
                shuffle_packing_list(instances[i]);
                res = local_search(instances[i], members[0]->mapping->buffer);
                mean_test_deviation += (res - instances[i]->lower_bound - mean_test_deviation) / cnt;
            }
        }
    } else {
        mean_test_deviation = NAN;
        best_deviation = NAN;
    }

    fprintf(stdout, "%3d %9.6f %3.0f %8.3f %3d \"%s\"\n", G, best_objective, best_deviation, mean_test_deviation, invalid, members[0]->mapping->buffer);
    fflush(stdout);
}


static void load_instances(char *source)
{
    int i, j;
    int n_objects, lower_bound;
    double capacity, *sizes;

    FILE *src;

    src = fopen(source, "r");

    fscanf(src, "%d", &num_instances);

    sizes = NULL;
    instances = malloc(num_instances * sizeof(struct packing_instance *));
    for (i = 0; i < num_instances; ++i) {
        fscanf(src, "%lf %d %d", &capacity, &n_objects, &lower_bound);
        sizes = realloc(sizes, n_objects * sizeof(double));
        for (j = 0; j < n_objects; ++j) fscanf(src, "%lf", sizes + j);
        instances[i] = new_packing_instance(n_objects, n_objects, capacity, sizes, genrand_real2, lower_bound);
    }
    free(sizes);

    fclose(src);
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

    G = gges_load_bnf(argv[1]);

    load_instances(argv[2]);
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

    /* force certain parameters from original work (i.e., ignore config) */
    params->population_size = 50;
    params->generation_count = 50;
    params->cache_fitness = false;

    params->init_codon_count_min = 10;
    params->init_codon_count = 50;
    params->mapping_wrap_count = 10;
    params->maximum_mutation_depth = 6;

    pop = gges_run_system(params, G, eval, reset, report, NULL);

    gges_release_population(pop);

    free(params);

    while (num_instances--) delete_packing_instance(instances[num_instances]);
    free(instances);

    gges_release_grammar(G);

    return EXIT_SUCCESS;
}
