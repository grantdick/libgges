#ifndef GGES_SYSTEM
#define GGES_SYSTEM

#ifdef __cplusplus
extern "C" {
#endif

    #include <stdbool.h>

    struct gges_individual;
    struct gges_population;
    struct gges_bnf_grammar;
    struct gges_parameters;

    enum gges_model_type { CONTEXT_FREE_GP, GRAMMATICAL_EVOLUTION, STRUCTURED_GRAMMATICAL_EVOLUTION };
    enum gges_generation_method { RANDOM_SEARCH, GENERATIONAL, STEADY_STATE, CUSTOM };
    enum gges_cfggp_node_selection { PICK_NODE_UNIFORM_RANDOM, PICK_NODE_KOZA_90_10, PICK_NODE_DEPTH_PROP };

    typedef void (*GGES_BEFORE_GENERATION)(struct gges_parameters *, int,
                                           struct gges_individual **, int,
                                           void *);

    typedef void (*GGES_AFTER_GENERATION)(struct gges_parameters *, int,
                                          struct gges_individual **, int,
                                          void *);

    typedef double (*GGES_EVAL)(struct gges_parameters *,
                                struct gges_individual *,
                                void *);

    typedef bool (*GGES_ITERATION)(struct gges_parameters *,
                                   struct gges_bnf_grammar *,
                                   GGES_EVAL,
                                   struct gges_population *,
                                   struct gges_population *,
                                   void *);

    struct gges_parameters {
        enum gges_model_type model;

        /* global parameters, regardless of representation */
        int population_size;
        int generation_count;

        enum gges_generation_method generation_method;
        double elitism_factor; /* if this is less than 1, then this is
                                * the proportion of the population
                                * (sorted by fitness) that is carried
                                * into the next generation
                                * unchanged. If this is >= 1, then it
                                * is the exact number of individuals
                                * from the previous generation carried
                                * unchanged into the new generation */

        bool cache_fitness;

        int tournament_size;

        enum gges_cfggp_node_selection node_selection_method;
        double crossover_rate;
        double mutation_rate;

        bool sensible_initialisation;

        int init_min_depth;
        int init_max_depth;

        /* Grammatical Evolution-specific parameters */
        int init_codon_count_min;
        int init_codon_count;

        double sensible_init_tail_length;

        int mapping_wrap_count;

        bool fixed_point_crossover;

        /* CFG-GP-specific parameters */
        int maximum_mutation_depth;
        int maximum_tree_depth;

        /* Structured GE-specific parameters (mainly for
         * initialisation of the representation) */
        int *sge_gene_sizes;
        int sge_genome_size;

        GGES_EVAL eval;

        GGES_BEFORE_GENERATION before_gen;
        GGES_AFTER_GENERATION after_gen;

        GGES_ITERATION iteration;

        double (*rnd)(void);
    };

    struct gges_population {
        int N;

        struct gges_individual **members;
    };

    struct gges_parameters *gges_default_parameters();

    struct gges_population *gges_run_system(struct gges_parameters *params,
                                            struct gges_bnf_grammar *grammar,
                                            GGES_EVAL evaluator,
                                            GGES_BEFORE_GENERATION before_gen,
                                            GGES_AFTER_GENERATION after_gen,
                                            void *args);

    void gges_release_population(struct gges_population *pop);

#ifdef __cplusplus
}
#endif

#endif
