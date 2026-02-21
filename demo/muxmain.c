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

struct details {
    bool **data;

    int n;
    int b;
};

static bool **generate_data(unsigned int bits)
{
    unsigned int i, j, n, n_addr, addr;
    bool **data;

    n = 1 << bits;
    n_addr = floor(log(bits) / log(2));

    data = malloc(n * sizeof(bool *));
    data[0] = malloc((bits + 1) * n * sizeof(bool));
    for (i = 0; i < n; ++i) data[i] = data[0] + i * (bits + 1);

    for (i = 0; i < n; ++i) {
        for (j = 0; j < bits; ++j) data[i][j] = (i & (1 << j)) != 0;

        addr = 0;
        for (j = 0; j < n_addr; ++j) addr += (1 << j) * data[i][j];
        data[i][bits] = data[i][addr + n_addr];
    }

    return data;
}

static void free_data(bool **data)
{
    free(data[0]);
    free(data);
}

static void push_bit(bool val, bool **stack, int *n, int *s)
{
    if (*n >= *s) {
        while (*s <= *n) *s *= 2;
        *stack = realloc(*stack, *s * sizeof(bool));
    }

    (*stack)[(*n)++] = val;
}

static bool pop_bit(bool *stack, int *n)
{
    if (*n == 0) {
        fprintf(stderr, "ERROR: no bits on stack to pop.\n");
        exit(EXIT_FAILURE);
    }

    return stack[--(*n)];
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
    if (strncmp(token, "if", 2) == 0) return true;
    if (strncmp(token, "not", 3) == 0) return true;
    return false;
}

static bool function(char *token, bool *output, int *n_o)
{
    bool q, t, f;
    if (strncmp(token, "if", 2) == 0) {
        q = pop_bit(output, n_o);
        t = pop_bit(output, n_o);
        f = pop_bit(output, n_o);
        return q ? t : f;
    }

    if (strncmp(token, "not", 3) == 0) return !(pop_bit(output, n_o));

    fprintf(stderr, "ERROR: unknown function %s\n", token);
    exit(0);
    return false;
}

static bool is_operator(char *token)
{
    if (strncmp(token, "and", 3) == 0) return true;
    if (strncmp(token, "or", 2) == 0) return true;
    return false;
}

static int operator_precedence(char *token)
{
    if (strncmp(token, "or", 2) == 0) return 1;
    if (strncmp(token, "and", 3) == 0) return 2;
    return 0;
}

static bool operator(char *token, bool a, bool b)
{
    if (strncmp(token, "and", 3) == 0) return a && b;
    if (strncmp(token, "or", 2) == 0) return a || b;

    fprintf(stderr, "ERROR: unknown function %s\n", token);
    exit(0);
    return false;
}

