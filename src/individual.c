#include <stdlib.h>

#include <string.h>

#include "gges.h"
#include "grammar.h"
#include "individual.h"
#include "cfggp.h"
#include "ge.h"
#include "sge.h"

#include "alloc.h"

#define BUFFER_INC BUFSIZ





/*******************************************************************************
 * internal helper function prototypes
 ******************************************************************************/
static struct gges_mapping *create_mapping();

static void copy_mapping(struct gges_mapping *src, struct gges_mapping *dest);

static void release_mapping(struct gges_mapping *mapping);










/*******************************************************************************
 * Public function implementations
 ******************************************************************************/
struct gges_individual *gges_create_individual(struct gges_parameters *params)
{
    struct gges_individual *ind;

    ind = ALLOC(1, sizeof(struct gges_individual), false);
    ind->type = params->model;
    if (ind->type == GRAMMATICAL_EVOLUTION) {
        ind->representation.list = gges_ge_create_codon_list();
    } else if (ind->type == STRUCTURED_GRAMMATICAL_EVOLUTION) {
        ind->representation.genome = gges_sge_create_genome();
    } else {
        ind->representation.tree = NULL;
    }

    ind->mapping = create_mapping();

    ind->mapped = false;
    ind->evaluated = false;
    ind->fitness = GGES_WORST_FITNESS;

    return ind;
}



void gges_release_individual(struct gges_individual *ind)
{
    if (ind->type == GRAMMATICAL_EVOLUTION) {
        gges_ge_release_codon_list(ind->representation.list);
    } else if (ind->type == STRUCTURED_GRAMMATICAL_EVOLUTION) {
        gges_sge_release_genome(ind->representation.genome);
    } else {
        gges_cfggp_release_tree(ind->representation.tree);
    }

    release_mapping(ind->mapping);
    free(ind);
}



bool gges_map_individual(struct gges_parameters *params,
                         struct gges_bnf_grammar *g,
                         struct gges_individual *ind)
{
    if (ind->mapping->buffer == NULL) {
        /* strictly speaking, if we get here, then something has gone
         * wrong (e.g., the individual has not been initialised
         * through the correct constructor, or has been manipulated
         * outside the specified API, or there has been a memory
         * allocation problem). For now, we'll just assume that we can
         * fix the problem by reallocating the memory (it'll fail
         * quickly if allocation was the problem, anyway) */
        ind->mapping->sz = BUFFER_INC;
        ind->mapping->buffer = ALLOC(ind->mapping->sz, sizeof(char), false);
    }


    /* reset the phenotype to an empty string */
    memset(ind->mapping->buffer, '\0', ind->mapping->sz);

    if (ind->type == GRAMMATICAL_EVOLUTION) {
        ind->mapped = gges_ge_map_codons(g, ind->representation.list, ind->mapping,
                                  params->mapping_wrap_count);
    } else if (ind->type == STRUCTURED_GRAMMATICAL_EVOLUTION) {
        ind->mapped = gges_sge_map_genome(g, ind->representation.genome, ind->mapping);
    } else {
        ind->mapped = gges_cfggp_map_tree(ind->representation.tree, ind->mapping);
    }

    return ind->mapped;
}



struct gges_derivation_tree *gges_derive_individual(
    struct gges_parameters *params,
    struct gges_bnf_grammar *g,
    struct gges_individual *ind)
{
    if (ind->type == GRAMMATICAL_EVOLUTION) {
        return gges_ge_derive(g, ind->representation.list,
                              params->mapping_wrap_count);
    } else if (ind->type == STRUCTURED_GRAMMATICAL_EVOLUTION) {
        return gges_sge_derive(g, ind->representation.genome);
    } else {
        return gges_cfggp_derive(ind->representation.tree);
    }
}



void gges_init_individual(struct gges_parameters *params,
                          struct gges_bnf_grammar *g,
                          struct gges_individual *ind)
{
    if (ind->type == GRAMMATICAL_EVOLUTION) {
        if (params->sensible_initialisation) {
            gges_ge_sensible_init(g, ind->representation.list,
                                  params->init_min_depth,
                                  params->init_max_depth,
                                  params->sensible_init_tail_length,
                                  params->rnd);
        } else {
            if (params->init_codon_count_min < 0) {
                gges_ge_random_init(g, ind->representation.list,
                                    params->init_codon_count,
                                    params->rnd);
            } else {
                gges_ge_random_init(g, ind->representation.list,
                                    params->init_codon_count_min
                                    + (int)(params->rnd() * (params->init_codon_count
                                                             - params->init_codon_count_min)),
                                    params->rnd);
            }
        }
    } else if (ind->type == STRUCTURED_GRAMMATICAL_EVOLUTION) {
        if (params->sge_gene_sizes == NULL) {
            params->sge_genome_size = gges_sge_compute_gene_sizes(g, &(params->sge_gene_sizes));
        }
        gges_sge_random_init(g, ind->representation.genome,
                             params->sge_gene_sizes, params->sge_genome_size,
                             params->rnd);
    } else {
        if (params->sensible_initialisation) {
            gges_cfggp_sensible_init(g, &(ind->representation.tree),
                                     params->init_min_depth,
                                     params->init_max_depth,
                                     params->rnd);
        } else {
            gges_cfggp_random_init(g, &(ind->representation.tree),
                                   params->init_max_depth,
                                   params->rnd);
        }
    }

