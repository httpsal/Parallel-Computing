#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utility.h"


/**
 * Elaborato 3:
 *
 * Sviluppare un algoritmo per il calcolo del prodotto matrice-matrice, in ambiente di calcolo parallelo su
 * architettura MIMD a memoria distribuita, che utilizzi la libreria MPI.
 * Le matrici A m x m e B m x m (quadrate) devono essere distribuite a p x p processi (per valori di m multipli di p),
 * disposti secondo una topologia a griglia bidimensionale.
 *
 * L’algoritmo deve implementare la strategia di comunicazione BMR ed essere organizzato in modo da utilizzare un
 * sottoprogramma per la costruzione della griglia bidimensionale dei processi ed un sottoprogramma per il calcolo
 * dei prodotti locali.
 *
 * Input:
 *     argv[1] - numero di righe/colonne delle matrici A e B
 *     argv[2] - dimensione della riga dei processori
 */

// ---------------------------------------------------- Main -----------------------------------------------------------


int main(int argc,char *argv[]) {

    // Identificativo e numero di processori
    int menum, nproc;

    // Dimensioni delle matrici
    int m, rowLoc, colLoc, rowG, colG;

    // Matrici input
    int **A = NULL, **B = NULL;

    // Risultato
    int *C = NULL;

    // Vettori locali
    int *subA, *subB, *subC;

    // Vettori temporanei
    int *ATemp, *BTemp, *temp = NULL;;

    // Coordinate del processore corrente
    int coords[2];

    // Griglie
    MPI_Comm grid, gridR, gridC;

    // Status e request per le operazioni di Recv e di Isend
    MPI_Status status;
    MPI_Request request;

    // Indici
    int i, j, k, index, startR, startC, endR, endC;

    // Coordinate ed id del processore che effettua il broadcast di subA
    int broadcaster[2], broadcasterId;

    // Coordinate ed id del processore che deve ricevere (e inviare) subB
    int sendBTo[2], sendBToId;
    int receiveBFrom[2], receiveBFromId;

    // Variabili per il calcolo dei tempi
    double t_start = 0, t_end = 0, t_local = 0, t_tot = 0;

    // Indici per la costruzione dell'array C
    int base = 0, offset = 0;

    // Numero di elementi di una sottomatrice
    int ndata = 0;

    // -----------------------------------------------------------------------------------------------------------------

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &menum);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);

    if(menum == 0){

        // Recupero dei valori dalla riga di comando

        if(argc != 3){
            printf("Wrong input arguments. \n> ./Main row_matrix row_proc\n");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        // Come richiesto, le matrici sono quadrate (m x m)
        m = atoi(argv[1]);

        rowG = atoi(argv[2]);

        if(rowG <= 0 || rowG > nproc){
            printf("Row of processors must be > 0 and < %d.\n", nproc + 1);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        colG = nproc / rowG;

        if(rowG != colG){
            // La griglia dei processori non e' quadrata
            printf("Processors grid must be p x p and not %d x %d.\n", rowG, colG);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        if(m < nproc){
            printf("Rows (and columns) must be >= than processors.\n");
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        if(m % rowG != 0){
            // m non e' multiplo di p
            printf("Rows (and columns) must be multiple of %d.\n", rowG);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        // Creazione delle matrici A e B
        A = populateMatrix(m, m);
        B = populateMatrix(m, m);

        if(A == NULL || B == NULL){
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
    }

    // Invio dati ai processori
    MPI_Bcast(&m, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&rowG, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&colG, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Creo la griglia di processori
    crea_griglia(&grid, &gridR, &gridC, menum, rowG, colG, coords);

    // ------------------------------------------------------------ //
    // Distribuzione delle sottomatrici ai processori della griglia


    // Calcolo le dimensioni della sottomatrice
    rowLoc = m / rowG;
    colLoc = m / colG;

    ndata = rowLoc * colLoc;

    // Creazione delle sottomatrici locali
    subA = (int *) calloc(ndata, sizeof(int));
    subB = (int *) calloc(ndata, sizeof(int));
    subC = (int *) calloc(ndata, sizeof(int));

    temp = (int *) calloc(ndata, sizeof(int));

    if(menum == 0){
        // Invio dei dati

        // Vettori "buffer" usati per l'invio dei dati ai processori
        ATemp = (int *) calloc(ndata, sizeof(int));
        BTemp = (int *) calloc(ndata, sizeof(int));

        // Inizializzazione degli indici
        startR = startC = endR = endC = 0;

        for(i = 0; i < nproc; i++){

            // Sono arrivato all'ultima colonna
            if(endC == m){
                endC = 0;
            }

            // Calcolo degli indici di partenza
            if(endC == 0){
                startR = endR;
                endR = endR + rowLoc;
            }

            startC = endC;
            endC = startC + colLoc;

            // Due cicli, per scorrere la sottomatrici ed inserire
            // gli elementi nei vettori da mandare ai processori.
            index = 0;

            for(j = startR; j < endR; j++){
                for(k = startC; k < endC; k++){

                    if(i == 0){
                        // Copia locale per il processore 0
                        subA[index] = A[j][k];
                        subB[index] = B[j][k];

                    } else {

                        ATemp[index] = A[j][k];
                        BTemp[index] = B[j][k];
                    }

                    index++;
                }
            }

            // Il processore i-esimo ricevera' il vettore appena inizializzato
            if(i != 0){
                MPI_Send(&ATemp[0], ndata, MPI_INT, i, i + 20, MPI_COMM_WORLD);
                MPI_Send(&BTemp[0], ndata, MPI_INT, i, i + 120, MPI_COMM_WORLD);
            }
        }

        // Libero la memoria occupata dai vettori "buffer" e dalle matrici degli input
        if(ATemp != NULL)
            free(ATemp);

        if(BTemp != NULL)
            free(BTemp);

    } else {
        // Lettura dei dati
        MPI_Recv(subA, ndata, MPI_INT, 0, menum + 20, MPI_COMM_WORLD, &status);
        MPI_Recv(subB, ndata, MPI_INT, 0, menum + 120, MPI_COMM_WORLD, &status);
    }

    // ------------------------------------------------------------ //
    // Calcolo del prodotto matrice-matrice con il metodo della BMR

    broadcaster[0] = coords[0];

    // Coordinate del processore a cui inviare subB
    sendBTo[0] = (coords[0] + colG - 1) % colG;
    sendBTo[1] = coords[1];

    MPI_Cart_rank(gridC, sendBTo, &sendBToId);

    // Coordinate del processore da cui ricevere subB
    receiveBFrom[0] = (coords[0] + 1) % colG;
    receiveBFrom[1] = coords[1];

    MPI_Cart_rank(gridC, receiveBFrom, &receiveBFromId);

    MPI_Barrier (MPI_COMM_WORLD);

    // Inizio della presa dei tempi

    t_start = MPI_Wtime();

    for(i=0; i < rowG; i++){

        // Coordinate del processore che deve inviare subA
        broadcaster[1] = (coords[0] + i) % rowG;

        if(i == 0){         // Primo passo

            if(coords[0] == coords[1]){
                // Imposto le coordinate del processore sulla diagonale
                broadcaster[1] = coords[1];

                // Copio il contenuto di subA in un blocco temporaneo
                memcpy(temp, subA, ndata * sizeof(int));
            }

            MPI_Cart_rank(gridR, broadcaster, &broadcasterId);

            MPI_Bcast(temp, ndata, MPI_INT, broadcasterId, gridR);

            // Calcolo il prodotto di una componente della matrice
            prod_Matr_x_Matr(temp, subB, subC, rowLoc);

        } else {            // Passi successivi

            // Broadcast di subA della diagonale + 1
            if(coords[1] == broadcaster[1]){

                // Copio il contenuto di subA in un blocco temporaneo
                memcpy(temp, subA, ndata * sizeof(int));
            }

            broadcasterId = (broadcasterId + 1)%rowG;

            MPI_Bcast(temp, ndata, MPI_INT, broadcasterId, gridR);

            // Rolling

            // Invio al processore "superiore"
            MPI_Isend(subB, ndata, MPI_INT, sendBToId, 30, gridC, &request);

            // Ricevo dal processore "inferiore"
            MPI_Recv(subB, ndata, MPI_INT, receiveBFromId, 30, gridC, &status);


            // Calcolo il prodotto di una componente della matrice
            prod_Matr_x_Matr(temp, subB, subC, rowLoc);
        }
    }

    t_end = MPI_Wtime();

    t_local = t_end - t_start;

    MPI_Reduce(&t_local, &t_tot, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    // Fine della presa dei tempi


    // Libero la memoria occupata dalle sottomatrici
    if(subA != NULL)
        free(subA);
    if(subB != NULL)
        free(subB);

    // ----------------------------------------- //
    // Il risultato si trovera' nel processore 0

    if(nproc > 1){
        if(menum == 0){

            index = 0;

            // Vettore di output
            C = (int *) calloc(m * m, sizeof(int));

            for(j=0; j < ndata; j++){

                C[index++] = subC[j];

                // Posizione della riga successiva
                if(index%rowLoc == 0){
                    index += (rowG - 1) * rowLoc;
                }
            }

            for(i=1; i < nproc; i++){

                // Ricezione della sottomatrice dal processore i
                MPI_Recv(temp, ndata, MPI_INT, i, 50 + i, MPI_COMM_WORLD, &status);

                if(i%rowG == 0){
                    // Ritorno alla sottomatrice nella prima colonna
                    base = base + rowLoc * colLoc * rowG;
                    offset = 0;

                } else {
                    // Mi sposto nella sottomatrice successiva
                    offset = offset + rowLoc;
                }

                index = base + offset;

                for(j=0; j < ndata; j++){

                    C[index++] = temp[j];

                    // Posizione della riga successiva
                    if(index%rowLoc == 0){
                        index += (rowG - 1) * rowLoc;
                    }
                }
            }

            if(subC != NULL)
                free(subC);

            if(temp != NULL)
                free(temp);

        } else {
            // Gli altri processori inviano il proprio risultato al processore 0
            MPI_Send (subC, ndata, MPI_INT, 0, 50 + menum, MPI_COMM_WORLD);
        }

    } else {
        // Se il processore utilizzato e' uno solo, allora
        // il risultato si trovera' proprio dentro a subC
        C = subC;
    }

    // Stampo il risultato
    if(menum == 0){

        if(m < MAX_PRINT){
            printf("A =");
            printMatrix(A, m, m);

            printf("\nB =");
            printMatrix(B, m, m);

            printf("\nA x B =");
            printVectorAsMatrix(C, m, m);
        }

        printf("A(%d x %d) * B(%d x %d) on %d processors.\n", m, m, m, m, nproc);
        printf("Program ended in %.4e seconds.\n", t_tot);

        freeMatrix(A, m);
        freeMatrix(B, m);

        if(C != NULL)
            free(C);
    }

    MPI_Finalize();

    return 0;
}

// ------------------------------------------------- Fine main ---------------------------------------------------------
