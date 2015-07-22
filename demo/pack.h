/*
 * File:   pack.h
 * Author: gdick
 *
 * Created on 28th April 2011 at 11:00am
 */
#ifndef _PACK_H
#define	_PACK_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdbool.h>

    struct packing_instance {
        int lower_bound;     /* the known lower bound on the solution for this instance */
        double (*rnd)(void); /* the random number generator associated with this instance */

        int N;     /* the number of objects to pack */
        int M;     /* the number of bins */
        double C;  /* the capacity of each bin */

        double *s; /* the sizes of the pieces to pack */
        int    *b; /* the allocation of a piece to a bin, or -1 if not allocated */
        double *u; /* the utilisation of each bin */
        int    *n; /* the number of objects in each bin */
    };

    enum gap_lessthan_mode { AVERAGE, MINIMUM, MAXIMUM };

    struct packing_instance *new_packing_instance(int N, int M, double C, double *s, double (*rnd)(void), int LB);
    struct packing_instance *copy_packing_instance(struct packing_instance *instance);
    void delete_packing_instance(struct packing_instance *instance);
    void reset_packing_instance(struct packing_instance *instance);
    void replace_packing_instance(struct packing_instance *dest, struct packing_instance *src);

    void shuffle_packing_list(struct packing_instance *instance);

    void initial_packing(struct packing_instance *instance);

    void choose_highest_filled(struct packing_instance *instance, bool *remove,
                               int num, double ignore, bool remove_all);
    void choose_lowest_filled(struct packing_instance *instance, bool *remove,
                              int num, double ignore, bool remove_all);
    void choose_random_bins(struct packing_instance *instance, bool *remove,
                            int num, double ignore, bool remove_all);
    void choose_gap_lessthan(struct packing_instance *instance, bool *remove,
                             int num, enum gap_lessthan_mode threshold, double ignore, bool remove_all);
    void choose_num_of_pieces(struct packing_instance *instance, bool *remove,
                              int num, int num_pieces, double ignore, bool remove_all);

    void remove_pieces_from_bins(struct packing_instance *instance, bool *remove);

    void repack_best_fit_decreasing(struct packing_instance *instance);
    void repack_worst_fit_decreasing(struct packing_instance *instance);
    void repack_first_fit_decreasing(struct packing_instance *instance);

    int number_of_bins_used(struct packing_instance *instance);

    double measure_instance_quality(struct packing_instance *instance);

#ifdef	__cplusplus
}
#endif

#endif	/* _PACK_H */