    ind->mapped = gges_map_individual(params, g, ind);
    ind->evaluated = false;
    ind->fitness = GGES_WORST_FITNESS;
}

void gges_reproduction(struct gges_parameters *params,
                       struct gges_individual *parent,
                       struct gges_individual *clone)
{
    if (params->model == GRAMMATICAL_EVOLUTION) {
        gges_ge_reproduction(parent->representation.list,
                             clone->representation.list);
    } else if (params->model == STRUCTURED_GRAMMATICAL_EVOLUTION) {
        gges_sge_reproduction(parent->representation.genome,
                              clone->representation.genome);
    } else {
        gges_cfggp_reproduction(parent->representation.tree,
                                &(clone->representation.tree));
    }

    copy_mapping(parent->mapping, clone->mapping);

    clone->mapped = parent->mapped;
    clone->evaluated = parent->evaluated;
    clone->fitness = parent->fitness;

    clone->objective = parent->objective;
}



void gges_breed(struct gges_parameters *params,
                struct gges_bnf_grammar *g,
                struct gges_individual *mother,
                struct gges_individual *father,
                struct gges_individual *daughter,
                struct gges_individual *son)
{
    bool cloned;
    if (params->model == GRAMMATICAL_EVOLUTION) {
        /* delegate to GE breeding functions */
        cloned = gges_ge_breed(mother->representation.list,
                               father->representation.list,
                               daughter->representation.list,
                               son->representation.list,
                               params->fixed_point_crossover,
                               params->crossover_rate, params->mutation_rate,
                               params->rnd);
    } else if (params->model == STRUCTURED_GRAMMATICAL_EVOLUTION) {
        /* delegate to Structured GE breeding functions */
        cloned = gges_sge_breed(g,
                                mother->representation.genome,
                                father->representation.genome,
                                daughter->representation.genome,
                                son->representation.genome,
                                params->crossover_rate, params->mutation_rate,
                                params->rnd);
    } else {
        /* delegate to CFGGP operators */
        cloned = gges_cfggp_breed(g, mother->representation.tree, father->representation.tree,
                                  &(daughter->representation.tree), &(son->representation.tree),
                                  params->maximum_mutation_depth, params->maximum_tree_depth,
                                  params->node_selection_method,
                                  params->crossover_rate, params->mutation_rate, params->rnd);
    }

    if (cloned) {
        copy_mapping(mother->mapping, daughter->mapping);
        daughter->mapped = mother->mapped;
        daughter->evaluated = mother->evaluated;
        daughter->fitness = mother->fitness;

        daughter->objective = mother->objective;

        copy_mapping(father->mapping, son->mapping);
        son->mapped = father->mapped;
        son->evaluated = father->evaluated;
        son->fitness = father->fitness;

        son->objective = father->objective;
    } else {
        daughter->mapped = son->mapped = false;
        daughter->evaluated = son->evaluated = false;
        daughter->fitness   = son->fitness   = GGES_WORST_FITNESS;
    }
}




void gges_mapping_append_symbol(struct gges_mapping *mapping, char *token)
{
    int tlen;

    if (mapping) {
        tlen = strlen(token);
        /* first, check to see if the string buffer needs
         * extending, and realloc as required */
        if ((mapping->l + tlen + 1) > mapping->sz) {
            while (mapping->sz < (mapping->l + tlen + 1)) mapping->sz += BUFFER_INC;

            mapping->buffer = REALLOC(mapping->buffer, mapping->sz, sizeof(char));
        }

        /* then, push terminal symbol into stream */
        strcat(mapping->buffer, token);
        mapping->l += tlen;
    } else {
        fprintf(stdout, "%s", token);
    }
}










/*******************************************************************************
 * internal helper function implementations
 ******************************************************************************/
static struct gges_mapping *create_mapping()
{
    struct gges_mapping *mapping;

    mapping = ALLOC(1, sizeof(struct gges_mapping), false);
    mapping->sz = BUFFER_INC;
    mapping->buffer = ALLOC(mapping->sz, sizeof(char), false);
    memset(mapping->buffer, '\0', mapping->sz);
    mapping->l = 0;
    return mapping;
}



static void copy_mapping(struct gges_mapping *src, struct gges_mapping *dest)
{
    if (dest->sz < (src->l + 1)) {
        while (dest->sz < (src->l + 1)) dest->sz += BUFFER_INC;
        dest->buffer = REALLOC(dest->buffer, dest->sz, sizeof(char));
    }

    strcpy(dest->buffer, src->buffer);
    dest->l = src->l;
}

static void release_mapping(struct gges_mapping *mapping)
{
    free(mapping->buffer);
    free(mapping);
}