static bool execute(char *buffer, bool *data)
{
    char *token, **ops, *o1, *o2;
    bool *output, res;
    int n_o, n_q, sz_o, sz_q;;

    sz_o = 8192;
    sz_q = 8192;
    ops = malloc(sz_q * sizeof(char *));
    output = malloc(sz_o * sizeof(bool));
    n_o = n_q = 0;


    for (token = strtok(buffer, " "); token; token = strtok(NULL, " ")) {
        if (strncmp(token, "b", 1) == 0) {
            push_bit(data[atoi(token + 1)], &output, &n_o, &sz_o);
        } else if (is_function(token)) {
            push_op(token, &ops, &n_q, &sz_q);
        } else if (strncmp(token, ",", 1) == 0) {
            while (strncmp(peek_op(ops, n_q), "(", 1) != 0) {
                token = pop_op(ops, &n_q);
                if (is_operator(token)) {
                    push_bit(operator(token, pop_bit(output, &n_o), pop_bit(output, &n_o)), &output, &n_o, &sz_o);
                } else {
                    push_bit(function(token, output, &n_o), &output, &n_o, &sz_o);
                }
            }
        } else if (is_operator(token)) {
            o1 = token;
            while (peek_op(ops, n_q) && is_operator(peek_op(ops, n_q))) {
                o2 = peek_op(ops, n_q);
                if (operator_precedence(o1) <= operator_precedence(o2)) {
                    push_bit(operator(pop_op(ops, &n_q), pop_bit(output, &n_o), pop_bit(output, &n_o)), &output, &n_o, &sz_o);
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
                    push_bit(operator(token, pop_bit(output, &n_o), pop_bit(output, &n_o)), &output, &n_o, &sz_o);
                } else {
                    push_bit(function(token, output, &n_o), &output, &n_o, &sz_o);
                }
            }

            token = pop_op(ops, &n_q); /* pop the left bracket */

            if (peek_op(ops, n_q) && is_function(peek_op(ops, n_q))) {
                push_bit(function(pop_op(ops, &n_q), output, &n_o), &output, &n_o, &sz_o);
            }
        } else {
            fprintf(stderr, "ERROR: unknown symbol: %s\n", token);
            exit(EXIT_FAILURE);
        }
    }

    while ((token = pop_op(ops, &n_q))) {
        if (is_operator(token)) {
            push_bit(operator(token, pop_bit(output, &n_o), pop_bit(output, &n_o)), &output, &n_o, &sz_o);
        } else {
            push_bit(function(token, output, &n_o), &output, &n_o, &sz_o);
        }
    }

    res = output[0];

    free(output);
    free(ops);

    return res;
}

static int measure(struct gges_individual *ind, bool **X, int n, int b)
{
    int i;
    bool q, r;
    int score;
    char *buffer;

    if (!ind->mapped) return n;

    score = 0;
    for (i = 0; i < n; ++i) {
        buffer = malloc(strlen(ind->mapping->buffer) + 1);
        strcpy(buffer, ind->mapping->buffer);

        q = X[i][b];
        r = execute(buffer, X[i]);

        score += (q == r) ? 1 : 0;

        free(buffer);
    }

    return (1 << b) - score;
}

static double eval(struct gges_parameters *params __attribute__((unused)), 
                   struct gges_individual *ind, void *args)
{
    struct details *data;

    data = args;

    ind->objective = data->n - measure(ind, data->data, data->n, data->b);

    return data->n - ind->objective;
}

/* function that gets run at the end of each generation, simply prints
 * out the best individual's evaluation score, and the number of
 * invalid individuals in the population */
static void report(struct gges_parameters *params __attribute__((unused)),
                   int G,
                   struct gges_individual **members, int N,
                   void *args __attribute__((unused)))
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

    struct details details;
    struct gges_population *pop;
    struct gges_parameters *params;
    struct gges_bnf_grammar *G;
    char bufvar[255];

    struct timeval t;
    gettimeofday(&t, NULL);
    init_genrand(t.tv_usec);

    details.b = atoi(argv[2]);
    details.n = 1 << details.b;
    details.data = generate_data(details.b);

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

    G = gges_load_bnf(argv[1]);
    /* add the features to the grammar, starting from the second
     * variable. The current implementation requires us to
     * "completely" specify the grammar in the source file, meaning
     * that no non-terminals can be left incomplete, therefore, we
     * have to include the first variable in the source file, and then
     * start from the second one */
    if (gges_grammar_has_non_terminal(G, "<bit>")) {
        for (i = 1; i < details.b; ++i) {
            sprintf(bufvar, "<bit> ::= b%d", i);
            gges_extend_grammar(G, bufvar, true);
        }
    } else {
        for (i = 1; i < details.b; ++i) {
            sprintf(bufvar, "<B> ::= b%d", i);
            gges_extend_grammar(G, bufvar, true);
        }
    }

    pop = gges_run_system(params, G, eval, NULL, report, &details);

    gges_release_population(pop);
    free(params);
    gges_release_grammar(G);

    free_data(details.data);

    return EXIT_SUCCESS;
}
