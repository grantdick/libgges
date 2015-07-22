#define  _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include "pack.h"

struct packing_instance *new_packing_instance(int N, int M, double C, double *s, double (*rnd)(void), int LB)
{
    struct packing_instance *instance;

    instance = malloc(sizeof(struct packing_instance));
    instance->lower_bound = LB;
    instance->rnd = rnd;
    instance->N = N;
    instance->M = M;
    instance->C = C;

    instance->s = malloc(N * sizeof(double));
    instance->b = malloc(N * sizeof(int));
    instance->u = malloc(M * sizeof(double));
    instance->n = malloc(M * sizeof(int));

    memcpy(instance->s, s, N * sizeof(double));

    return instance;
}



struct packing_instance *copy_packing_instance(struct packing_instance *instance)
{
    struct packing_instance *dest;

    dest = new_packing_instance(instance->N, instance->M, instance->C, instance->s, instance->rnd, instance->lower_bound);
    replace_packing_instance(dest, instance);

    return dest;
}



void replace_packing_instance(struct packing_instance *dest, struct packing_instance *src)
{
    memcpy(dest->b, src->b, dest->N * sizeof(int));
    memcpy(dest->u, src->u, dest->M * sizeof(double));
    memcpy(dest->n, src->n, dest->M * sizeof(int));
}



void delete_packing_instance(struct packing_instance *instance)
{
    if (instance == NULL) return;

    free(instance->s);
    free(instance->b);
    free(instance->u);
    free(instance->n);
    free(instance);
}



void reset_packing_instance(struct packing_instance *instance)
{
    int i;

    memset(instance->b, -1, instance->N * sizeof(int));
    for (i = 0; i < instance->M; ++i) instance->u[i] = 0;
    memset(instance->n,  0, instance->M * sizeof(int));
}










void shuffle_packing_list(struct packing_instance *instance)
{
    int i, j;
    double t;

    for (i = instance->N - 1; i > 0; --i) {
        j = (int)((i + 1) * instance->rnd());
        t = instance->s[i];
        instance->s[i] = instance->s[j];
        instance->s[j] = t;
    }
}










void initial_packing(struct packing_instance *instance)
{
    int i, j;

    /* use a first-fit allocation into the bins */
    for (i = 0; i < instance->N; ++i) {
        for (j = 0; j < instance->M; ++j) {
            if (instance->s[i] <= (instance->C - instance->u[j])) {
                /* we have space in this bin */
                instance->b[i] = j;
                instance->u[j] += instance->s[i];
                instance->n[j]++;
                break;
            }
        }
    }
}










static int collect_objects_in_bin(struct packing_instance *instance, int bin, int *objects)
{
    int i, n_obj;

    n_obj = 0;
    for (i = 0; i < instance->N; ++i) {
        if (instance->b[i] == bin) objects[n_obj++] = i;
    }

    return n_obj;
}

static int sort_bins_decreasing(const void *a, const void *b, void *u_ptr)
{
    int b1, b2;
    double *u;

    u = u_ptr;

    b1 = *((int const *)a);
    b2 = *((int const *)b);

    if (u[b1] > u[b2]) {
        return -1;
    } else if (u[b1] < u[b2]) {
        return 1;
    } else {
        return 0;
    }
}

static int sort_bins_increasing(const void *a, const void *b, void *u_ptr)
{
    int b1, b2;
    double *u;

    u = u_ptr;

    b1 = *((int const *)a);
    b2 = *((int const *)b);

    if (u[b1] < u[b2]) {
        return -1;
    } else if (u[b1] > u[b2]) {
        return 1;
    } else {
        return 0;
    }
}

void choose_highest_filled(struct packing_instance *instance, bool *remove,
                           int num, double ignore, bool remove_all)
{
    int *bins, n_bins; /* the bins to inspect */
    int *objs, n_objs; /* the objects in the current bin */
    int i, j;

    objs = malloc(instance->N * sizeof(int));

    bins = malloc(instance->M * sizeof(int));
    n_bins = 0;

    for (i = 0; i < instance->M; ++i) {
        if (instance->u[i] == 0) continue; /* skip empty bins */
        /* add bin if its utilisation is below the "ignore" threshold */
        if ((instance->u[i] / instance->C) < ignore) bins[n_bins++] = i;
    }
    qsort_r(bins, n_bins, sizeof(int), sort_bins_decreasing, instance->u);

    if (n_bins < num) num = n_bins;
    for (i = 0; i < num; ++i) {
        n_objs = collect_objects_in_bin(instance, bins[i], objs);

        if (remove_all) {
            for (j = 0; j < n_objs; ++j) remove[objs[j]] = true;
        } else {
            remove[objs[(int)(n_objs * instance->rnd())]] = true;
        }
    }

    free(objs);
    free(bins);
}



