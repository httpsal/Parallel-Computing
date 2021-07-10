#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>

#include "mpi.h"

#define BUF_LEN 10
#define SEED 13

// Crea ed inizializza l'array con i dati che devono essere inviati agli altri processori.
int *populateDataArray(int N);

// Restituisce un numero intero casuale nel range 1 --> maxValue.
int getRandomInt(int maxValue);

// Calcola la somma dato l'array data e la sua dimensione.
int getSum(int *data, int dim);

// Calcola il logaritmo di base 2 del valore passato in input.
double logb2(int x);

// Stampa un semplice messaggio informativo sulla strategia adottata.
void printStrategyInfo(int current_processor, int s);


int strategy1(int current_processor, int tot_processor, int sum);
int strategy2(int current_processor, int tot_processor, int sum);
int strategy3(int current_processor, int tot_processor, int sum);

/*
 * Programma per il calcolo della somma di N numeri reali.
 * 
 * L'algoritmo deve prendere in input:
 *     argv[1] - numero N di numeri da sommare,
 *                 - prendere in input i numeri se N <= 20 (da file con il comando "< dati.txt")
 *                 - generare i numeri random se N > 20
 * 
 *     argv[2] - numero di strategia da utilizzare
 * 
 *     argv[3] - opzionale. ID del processore che deve stampare il risultato
 *                 - se ID > -1 stampa il processore ID
 *                 - se ID = -1 stampano tutti i processori
 * 
 * Implementa la I, II e III strategia di comunicazione tra processori
 * Se viene richiesta la II o la III strategia ma queste non sono applicabili, viene applicata la I 
 * strategia.
 */
