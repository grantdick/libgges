#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <unistd.h>

#include <float.h>
#include <math.h>
#include <string.h>

#include <sys/time.h>

#include "mt19937ar.h"

#include "gges.h"
#include "grammar.h"
#include "individual.h"

#include "data.h"
#include "parameters.h"

struct data_set_details {
    int n_features;

    double **train_X;
    double  *train_Y;
    int      n_train;
    double train_mean_rmse;

    double **test_X;
    double  *test_Y;
    int      n_test;
    double test_mean_rmse;
};

static double safe_divide(double a, double b)
{
    return (b == 0) ? 1 : a / b;
}
static double safe_log(double a)
{
    return (a == 0) ? 0 : log(fabs(a));
}

static void push_number(double val, double **stack, int *n, int *s)
{
    if (*n >= *s) {
        while (*s <= *n) *s *= 2;
        *stack = realloc(*stack, *s * sizeof(double));
    }

    (*stack)[(*n)++] = val;
}

static double pop_number(double *stack, int *n)
{
    return ((*n) == 0) ? NAN : stack[--(*n)];
}

static void push_op(char *op, char ***stack, int *n, int *s)
{
    if (*n >= *s) {
        while (*s <= *n) *s *= 2;
        *stack = realloc(*stack, *s * sizeof(char *));
    }

    (*stack)[(*n)++] = op;
}

static char *peek_op(char **stack, int n)
{
    return (n == 0) ? NULL : stack[n - 1];
}

static char *pop_op(char **stack, int *n)
{
    return ((*n) == 0) ? NULL : stack[--(*n)];
}

static bool is_function(char *token)
{
    if (strncmp(token, "inv", 3) == 0) return true;

    if (strncmp(token, "cos", 3) == 0) return true;
    if (strncmp(token, "sin", 3) == 0) return true;
    if (strncmp(token, "tan", 3) == 0) return true;
    if (strncmp(token, "log", 3) == 0) return true;
    if (strncmp(token, "exp", 3) == 0) return true;
    if (strncmp(token, "sqrt", 4) == 0) return true;

    if (strncmp(token, "neg", 3) == 0) return true;
    if (strncmp(token, "pow", 3) == 0) return true;

    if (strncmp(token, "plog", 4) == 0) return true;
    if (strncmp(token, "pdiv", 4) == 0) return true;
    if (strncmp(token, "psqrt", 5) == 0) return true;

    return false;
}

static double function(char *token, double *output, int *n_o)
{
    if (strncmp(token, "inv", 3) == 0) return 1 / pop_number(output, n_o);

    if (strncmp(token, "cos", 3) == 0) return cos(pop_number(output, n_o));
    if (strncmp(token, "sin", 3) == 0) return sin(pop_number(output, n_o));
    if (strncmp(token, "tan", 3) == 0) return tan(pop_number(output, n_o));
    if (strncmp(token, "log", 3) == 0) return log(pop_number(output, n_o));
    if (strncmp(token, "exp", 3) == 0) return exp(pop_number(output, n_o));
    if (strncmp(token, "sqrt", 4) == 0) return sqrt(pop_number(output, n_o));

    if (strncmp(token, "neg", 3) == 0) return -(pop_number(output, n_o));
    if (strncmp(token, "pow", 3) == 0) return pow(pop_number(output, n_o), pop_number(output, n_o));

    if (strncmp(token, "plog", 4) == 0) return safe_log(pop_number(output, n_o));
    if (strncmp(token, "pdiv", 4) == 0) return safe_divide(pop_number(output, n_o), pop_number(output, n_o));
    if (strncmp(token, "psqrt", 5) == 0) return sqrt(fabs(pop_number(output, n_o)));

    return NAN;
}

static bool is_operator(char *token)
{
    if (strncmp(token, "+", 1) == 0) return true;
    if (strncmp(token, "-", 1) == 0) return true;
    if (strncmp(token, "*", 1) == 0) return true;
    if (strncmp(token, "/", 1) == 0) return true;
    if (strncmp(token, "%", 1) == 0) return true;
    return false;
}