void choose_lowest_filled(struct packing_instance *instance, bool *remove,
                          int num, double ignore, bool remove_all)
{
    int *bins, n_bins; /* the bins to inspect */
    int *objs, n_objs; /* the objects in the current bin */
    int i, j;

    objs = malloc(instance->N * sizeof(int));

    bins = malloc(instance->M * sizeof(int));
    n_bins = 0;

    for (i = 0; i < instance->M; ++i) {
        if (instance->u[i] == 0) continue; /* skip empty bins */
        /* add bin if its utilisation is below the "ignore" threshold */
        if ((instance->u[i] / instance->C) < ignore) bins[n_bins++] = i;
    }
    qsort_r(bins, n_bins, sizeof(int), sort_bins_increasing, instance->u);

    if (n_bins < num) num = n_bins;
    for (i = 0; i < num; ++i) {
        n_objs = collect_objects_in_bin(instance, bins[i], objs);

        if (remove_all) {
            for (j = 0; j < n_objs; ++j) remove[objs[j]] = true;
        } else {
            remove[objs[(int)(n_objs * instance->rnd())]] = true;
        }
    }

    free(objs);
    free(bins);
}



void choose_random_bins(struct packing_instance *instance, bool *remove,
                        int num, double ignore, bool remove_all)
{
    int *bins, n_bins; /* the bins to inspect */
    int *objs, n_objs; /* the bins to inspect */
    int i, j;

    objs = malloc(instance->N * sizeof(int));

    bins = malloc(instance->M * sizeof(int));
    n_bins = 0;

    for (i = 0; i < instance->M; ++i) {
        if (instance->u[i] == 0) continue; /* skip empty bins */
        /* add bin if its utilisation is below the "ignore" threshold */
        if ((instance->u[i] / instance->C) < ignore) bins[n_bins++] = i;
    }

    if (n_bins < num) num = n_bins;
    for (i = 0; i < num; ++i) {
        do { j = (int)(n_bins * instance->rnd()); } while (bins[j] < 0);

        n_objs = collect_objects_in_bin(instance, bins[j], objs);
        bins[j] = -1;

        if (remove_all) {
            for (j = 0; j < n_objs; ++j) remove[objs[j]] = true;
        } else {
            remove[objs[(int)(n_objs * instance->rnd())]] = true;
        }
    }

    free(objs);
    free(bins);
}



void choose_gap_lessthan(struct packing_instance *instance, bool *remove,
                         int num, enum gap_lessthan_mode threshold, double ignore, bool remove_all)
{
    int *bins, n_bins; /* the bins to inspect */
    int *objs, n_objs; /* the bins to inspect */
    int i, j;
    double cutoff;

    if (threshold == AVERAGE) {
        cutoff = 0;
        for (i = 0; i < instance->N; ++i) {
            cutoff += (instance->s[i] - cutoff) / (i + 1);
        }
    } else if (threshold == MINIMUM) {
        cutoff = instance->s[0];
        for (i = 1; i < instance->N; ++i) {
            if (cutoff > instance->s[i]) cutoff = instance->s[i];
        }
    } else {
        cutoff = instance->s[0];
        for (i = 1; i < instance->N; ++i) {
            if (cutoff < instance->s[i]) cutoff = instance->s[i];
        }
    }

    objs = malloc(instance->N * sizeof(int));

    bins = malloc(instance->M * sizeof(int));
    n_bins = 0;

    for (i = 0; i < instance->M; ++i) {
        if (instance->u[i] == 0) continue;
        if ((instance->C - instance->u[i]) >= cutoff) continue;
        if ((instance->u[i] / instance->C) < ignore) bins[n_bins++] = i;
    }

    if (n_bins < num) num = n_bins;
    for (i = 0; i < num; ++i) {
        do { j = (int)(n_bins * instance->rnd()); } while (bins[j] < 0);

        n_objs = collect_objects_in_bin(instance, bins[j], objs);
        bins[j] = -1;

        if (remove_all) {
            for (j = 0; j < n_objs; ++j) remove[objs[j]] = true;
        } else {
            remove[objs[(int)(n_objs * instance->rnd())]] = true;
        }

    }

    free(objs);
    free(bins);
}



void choose_num_of_pieces(struct packing_instance *instance, bool *remove,
                          int num, int num_pieces, double ignore, bool remove_all)
{
    int *bins, n_bins; /* the bins to inspect */
    int *objs, n_objs; /* the bins to inspect */
    int i, j;

    objs = malloc(instance->N * sizeof(int));

    bins = malloc(instance->M * sizeof(int));
    n_bins = 0;

    for (i = 0; i < instance->M; ++i) {
        if (instance->u[i] == 0) continue;
        if (instance->n[i] != num_pieces) continue;
        if ((instance->u[i] / instance->C) < ignore) bins[n_bins++] = i;
    }

    if (n_bins < num) num = n_bins;
    for (i = 0; i < num; ++i) {
        do { j = (int)(n_bins * instance->rnd()); } while (bins[j] < 0);

        n_objs = collect_objects_in_bin(instance, bins[j], objs);
        bins[j] = -1;

        if (remove_all) {
            for (j = 0; j < n_objs; ++j) remove[objs[j]] = true;
        } else {
            remove[objs[(int)(n_objs * instance->rnd())]] = true;
        }
    }

    free(objs);
    free(bins);
}