int main(int argc, char **argv)
{
    
    int ID = -1,    // Processore che deve stampare il risultato
        N,          // Numero degli interi da sommare
    strategia;      // Strategia da usare
    
    int current_processor = 0,  // Processore corrente
    tot_processor = 0,          // Totale processori
    n_loc;                      // Dimensione dell'array locale dei dati
    
    int rest, tmp, sum = 0;
    
    int i, start, tag;  // Indici ed etichetta usata durante la comunicazione
    
    int *data = NULL, *xLoc = NULL;     // Array di input e array locale dei dati
    
    MPI_Status status;  // Status della comunicazione
    
    double t_start = 0, t_end = 0, t_local = 0, t_tot = 0;	// Variabili per il calcolo dei tempi
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &current_processor);
    MPI_Comm_size(MPI_COMM_WORLD, &tot_processor);
    
    // Fase 1: recupero l'input
    if(current_processor == 0){
        
        if(argc >= 3){
            // Avro' come minimo il numero di numeri da sommare ed il tipo di strategia.
            N = atoi(argv[1]);
            strategia = atoi(argv[2]);
            
            if(N <= 0 || strategia < 1 || strategia > 3){
                printf("Invalid arguments N=%d, strategy=%d\n", N, strategia);
                MPI_Finalize();
                return -1;
            }
            
            if(argc == 4){
                // Prendo l'id del processore che deve stampare il risultato. 
                // Se non e' presente, o è errato, avro' come valore di default -1
                ID = atoi(argv[3]);
                if(ID < -1 || ID > tot_processor - 1){
                    printf("Invalid processor ID (%d): using defalut value...\n", ID);
                    ID = -1;
                }
            }
            
        }else{
            printf("Missing arguments!\n");
            printf("%s N strategy [ID]\n", argv[0]);
            MPI_Finalize();
            return -1;
        }
        
        
        // Fase 2: setto l'array con i dati da sommare
        data = populateDataArray(N);
        if(data == NULL){
            printf("populateDataArray error!\n");
            MPI_Finalize();
            return -1;
        }
    }
    
    // Fase 3: invio dei dati 
    
    // Invio l'id del processore che deve stampare il risultato
    MPI_Bcast(&ID, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Invio la strategia da usare
    MPI_Bcast(&strategia, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Il processore 0 comunica l'intero N agli altri processori
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    if(N < tot_processor){
        printf("Not enough data for all this processors...\n");
        MPI_Finalize();
        return -1;
    }
    
    n_loc = N / tot_processor;
    rest = N % tot_processor;
    
    if(current_processor < rest){
        n_loc++;
    }
    
    
    xLoc = (int *) malloc(sizeof(int *) * n_loc);
    
    if(current_processor == 0){
        // Invio dei dati
        tmp = n_loc;
        start = 0;
        
        for(i = 1; i < tot_processor; i++){
            start += tmp;
            
            tag = 22 + i;
            if(i == rest){
                tmp--;
            }
            MPI_Send(&data[start], tmp, MPI_INT, i, tag, MPI_COMM_WORLD);
            
        }
        
        // Copia locale dei dati
        for(i = 0; i < n_loc; i++){
            xLoc[i] = data[i];
        }
        
        free(data);
        data = NULL;
        
    }else{
        // Ricezione dei dati
        tag = 22 + current_processor;
        MPI_Recv(xLoc, n_loc, MPI_INT, 0, tag, MPI_COMM_WORLD, &status);
    }
    
    /*printf("Processor (%d) received: ", current_processor);
    for(i = 0; i < n_loc; i++){
        printf("%d ", xLoc[i]);
    }
    printf("\n");*/
    
    MPI_Barrier(MPI_COMM_WORLD);
    t_start = MPI_Wtime();
    
    // Fase 4: calcolo della somma locale
    sum = getSum(xLoc, n_loc);
    
    //printf("Processor (%d): Partial sum: %d\n", current_processor, sum);
    
    // Fase 5: scelta della strategia da adottare
    if(strategia == 2)
        sum = strategy2(current_processor, tot_processor, sum);
    else if(strategia == 3)
        sum = strategy3(current_processor, tot_processor, sum);
    else 
        sum = strategy1(current_processor, tot_processor, sum);
    
    t_end = MPI_Wtime();
    
    t_local = t_end - t_start;
    
    MPI_Reduce(&t_local, &t_tot, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    
    // Fase 6: stampa del risultato
    
    if(strategia == 1 || strategia == 2){
        if(ID == -1){
            // Tutti devono ricevere il risultato
            MPI_Bcast(&sum, 1, MPI_INT, 0, MPI_COMM_WORLD);
            
        }else if(ID != 0){
            // Solo un processore deve ricevere il risultato
            if(current_processor == 0){
                MPI_Send(&sum, 1, MPI_INT, ID, 10 + ID, MPI_COMM_WORLD);
            }else if(current_processor == ID){
                MPI_Recv(&sum, 1, MPI_INT, 0, 10 + ID, MPI_COMM_WORLD, &status);
            }
        }
    }
    
    
    // Il processore selezionato stampa la somma (o tutti)
    if(current_processor == ID || ID == -1){
        printf("Processor (%d): Sum = %d\n", current_processor, sum);
    }
    
    // Il processore 0 stampa le informazioni sui tempi di esecuzione.
    if(current_processor == 0){
        // Tempo in microsecondi
        printf("Strategy = %d, N = %d, Elapsed time = %.2f\n", strategia, N, t_tot * 1000000);
    }
    
    //printf("\n---------------\nClosing up (%d)...\n", current_processor);
    
    MPI_Finalize();
    return 0;
}


/**
 * @brief Crea ed inizializza un vettore di dimensione N con dei dati presi dallo stdin
 *        se N <= 0, o casuali se N > 0.
 *
 * @param N Dimensione del vettore.
 * @return Il vettore di N elementi o NULL in caso di errore.
 */
int *populateDataArray(int N){
    
    char buffer[BUF_LEN];
    int *data = (int *) malloc(sizeof(int) * N);
    int i, flags;
    
    if(data == NULL){
        printf("malloc error!\n");
        return NULL;
    }
        
    if(N <= 20){
        // Leggo da stdin
        //printf("\n\nReading data...\n");
        for(i=0; i < N; i++){
            
            // Lo stdin viene reso non bloccante. Se non e' presente il file da leggere
            // o se non ci sono abbastanza dati, fgets restituisce NULL ed il programma
            // termina.
            flags = fcntl(0, F_GETFL, 0);
            flags |= O_NONBLOCK;
            fcntl(0, F_SETFL, flags);
            
            if(fgets(buffer, BUF_LEN, stdin) != NULL){
                
                data[i] = atof(buffer);
                //printf("%d ", data[i]);   // Solo per il debug di piccole quantita' di dati
                
            } else {
                printf("Error: fgets returned NULL!\n");
                free(data);
                return NULL;
            }
        }
        
    } else {
        // Genero i valori
        //printf("\n\nData generation...\n");
        
        srand(SEED);
        
        for(i=0; i < N; i++){
            data[i] = getRandomInt(200);    // Numero casuale da 1 a 200
            //printf("%d ", data[i]);
        }
    }
    
    printf("\n");
    
    return data;
}

/**
 * @brief Genera un valore casuale compreso tra 1 e maxValue.
 * @param maxValue Limite superiore.
 */
int getRandomInt(int maxValue) {
    return (rand() % maxValue) + 1;   // Random number between 1 and maxValue
}

/**
 * @brief Somma il contenuto dell'array.
 * @param data Vettore il cui contenuto deve essere sommato.
 * @param dim Dimensione del vettore
 */
int getSum(int *data, int dim){
    int i, temp_sum = 0;
    
    if(data != NULL){
        for(i = 0; i < dim; i++){
        temp_sum = temp_sum + data[i];
        }
    }
    return temp_sum;
}

/**
 * @brief Calcola il logaritmo di base 2 di x.
 */
double logb2(int x){
    return log10(x) / log10(2);
}


void printStrategyInfo(int current_processor, int s){
    if(current_processor == 0){
        printf("\nUsing strategy n: %d\n", s);
    }
}

/**
 * @brief Applica la strategia 1 per il calcolo della somma totale.
 * @param current_processor Identificativo del processore corrente,
 * @param tot_processor Numero totale dei processori.
 * @param sum Somma parziale.
 * @return Somma totale.
 */
int strategy1(int current_processor, int tot_processor, int sum){
    
    int i = 0, tag, sum_parz;
    MPI_Status status;
    
    printStrategyInfo(current_processor, 1);
    
    // Il processore 0 riceve le somme parziali
    if(current_processor == 0){
        
        for(i=1; i < tot_processor; i++){
            tag = 80 + i;
            
            MPI_Recv(&sum_parz, 1, MPI_INT, i, tag, MPI_COMM_WORLD, &status);
            
            // Eseguo la somma
            sum = sum + sum_parz;
        }
    }else{
        // Gli altri inviano il valore che hanno calcolato
        tag = 80 + current_processor;
        
        MPI_Send(&sum, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);
    }
    
    return sum;
}

/**
 * @brief Applica la strategia 2 per il calcolo della somma totale.
 * @param current_processor Identificativo del processore corrente,
 * @param tot_processor Numero totale dei processori.
 * @param sum Somma parziale.
 * @return Somma totale.
 */
int strategy2(int current_processor, int tot_processor, int sum){
    
    int i, sum_parz = 0, power, tag;
    long max_step;
    MPI_Status status;
    
    if(tot_processor%2 != 0){
        printf("\nInvalid strategy for %d processor!!! Fallback to strategy 1.\n", tot_processor);
        return strategy1(current_processor, tot_processor, sum);
    }
    
    printStrategyInfo(current_processor, 2);
    
    max_step = (int) logb2(tot_processor); 
    
    for(i=0; i < max_step; i++){
        
        power = (int) pow(2, i);
        
        if((current_processor % power) == 0){
            // Processori che partecipano alla comunicazione
            
            if(current_processor % (int) pow(2, i + 1) == 0){
                
                tag = 100 + current_processor;
                // In lettura
                MPI_Recv(&sum_parz, 1, MPI_INT, current_processor + power, tag, MPI_COMM_WORLD, &status);
                
                sum = sum + sum_parz;
                
            }else{
                tag = 100 - power + current_processor;
                
                // In scrittura
                MPI_Send(&sum, 1, MPI_INT, current_processor - power, tag, MPI_COMM_WORLD);
               
            }
        }
    }
    
    return sum;
}

/**
 * @brief Applica la strategia 3 per il calcolo della somma totale.
 * @param current_processor Identificativo del processore corrente,
 * @param tot_processor Numero totale dei processori.
 * @param sum Somma parziale.
 * @return Somma totale.
 */
int strategy3(int current_processor, int tot_processor, int sum){
    
    int i, sum_parz = 0, power2, power2_1, tag;
    long max_step;
    MPI_Status status;
    
    if(tot_processor%2 != 0){
        printf("\nInvalid strategy for %d processor!!! Fallback to strategy 1.\n", tot_processor);
        return strategy1(current_processor, tot_processor, sum);
    }
    
    printStrategyInfo(current_processor, 3);
    
    max_step = (int) logb2(tot_processor); 
    
    for(i=0; i < max_step; i++){
        // Tutti i processori partecipano alla comunicazione
        
        power2 = (int) pow(2, i);
        power2_1 = (int) pow(2, i + 1);
        
        if(current_processor % power2_1 < power2){
            
            tag = 200 + current_processor;
            
            // Scrittura
            MPI_Send(&sum, 1, MPI_INT, current_processor + power2, tag, MPI_COMM_WORLD);

            // Lettura
            MPI_Recv(&sum_parz, 1, MPI_INT, current_processor + power2, tag, MPI_COMM_WORLD, &status);
            
            sum = sum + sum_parz;
            
        }else{
            tag = 200 - power2 + current_processor;
            
            // Scrittura
            MPI_Send(&sum, 1, MPI_INT, current_processor - power2, tag, MPI_COMM_WORLD);
           
           // Lettura
            MPI_Recv(&sum_parz, 1, MPI_INT, current_processor - power2, tag, MPI_COMM_WORLD, &status);
            
            sum = sum + sum_parz;
        }
        
    }
    
    return sum;
}