static int operator_precedence(char *token)
{
    if (strncmp(token, "+", 1) == 0) return 1;
    if (strncmp(token, "-", 1) == 0) return 1;
    if (strncmp(token, "*", 1) == 0) return 2;
    if (strncmp(token, "/", 1) == 0) return 2;
    if (strncmp(token, "%", 1) == 0) return 2;
    return 0;
}

static double operator(char *token, double a, double b)
{
    if (strncmp(token, "+", 1) == 0) return a + b;
    if (strncmp(token, "-", 1) == 0) return a - b;
    if (strncmp(token, "*", 1) == 0) return a * b;
    if (strncmp(token, "/", 1) == 0) return a / b;
    if (strncmp(token, "%", 1) == 0) return fmod(a, b);
    return NAN;
}

static double execute(char *buffer, double *x)
{
    char *token, **ops, *o1, *o2;
    double *output, res;
    int n_o, n_q, sz_o, sz_q;;

    sz_o = 8192;
    sz_q = 8192;
    ops = malloc(sz_q * sizeof(char *));
    output = malloc(sz_o * sizeof(double));
    n_o = n_q = 0;


    for (token = strtok(buffer, " "); token; token = strtok(NULL, " ")) {
        if (strncmp(token, "x", 1) == 0) {
            push_number(x[atoi(token + 1) - 1], &output, &n_o, &sz_o);
        } else if (is_function(token)) {
            push_op(token, &ops, &n_q, &sz_q);
        } else if (strncmp(token, ",", 1) == 0) {
            while (strncmp(peek_op(ops, n_q), "(", 1) != 0) {
                token = pop_op(ops, &n_q);
                if (is_operator(token)) {
                    push_number(operator(token, pop_number(output, &n_o), pop_number(output, &n_o)), &output, &n_o, &sz_o);
                } else {
                    push_number(function(token, output, &n_o), &output, &n_o, &sz_o);
                }
            }
        } else if (is_operator(token)) {
            o1 = token;
            while (peek_op(ops, n_q) && is_operator(peek_op(ops, n_q))) {
                o2 = peek_op(ops, n_q);
                if (operator_precedence(o1) <= operator_precedence(o2)) {
                    push_number(operator(pop_op(ops, &n_q), pop_number(output, &n_o), pop_number(output, &n_o)), &output, &n_o, &sz_o);
                } else {
                    break;
                }
            }
            push_op(o1, &ops, &n_q, &sz_q);
        } else if (strncmp(token, "(", 1) == 0) {
            push_op(token, &ops, &n_q, &sz_q);
        } else if (strncmp(token, ")", 1) == 0) {
            while (strncmp(peek_op(ops, n_q), "(", 1) != 0) {
                token = pop_op(ops, &n_q);
                if (is_operator(token)) {
                    push_number(operator(token, pop_number(output, &n_o), pop_number(output, &n_o)), &output, &n_o, &sz_o);
                } else {
                    push_number(function(token, output, &n_o), &output, &n_o, &sz_o);
                }
            }

            token = pop_op(ops, &n_q); /* pop the left bracket */

            if (peek_op(ops, n_q) && is_function(peek_op(ops, n_q))) {
                push_number(function(pop_op(ops, &n_q), output, &n_o), &output, &n_o, &sz_o);
            }
        } else {
            push_number(atof(token), &output, &n_o, &sz_o);
        }
    }

    while ((token = pop_op(ops, &n_q))) {
        if (is_operator(token)) {
            push_number(operator(token, pop_number(output, &n_o), pop_number(output, &n_o)), &output, &n_o, &sz_o);
        } else {
            push_number(function(token, output, &n_o), &output, &n_o, &sz_o);
        }
    }

    res = output[0];

    free(output);
    free(ops);

    return res;
}

