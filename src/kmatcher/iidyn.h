#ifndef __IIDYN_H__
#define __IIDYN_H__

int iidyn(const double *A, double *x, int size, double toll, int max_iters);
//int iidyn_m(const double *A, double *x, int size, double toll,bool *mask);

int repdyn(const double *A, double *x, int size, double toll);
int repdyn_v(const double *A, double *x, int size, double toll, double &error);

int clustering(const double *A, int *clusters, int size, int k);
int clustering_noreass(const double *A, int *clusters, int size, int k);

void print(const double *x, int size);
void print_m(const double *A, int size);

void mrand(double *A, int size, double offset=0, double density=1);
void vrand(double *x, int size);

#endif
