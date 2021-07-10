#include "mpi.h"

// Valore massimo generato da getRandomInt
#define MAX_VALUE 10

// Dimensione massima delle righe e delle colonne entro le quali posso stampare le matrici e i vettori
#define MAX_PRINT 9

// Seme usato per impostare rand()
#define SEED 13

// -------------------------------------------------------------------------------------------------

void printMatrix(int **data, int r, int c);

void printVectorAsMatrix(int *date, int r, int c);


int **populateMatrix(int r, int c);

int *populateDataArray(int n, int isRandom);

int getRandomInt(int maxValue);

void freeMatrix(int **data, int r);


void crea_griglia(MPI_Comm *g, MPI_Comm *gr, MPI_Comm *gc, int m, int r, int c, int *coords);

void prod_Matr_x_Matr(int *A, int *B, int *C, int m);

// -------------------------------------------------------------------------------------------------