static double measure_rmse(struct gges_individual *ind, double **X, double *Y, int n)
{
    int i;
    double y, yhat;
    double residual, mse;
    char *buffer;

    if (!ind->mapped) return DBL_MAX - 1.0;

    mse = 0;
    buffer = malloc(strlen(ind->mapping->buffer) + 1);
    for (i = 0; i < n; ++i) {
        strcpy(buffer, ind->mapping->buffer);

        y    = Y[i];
        yhat = execute(buffer, X[i]);
        residual = y - yhat;

        mse += ((residual * residual) - mse) / (i + 1);

    }
    free(buffer);

    return isfinite(mse) ? sqrt(mse) : (DBL_MAX - 1.0);
}

static double eval(struct gges_parameters *params, struct gges_individual *ind, void *args)
{
    struct data_set_details *data;

    data = args;

    ind->objective = measure_rmse(ind, data->train_X, data->train_Y, data->n_train);

    return 1 / (1 + ind->objective);
}

/* function that gets run at the end of each generation, simply prints
 * out the best individual's evaluation score, and the number of
 * invalid individuals in the population */
static void report(struct gges_parameters *params, int G,
                   struct gges_individual **members, int N,
                   void *args)
{
    struct data_set_details *details;
    double best_train, best_test;
    int i, invalid;

    details = args;

    invalid = 0;
    for (i = 0; i < N; ++i) if (!members[i]->mapped) invalid++;

    best_train = measure_rmse(members[0], details->train_X, details->train_Y, details->n_train);
    best_test  = measure_rmse(members[0], details->test_X, details->test_Y, details->n_test);

    fprintf(stdout, "%4d %10f %10f %10f %10f %d\n", G,
            best_train, best_test,
            best_train / details->train_mean_rmse,
            best_test / details->test_mean_rmse,
            invalid);
    fflush(stdout);
}

int main(int argc, char **argv)
{
    int i;

    struct data_set_details details;
    struct gges_population *pop;
    struct gges_parameters *params;
    struct gges_bnf_grammar *G;
    char bufvar[255];

    struct timeval t;
    gettimeofday(&t, NULL);
    init_genrand(t.tv_usec);

    load_fold(argv[2], argv[3], atoi(argv[4]),
              &(details.train_X), &(details.train_Y), &(details.n_train),
              &(details.test_X), &(details.test_Y), &(details.n_test),
              &(details.n_features),
              &(details.train_mean_rmse), &(details.test_mean_rmse));

    params = gges_default_parameters();
    params->rnd = genrand_real2;
    i = 5; while (i < argc) {
        if (strncmp(argv[i], "-p", 2) == 0) {
            process_parameter(argv[i + 1], params);
            i += 2;
        } else if (access(argv[i], F_OK) != -1) {
            parse_parameters(argv[i++], params);
        } else {
            i++;
        }
    }

    G = gges_load_bnf(argv[1]);
    /* add the features to the grammar, starting from the second
     * variable. The current implementation requires us to
     * "completely" specify the grammar in the source file, meaning
     * that no non-terminals can be left incomplete, therefore, we
     * have to include the first variable in the source file, and then
     * start from the second one */
    if (gges_grammar_has_non_terminal(G, "<var>")) {
        for (i = 2; i <= details.n_features; ++i) {
            sprintf(bufvar, "<var> ::= x%d", i);
            gges_extend_grammar(G, bufvar, true);
        }
    } else {
        for (i = 1; i <= details.n_features; ++i) {
            sprintf(bufvar, "<expr> ::= x%d", i);
            gges_extend_grammar(G, bufvar, true);
        }
    }

    pop = gges_run_system(params, G, eval, NULL, report, &details);

    gges_release_population(pop);
    free(params);
    gges_release_grammar(G);

    unload_data(details.train_X, details.train_Y, details.test_X, details.test_Y);

    return EXIT_SUCCESS;
}
