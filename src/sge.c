#include <stdlib.h>

#include <string.h>

#include "grammar.h"
#include "sge.h"

#include "alloc.h"

#define ONE_PER_GENE_MUTATION


/*******************************************************************************
 * internal helper function prototypes
 ******************************************************************************/
static void map_sequence(struct gges_mapping *m,
                         struct gges_sge_genome *genome,
                         struct gges_bnf_non_terminal *nt,
                         int *offset, int *cnt);

static void map_derivation(struct gges_derivation_tree **dest,
                           struct gges_bnf_grammar *g,
                           struct gges_sge_genome *genome,
                           struct gges_bnf_non_terminal *nt,
                           int *offset, int *cnt);

static int compute_gene_size(struct gges_bnf_non_terminal *nt,
                             struct gges_bnf_non_terminal *lhs);

static void ensure_correct_size(struct gges_sge_genome *genome,
                                struct gges_sge_genome *base);



/*******************************************************************************
 * Public function implementations
 ******************************************************************************/
int gges_sge_compute_gene_sizes(struct gges_bnf_grammar *g,
                                int **gene_sizes_ptr)
{
    int i, *gene_sizes, genome_size;
    struct gges_bnf_non_terminal *start;

    /* SGE only works on non-recursive grammars (it uses an expansion
     * trick to turn a recursive grammar into a non-recursive grammar,
     * which can be done offline) - if the grammar is recursive, then
     * we'd better let the user know so that they can fix it. */
    for (i = 0; i < g->size; ++i)  {
        if (g->non_terminals[i].recursive) {
            fprintf(stderr, "%s:%d - ERROR: SGE cannot operate on a recursive grammar, but "
                    "one has been supplied. You must convert the grammar into a "
                    "non-recursive alternative before it can be used. Quitting...\n",
                    __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }

    if (g->start == NULL) {
        /* the supplied grammar has no explicitly nominated start
         * symbol, so we will use the first defined non-terminal as
         * our starting point */
        start = g->non_terminals + 0;
    } else {
        start = g->start;
    }

    genome_size = 0;
    gene_sizes = ALLOC(g->size, sizeof(int), false);
    for (i = 0; i < g->size; ++i)  {
        gene_sizes[i] = compute_gene_size(g->non_terminals + i, start);
        /* fprintf(stderr, "%2d = %s = %d\n", i, g->non_terminals[i].label, gene_sizes[i]); */
        /* this takes account for non-terminals that are not used on
         * the RHS of a production (e.g., start symbols) */
        if (gene_sizes[i] == 0) gene_sizes[i]++;
        genome_size += gene_sizes[i];
    }

    *gene_sizes_ptr = gene_sizes;
    return genome_size;
}



struct gges_sge_genome *gges_sge_create_genome(void)
{
    struct gges_sge_genome *genome;

    genome = ALLOC(1, sizeof(struct gges_sge_genome), false);
    genome->genes = NULL;
    genome->gene_offset = NULL;
    genome->gene_size   = NULL;
    genome->n_genes = 0;
    genome->total_size = 0;

    return genome;
}



void gges_sge_release_genome(struct gges_sge_genome *genome)
{
    free(genome->gene_size);
    free(genome->gene_offset);
    free(genome->genes);
    free(genome);
}



bool gges_sge_random_init(struct gges_bnf_grammar *g,
                          struct gges_sge_genome *genome,
                          int *gene_sizes, int genome_size,
                          double (*rnd)(void))
{
    int i, nt;

    free(genome->gene_size);
    free(genome->gene_offset);
    free(genome->genes);

    genome->n_genes = g->size;
    genome->gene_offset = ALLOC(g->size, sizeof(int), true);
    genome->gene_size   = ALLOC(g->size, sizeof(int), true);

    /* compute the offset of each gene, this will also determine the
     * overall genome length */
    genome->total_size = 0;
    for (i = 0; i < g->size; ++i) {
        genome->gene_offset[i] = genome->total_size;
        genome->total_size += gene_sizes[i];
    }

    genome->genes = ALLOC(genome->total_size, sizeof(int), false);

    for (i = genome->total_size, nt = g->size - 1; i--;) {
        if (i < genome->gene_offset[nt]) nt--;
        genome->genes[i] = rnd() * g->non_terminals[nt].size;
    }

    return true;
}



bool gges_sge_map_genome(struct gges_bnf_grammar *g,
                         struct gges_sge_genome *genome,
                         struct gges_mapping *mapping)
{
    struct gges_bnf_non_terminal *start;

    if (g->start == NULL) {
        /* the supplied grammar has no explicitly nominated start
         * symbol, so we will use the first defined non-terminal as
         * our starting point */
        start = g->non_terminals + 0;
    } else {
        start = g->start;
    }

    memset(genome->gene_size, 0, genome->n_genes * sizeof(int));
    map_sequence(mapping, genome, start, genome->gene_offset, genome->gene_size);

    return true;
}



struct gges_derivation_tree *gges_sge_derive(struct gges_bnf_grammar *g,
                                             struct gges_sge_genome *genome)
{
    struct gges_derivation_tree *dt;
    struct gges_bnf_non_terminal *start;

    if (g->start == NULL) {
        /* the supplied grammar has no explicitly nominated start
         * symbol, so we will use the first defined non-terminal as
         * our starting point */
        start = g->non_terminals + 0;
    } else {
        start = g->start;
    }

    memset(genome->gene_size, 0, genome->n_genes * sizeof(int));
    map_derivation(&dt, g, genome, start, genome->gene_offset, genome->gene_size);

    return dt;
}



void gges_sge_reproduction(struct gges_sge_genome *p,
                           struct gges_sge_genome *o)
{
    ensure_correct_size(o, p);
    memcpy(o->genes, p->genes, p->total_size * sizeof(int));
    memcpy(o->gene_size, p->gene_size, p->n_genes * sizeof(int));
}

void gges_sge_crossover(struct gges_sge_genome *m,
                        struct gges_sge_genome *f,
                        struct gges_sge_genome *d,
                        struct gges_sge_genome *s,
                        double (*rnd)(void))
{
    int i, gene_start, gene_end;

    ensure_correct_size(d, m);
    ensure_correct_size(s, f);

    /* uniform crossover on genes, so we need to take account of the
     * gene boundaries. Given that we're storing everything in one
     * single array, and we don't store the actual gene sizes (only
     * their offsets), starting from the end of the array makes this a
     * little easier to perform */
    gene_start = m->total_size;
    for (i = m->n_genes; i--;) {
        gene_end = gene_start;
        gene_start = m->gene_offset[i];

        if (rnd() < 0.5) {
            memcpy(d->genes + gene_start, m->genes + gene_start, (gene_end - gene_start) * sizeof(int));
            memcpy(s->genes + gene_start, f->genes + gene_start, (gene_end - gene_start) * sizeof(int));
        } else {
            memcpy(d->genes + gene_start, f->genes + gene_start, (gene_end - gene_start) * sizeof(int));
            memcpy(s->genes + gene_start, m->genes + gene_start, (gene_end - gene_start) * sizeof(int));
        }
        d->gene_size[i] = s->gene_size[i] = (m->gene_size[i] > f->gene_size[i]) ? m->gene_size[i] : f->gene_size[i];
    }
}


/* the implementation of mutation in the python version of SGE
 * supplied by Nuno Lourenço is a one-mutation-per-gene operator (this
 * can be one interpretation of the mutation operator described in the
 * GP&EM paper). It operates only on the gene values that were used in
 * mapping, and ensures that the value changes under mutation (i.e.,
 * where multiple choices exist, mutation will select something
 * different from the current value in the genome).
 *
 * an alternative interpretation of the mutation operator is that it
 * walks over every position in the genome and mutates it with a
 * probability pm.
 *
 * In testing, the first operator seemed to be a bit more effective
 * than the blanket operator, but we make either available through a
 * compile-time option
 */
#ifdef ONE_PER_GENE_MUTATION
void gges_sge_mutation(struct gges_bnf_grammar *g,
                       struct gges_sge_genome *o,
                       double pm, double (*rnd)(void))
{
    int i, pos, cur;

    for (i = 0; i < o->n_genes; ++i) {
        if (g->non_terminals[i].size < 2) continue;
        if (rnd() >= pm) continue;
        pos = o->gene_offset[i] + (int)(rnd() * o->gene_size[i]);
        cur = o->genes[pos];
        do { o->genes[pos] = rnd() * g->non_terminals[i].size; } while (o->genes[pos] == cur);
    }
}
#else
void gges_sge_mutation(struct gges_bnf_grammar *g,
                       struct gges_sge_genome *o,
                       double pm, double (*rnd)(void))
{
    int i, nt;
    for (i = o->total_size, nt = g->size - 1; i--;) {
        if (i < o->gene_offset[nt]) nt--;
        if (rnd() < pm) o->genes[i] = rnd() * g->non_terminals[nt].size;
    }
}
#endif

bool gges_sge_breed(struct gges_bnf_grammar *g,
                    struct gges_sge_genome *m,
                    struct gges_sge_genome *f,
                    struct gges_sge_genome *d,
                    struct gges_sge_genome *s,
                    double pc, double pm,
                    double (*rnd)(void))
{
    if (rnd() < pc) {
        gges_sge_crossover(m, f, d, s, rnd);
    } else {
        gges_sge_reproduction(m, d);
        gges_sge_reproduction(f, s);
    }

    gges_sge_mutation(g, d, pm, rnd);
    gges_sge_mutation(g, s, pm, rnd);

    return false;
}










/*******************************************************************************
 * internal helper function implementations
 ******************************************************************************/
static void map_sequence(struct gges_mapping *m,
                         struct gges_sge_genome *genome,
                         struct gges_bnf_non_terminal *nt,
                         int *offset, int *cnt)
{
    int i;
    struct gges_bnf_production *p;

    /* look up the required production */
    p = nt->productions + genome->genes[offset[nt->id] + cnt[nt->id]++];

    for (i = 0; i < p->size; ++i) {
        if (p->tokens[i].terminal) {
            /* current token is a terminal, and needs to be printed
             * into the destination stream */
            gges_mapping_append_symbol(m, p->tokens[i].symbol);
        } else {
            /* the current token is a non-terminal, and so needs
             * further expansion. We do this via a recursive call to
             * the relevant production of the corresponding
             * non-terminal */
            map_sequence(m, genome, p->tokens[i].nt, offset, cnt);
        }
    }
}



static void map_derivation(struct gges_derivation_tree **dest,
                           struct gges_bnf_grammar *g,
                           struct gges_sge_genome *genome,
                           struct gges_bnf_non_terminal *nt,
                           int *offset, int *cnt)
{
    int i;
    struct gges_bnf_production *p;
    struct gges_derivation_tree *dt, *sub;

    *dest = NULL;

    /* look up the required production */
    p = nt->productions + genome->genes[offset[nt->id] + cnt[nt->id]++];

    /* create a new element in the derivation tree, rooted with the
     * current non-terminal */
    *dest = dt = gges_create_derivation_tree(p->size);
    dt->label = ALLOC(strlen(nt->label) + 1, sizeof(char), false);
    strcpy(dt->label, nt->label);

    /* then, copy each token in the current production into the
     * derivation tree */
    for (i = 0; i < p->size; ++i) {
        /* if the current token is a terminal, then place a single
         * element into the derivation tree containing the current
         * symbol, otherwise, we need to recursively build the
         * required derivation tree for the non-terminal symbol */
        if (p->tokens[i].terminal) {
            sub = gges_create_derivation_tree(0);
            sub->label = ALLOC(strlen(p->tokens[i].symbol) + 1, sizeof(char), false);
            strcpy(sub->label, p->tokens[i].symbol);
        } else {
            map_derivation(&sub, g, genome, p->tokens[i].nt, offset, cnt);
            if (sub == NULL) { /* derivation has failed, for some reason */
                sub = gges_create_derivation_tree(0);
                sub->label = ALLOC(strlen(p->tokens[i].symbol) + 2, sizeof(char), false);
                strcpy(sub->label, p->tokens[i].symbol);
                strcat(sub->label, "!");
            }
        }
        gges_add_sub_derivation(dt, sub);
    }
}



static int compute_gene_size(struct gges_bnf_non_terminal *nt,
                             struct gges_bnf_non_terminal *lhs)

{
    int i, j;
    int total, production_total;
    struct gges_bnf_non_terminal *rhs;


    total = 0;
    for (i = 0; i < lhs->size; ++i) {
        production_total = 0;

        for (j = 0; j < lhs->productions[i].size; ++j) {
            if (lhs->productions[i].tokens[j].terminal) continue;
            rhs = lhs->productions[i].tokens[j].nt;

            if (rhs == nt) {
                production_total++;
            } else {
                production_total += compute_gene_size(nt, rhs);
            }
        }

        if (production_total > total) total = production_total;
    }

    return total;
}



static void ensure_correct_size(struct gges_sge_genome *genome,
                                struct gges_sge_genome *base)
{
    if (genome->total_size != base->total_size) {
        genome->total_size = base->total_size;
        genome->genes = REALLOC(genome->genes, base->total_size, sizeof(int));
    }
    if (genome->n_genes != base->n_genes) {
        genome->n_genes = base->n_genes;
        genome->gene_offset = REALLOC(genome->gene_offset, base->n_genes, sizeof(int));
        memcpy(genome->gene_offset, base->gene_offset, base->n_genes * sizeof(int));

        genome->gene_size = REALLOC(genome->gene_size, base->n_genes, sizeof(int));
    }
}
