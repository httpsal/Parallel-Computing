#include <omp.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define MAX_VALUE 20
#define SEED 13

#define MAX_PRINT 50

/**
 * Sviluppare un algoritmo per il calcolo del prodotto matrice - vettore, in ambiente
 * di calcolo parallelo su achitettura MIMD a memoria condivisa, che utilizzi la
 * libreria OpenMP.
 *
 * Input:
 *     argv[1] - numero di righe della matrice
 *     argv[2] - numero di colonne della matrice
 *
 */

void printVector(int *data, int r);
void printMatrix(int **data, int r, int c);

int **populateMatrix(int r, int c);
int *populateDataArray(int n, int isRandom);
int getRandomInt(int maxValue);

int *matXvett(int **A, int *x, int rows, int col);


int main(int argc, char **argv){

    // Dimensioni della matrice
    int rows, columns;

    // Matrici e vettori: Ax
    int **matrix = NULL, *x = NULL;

    // Vettore risultato
    int *b = NULL;

    // Variabili per il calcolo del tempo di esecuzione
    struct timeval time;
    double start = 0, end = 0;

    // Recupero l'input dalla riga di comando
    if(argc < 3){
        printf("Missing input arguments. > /Main rows columns\n");
        return -1;
    }

    rows = atoi(argv[1]);
    columns = atoi(argv[2]);

    if(rows < 1 || columns < 1){
        printf("Rows (and columns) must be >= 1.\n");
        return -1;
    }

    // Creazione della matrice A
    matrix = populateMatrix(rows, columns);
    if(matrix == NULL){
        return -1;
    }

    if((rows <= MAX_PRINT) && (columns <= MAX_PRINT)){
        // Stampa la matrice se non ci sono troppi valori
        printf("A =\n");
        printMatrix(matrix, rows, columns);
        printf("\n");
    }


    //Creazione del vettore x
    x = populateDataArray(columns, 0);
    if(x == NULL){
        return -1;
    }

    if(columns <= MAX_PRINT){
        printf("x =");
        printVector(x, columns);
        printf("\n");
    }

    // Calcolo del prodotto matrice-vettore Ax = b

    gettimeofday(&time, NULL);
    start = time.tv_sec + (time.tv_usec / 1000000.0);

    b = matXvett(matrix, x, rows, columns);

    gettimeofday(&time, NULL);
    end = time.tv_sec + (time.tv_usec / 1000000.0);


    if(b == NULL){
        return -2;
    }

    // Stampa del risultato

    if(columns <= MAX_PRINT){
        printf("Results = ");
        printVector(b, columns);
        printf("\n");
    }

    printf("Matrix A = %d x %d, vector x = %d\n", rows, columns, columns);
    printf("Program ended in %e seconds.\n", end - start);

    return 0;
}

// -------------------------------------------------------------------------------------------------

/**
 * @brief Calcola il prodotto matrice vettore.
 * @param A Matrice bidimensionale.
 * @param x Vettore.
 * @param rows Numero di righe.
 * @param col Numero di colonne.
 * @return Vettore con il risultato o NULL in caso di errore.
 */
int *matXvett(int **A, int *x, int rows, int col){

    int i, j;

    int *b = (int *) calloc(rows, sizeof(int));

    if(b != NULL){

        //Inizio regione critica
        #pragma omp parallel for default (none) shared (A, x, b, rows, col) private (i,j)
        for (i = 0; i < rows; i++){
            for (j = 0; j < col; j++){
                b[i] += (A[i][j] * x[j]);
            }
        }
        // Fine regione critica
    }

    return b;
}

/**
 * @brief Stampa il contenuto del vettore.
 * @param data Vettore.
 * @param r Dimensione del vettore.
 */
void printVector(int *data, int r){

    int i;

    for(i=0; i < r; i++){
        printf(" %d", data[i]);
    }
    printf("\n");
}

/**
 * @brief Stampa il contenuto della matrice.
 * @param data Matrice
 * @param r Numero di righe.
 * @param c Numero di colonne.
 */
void printMatrix(int **data, int r, int c){

    int i, j;

    for(i=0; i < r; i++){
        for(j=0; j < c; j++){
            printf("%d\t", data[i][j]);
        }
        printf("\n");
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

