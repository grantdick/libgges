#ifndef GGES_SGE
#define GGES_SGE

#ifdef __cplusplus
extern "C" {
#endif

    #include <stdbool.h>
    #include <limits.h>

    #include "grammar.h"
    #include "derivation.h"
    #include "mapping.h"

    struct gges_sge_genome {
        /* SGE uses a fixed-length representation (the length of this
         * is determined by analysing the grammar), but maintains
         * information about the structure of the representation (it
         * is effectively an array of genes, with each gene a list of
         * instructions the inform the mapping process) to ensure that
         * nothing bad happens during the mapping process
         *
         * here, we model this as a single dimension integer array,
         * and note the offset of each gene in this array. When
         * mapping the genome into a derivation tree, we'll use this
         * offset to properly index the required value for
         * production */
        int *genes;

        int n_genes;      /* this should always be equal to the number
                           * of non-terminals in the grammar. However,
                           * we keep a copy of it just in case we need
                           * to perform an operation on the
                           * representation and the grammar has not
                           * been provided (e.g., crossover) */

        int *gene_offset; /* where does each gene start in the
                           * genome? */

        int *gene_size;   /* how many elements in the gene were used
                           * to map the individual? */

        int total_size;   /* the total size of the genome (the
                           * cumulative sum of the length of the list
                           * of each gene in the genome) */

    };

    /* walks the provided grammar to establish the required genome
     * size for each individual (and the size of the list required
     * each gene). Modifies the array passed in through gene_sizes and
     * returns the total length of the genome
     *
     * This uses a different approach than specified in the GP&EM
     * paper on SGE, as the algorithms in that paper contain a few
     * errors that make them difficult to implement correctly. The
     * method used here is essentially a walk down the possible
     * derivation trees to establish the upper bound on the number of
     * times a non-terminal will eventually be used - for relatively
     * small grammars, this won't be too much of a cost (and is only
     * done once during a run, normally), but may be a problem for
     * fairly large grammars
     */
    int gges_sge_compute_gene_sizes(struct gges_bnf_grammar *g,
                                    int **gene_sizes);

    struct gges_sge_genome *gges_sge_create_genome(void);
    void gges_sge_release_genome(struct gges_sge_genome *genome);


    /* runs the process that maps the genome into the corresponding
     * executable code via the supplied grammar
     *
     * returns true to indicate that the mapping was successful, which
     * it will always be in the case of SGE */
    bool gges_sge_map_genome(struct gges_bnf_grammar *g,
                             struct gges_sge_genome *genome,
                             struct gges_mapping *mapping);

    struct gges_derivation_tree *gges_sge_derive(struct gges_bnf_grammar *g,
                                                 struct gges_sge_genome *genome);

    /* uses information from the grammar (specifically, the number of
     * times a non-terminal is used, against the number of productions
     * against that non-terminal) to initialise the genome. The last
     * parameter is a pseudorandom number generator function pointer
     * that returns values in [0,1) */
    bool gges_sge_random_init(struct gges_bnf_grammar *g,
                              struct gges_sge_genome *genome,
                              int *gene_sizes, double (*rnd)(void));

    void gges_sge_reproduction(struct gges_sge_genome *p,
                              struct gges_sge_genome *o);

    bool gges_sge_breed(struct gges_bnf_grammar *g,
                        struct gges_sge_genome *m,
                        struct gges_sge_genome *f,
                        struct gges_sge_genome *d,
                        struct gges_sge_genome *s,
                        double pc, double pm,
                        double (*rnd)(void));

#ifdef __cplusplus
}
#endif

#endif
