#ifndef _DATA_H
#define	_DATA_H

#ifdef	__cplusplus
extern "C" {
#endif

    #include <stdio.h>

    void load_fold(char *source_file, char *fold_file, int fold,
                   double ***trainX, double **trainY, int *n_train,
                   double ***testX, double **testY, int *n_test,
                   int *n_features,
                   double *train_rmse, double *test_rmse);

    void unload_data(double **trainX, double *trainY,
                     double **testX, double *testY);

    char *next_line(char **buffer, size_t *sz, FILE *data);
    char *trim(char *str);

#ifdef	__cplusplus
}
#endif

#endif	/* _DATA_H */
