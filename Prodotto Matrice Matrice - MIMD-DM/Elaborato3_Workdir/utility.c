#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utility.h"


/**
 * @brief Calcola il prodotto matrice-matrice tra A e B inserendo poi il risultato in C.
 *        Le matrici sono tutte quadrate.
 * @param A Matrice input.
 * @param B Matrice input.
 * @param C Matrice output.
 * @param m Dimensione delle righe (colonne).
 */
void prod_Matr_x_Matr(int *A, int *B, int *C, int m){

    int i, j, h;

    if(A == NULL || B == NULL || C == NULL){
        printf("prod_Matr_x_Matr error.\n");
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    for (i = 0; i < m; i++){
        for (j = 0; j < m; j++){
            for (h = 0; h < m; h++){
                C[i*m + h] += A[i*m + j] * B[j*m + h];
            }
        }
    }
}


/**
 * @brief Stampa il contenuto della matrice.
 * @param data Matrice.
 * @param r Numero di righe.
 * @param c Numero di colonne.
 */
void printMatrix(int **data, int r, int c){

    int i, j;

    for(i=0; i < r; i++){
        for(j=0; j < c; j++){
            printf("\t%d", data[i][j]);
        }
        printf("\n");
    }
}

/**
 * @brief Stampa il contenuto di un vettore come una matrice bidimensionale.
 * @param data Vettore da stampare.
 * @param r Numero di righe.
 * @param c Numero di colonne.
 */
void printVectorAsMatrix(int *data, int r, int c){

    int i;

    for(i=0; i < r * c; i++){
        printf("\t%d", data[i]);

        if((i + 1)%c == 0){
            printf("\n");
        }
    }
    printf("\n");
}

/**
 * @brief Crea ed inizializza una griglia di processori.
 * @param griglia Paremetro in output.
 * @param grigliar Parametro in output.
 * @param grigliac Parametro in output.
 * @param menum Identificativo del processore corrente.
 * @param riga Numero di righe.
 * @param col Numero di colonne.
 * @param coordinate Parametro in output. Coordinate del processore sulla griglia.
 */
void crea_griglia(MPI_Comm *grid, MPI_Comm *gridr, MPI_Comm *gridc,
                        int menum, int riga, int col, int *coordinate){

    int dim = 2, *ndim = NULL, reorder, *period = NULL, vc[2];

    ndim = (int *) calloc(dim, sizeof(int));
    ndim[0] = riga;
    ndim[1] = col;

    period = (int *) calloc(dim, sizeof(int));
    period[0] = period[1] = 1;

    reorder = 0;

    MPI_Cart_create(MPI_COMM_WORLD, dim, ndim, period, reorder, grid);
    MPI_Cart_coords(*grid, menum, dim, coordinate);

    // Creazione della griglia delle righe
    vc[0] = 0;
    vc[1] = 1;

    MPI_Cart_sub(*grid, vc, gridr);

    // Creazione della griglia delle colonne
    vc[0] = 1;
    vc[1] = 0;
    MPI_Cart_sub(*grid, vc, gridc);

    return;
}

/**
 * @brief Libera la memoria occupata dalla matrice.
 * @param data Matrice da deallocare.
 * @param r Lunghezza delle righe.
 */
void freeMatrix(int **data, int r){

    int i;
    if(data != NULL){
        for(i=0; i < r; i++){
            free(data[i]);
        }
        data = NULL;
    }
}

/**
 * @brief Crea ed inizializza una matrice con dei valori casuali.
 * @param r Numero di righe.
 * @param c Numero di colonne.
 * @return La matrice inizializzata o NULL in caso di errore.
 */
int **populateMatrix(int r, int c){

    int **data = (int **) malloc(sizeof(int *) * r), i;

    if(data == NULL){
        printf("malloc error!\n");
        return NULL;
    }

    for(i=0; i < r; i++){
        srand(SEED + i);
        data[i] = populateDataArray(c, 1);
    }

    return data;
}

/**
 * @brief Crea ed inizializza un array con dei valori casuali.
 * @param n Dimensione dell'array.
 * @param isRandom Se e' uguale a 1, crea un vettore con dei valori casuali.
 * @return L'array inizializzato o NULL in caso di errore.
 */
int *populateDataArray(int n, int isRandom){

    int *data = (int *) malloc(sizeof(int) * n), i;

    if(data == NULL){
        printf("malloc error!\n");
        return NULL;
    }

    if(isRandom == 1){
        for(i=0; i < n; i++){
            data[i] = getRandomInt(MAX_VALUE);
        }
    } else {
        for(i=0; i < n; i++){
            data[i] = (i%9) + 1;
        }
    }

    return data;
}

/**
 * @brief Genera un valore casuale che va da 1 a maxValue.
 * @param maxValue Valore massimo da generare.
 * @return Valore casuale.
 */
int getRandomInt(int maxValue) {
    return (rand() % maxValue) + 1;   // Random number between 1 and maxValue
}