void remove_pieces_from_bins(struct packing_instance *instance, bool *remove)
{
    int i, j, bin;

    for (i = 0; i < instance->N; ++i) {
        if (remove[i]) {
            bin = instance->b[i];

            /* remove the object from this bin */
            instance->u[bin] -= instance->s[i];
            instance->n[bin]--;

            /* mark the object as needing packing */
            instance->b[i] = -1;
        }
    }

    /* need to shuffle everything backwards */
    bin = 0;
    for (i = 0; i < instance->M; ++i) {
        if (instance->n[i] == 0) continue;

        for (j = 0; j < instance->N; ++j) {
            if (instance->b[j] == i) instance->b[j] = bin;
        }

        instance->u[bin] = instance->u[i];
        instance->n[bin] = instance->n[i];

        bin++;
    }

    for (i = bin; i < instance->M; ++i) {
        instance->u[i] = 0;
        instance->n[i] = 0;
    }
}










/* helper function to sort objects in decreasing order base upon their
 * sizes passed in through the s_ptr pointer */
static int compare_objects(const void *a, const void *b, void *s_ptr)
{
    int o1, o2;
    double *s;

    s = s_ptr;

    o1 = *((int const *)a);
    o2 = *((int const *)b);

    if (s[o1] > s[o2]) {
        return -1;
    } else if (s[o1] < s[o2]) {
        return 1;
    } else {
        return 0;
    }
}
static int order_remaining_objects(int **obj, int *x, double *s, int N)
{
    int i;
    int *o, n_rem;

    o = malloc(N * sizeof(int));

    n_rem = 0;
    for (i = 0; i < N; ++i) {
        if (x[i] < 0) o[n_rem++] = i;
    }

    qsort_r(o, n_rem, sizeof(int), compare_objects, s);

    *obj = o;
    return n_rem;
}

void repack_best_fit_decreasing(struct packing_instance *instance)
{
    int *obj, n_obj;
    int i, j, o, best;

    n_obj = order_remaining_objects(&obj, instance->b, instance->s, instance->N);

    /* now, do a best-fit allocation into the bins */
    for (i = 0; i < n_obj; ++i) {
        o = obj[i]; /* the next object to pack */

        best = -1;
        for (j = 0; j < instance->M; ++j) {
            if (instance->s[o] <= (instance->C - instance->u[j])) {
                /* we have space in this bin, is it any good? */
                if ((best < 0) || (instance->u[j] > instance->u[best])) best = j;
            }
        }

        instance->b[o] = best;
        instance->u[best] += instance->s[o];
        instance->n[best]++;
    }

    free(obj);
}

void repack_worst_fit_decreasing(struct packing_instance *instance)
{
    int *obj, n_obj;
    int i, j, o, best;

    n_obj = order_remaining_objects(&obj, instance->b, instance->s, instance->N);

    /* now, do a best-fit allocation into the bins */
    for (i = 0; i < n_obj; ++i) {
        o = obj[i]; /* the next object to pack */
        best = -1;

        for (j = 0; j < instance->M; ++j) {
            if (instance->s[o] <= (instance->C - instance->u[j])) {
                /* we have space in this bin, is it any good? */
                if ((best < 0) || (instance->u[j] < instance->u[best])) best = j;
            }
        }

        if (best < 0) continue; /* could not pack this item, which will only happen if M < N */
        instance->b[o] = best;
        instance->u[best] += instance->s[o];
        instance->n[best]++;
    }

    free(obj);
}

void repack_first_fit_decreasing(struct packing_instance *instance)
{
    int *obj, n_obj;
    int i, j, o;

    n_obj = order_remaining_objects(&obj, instance->b, instance->s, instance->N);

    /* now, do a first-fit allocation into the bins */
    for (i = 0; i < n_obj; ++i) {
        o = obj[i]; /* the next object to pack */
        for (j = 0; j < instance->M; ++j) {
            if (instance->s[o] <= (instance->C - instance->u[j])) {
                /* we have space in this bin */
                instance->b[o] = j;
                instance->u[j] += instance->s[o];
                instance->n[j]++;
                break;
            }
        }
    }

    free(obj);
}



int number_of_bins_used(struct packing_instance *instance)
{
    int i, n;

    n = 0;
    for (i = 0; i < instance->M; ++i) {
        if (instance->n[i] > 0) n++;
    }

    return n;
}










double measure_instance_quality(struct packing_instance *instance)
{
    int i;
    double tot, num_bins;

    tot = 0;
    num_bins = 0;
    for (i = 0; i < instance->M; ++i) {
        if (instance->n[i] == 0) continue;
        num_bins++;
        tot += instance->u[i] * instance->u[i];
    }

    return tot / (num_bins * instance->C * instance->C);
}
