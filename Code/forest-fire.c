/**
* ░▒█▀▀▀░▄▀▀▄░█▀▀▄░█▀▀░█▀▀░▀█▀░░░░█▀▀░░▀░░█▀▀▄░█▀▀░░░▒█▀▀▀█░░▀░░█▀▄▀█░█░▒█░█░░█▀▀▄░▀█▀░░▀░░▄▀▀▄░█▀▀▄
* ░▒█▀▀░░█░░█░█▄▄▀░█▀▀░▀▀▄░░█░░▀▀░█▀░░░█▀░█▄▄▀░█▀▀░░░░▀▀▀▄▄░░█▀░█░▀░█░█░▒█░█░░█▄▄█░░█░░░█▀░█░░█░█░▒█
* ░▒█░░░░░▀▀░░▀░▀▀░▀▀▀░▀▀▀░░▀░░░░░▀░░░▀▀▀░▀░▀▀░▀▀▀░░░▒█▄▄▄█░▀▀▀░▀░░▒▀░░▀▀▀░▀▀░▀░░▀░░▀░░▀▀▀░░▀▀░░▀░░▀
*          _                     ___                
* |_ \/   |_| _ _|_ _  _  . _     _/ . _  _  _   _ .
* |_)/    | || | |_(_)| | |(_)   /__ | /_ /_(_| |  |
* 
* @author Antonio Zizzari
* @date 08/04/2022
* @brief MPI Forest-Fire Simulation 
* @details Il modello è definito come un automa cellulare su una griglia NxN. Una cella può essere vuota,
* occupata da un albero, da un seme di albero, o in fiamme. Il modello è definito dalle seguenti cinque regole 
* che vengono eseguite contemporaneamente:
*     1. Una cella in fiamme si trasforma in una cella vuota;
*     2. Un albero brucerà se almeno un vicino sta bruciando;
*     3. Un seme di albero si trasforma in un albero;
*     4. Un albero si accende con probabilità f=1% anche se nessun vicino sta bruciando;
*     5. Uno spazio vuoto si riempie di un seme d’albero con probabilità p=6%;
* Lo stato di una cella può essere:
*   T (Albero)
*   i (Seme albero)
*   - (vuoto)
*   * (fuoco)
* Materiali usati:
*   Graphics: https://textfancy.com/multiline-text-art/
*   Documentation: 
*       https://www.mpich.org/static/docs/v3.2/
*       https://www.math.tu-cottbus.de/~kd/parallel/mpi/mpi-course.book_2.html
*       https://www.rookiehpc.com/mpi/docs/index.php
*   Course materials: https://tech.io/playgrounds/47058/have-fun-with-mpi-in-c/lets-start-to-have-fun-with-mpi
*
*
*
**/

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "mpi.h"
#include "mycollective.h" //Libreria con tutte le funzioni essenziali

int main(int argc, char **argv) {
    /*Dichiarazione variabili iniziali*/
    int myrank,numtasks;
    int split,plus_i,N,M,style=0,print_mode;
    bool send=false;
    
    /*Probabilità della simulazione*/
    int prob_generate_tree=60;
    int prob_generate_fire=1;
    int prob_generate_seed=6;

    /*Variabili tempo per speed-up*/
    double start, end;
    
    /*Codice MPI essenziale*/
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

    //Nel caso ci interessasse la non esecuzione sequenziale
    /* 
    if(numtasks==1){
        fprintf( stderr, "Numero processori non valido!!!\n");
        MPI_Abort(MPI_COMM_WORLD,1);
    }
    */

    //CODICE PER MOSTRARE CHE OLTRE I 16.000.000 ELEMENTI CRASHA IL CODICE
    //char test[16000001];
    //test[16000000]='1';

    /*Prendo in inut le variabili essenziali*/
    if (myrank == 0)
    {
        print_menu();
        printf("Wanna run the <print-mode> (type 0 'NO', or 1 'YES')?:\n");
        scanf("%d",&print_mode);  
        if(print_mode){
            printf("Choose the program style (avialbe: 0, 1, 2):\n");
            scanf("%d",&style); 
        }
        printf("Give the matrix size (NxM), give N:\n");
        scanf("%d",&N);  
        printf("Give the matrix size (NxM), give M:\n");
        scanf("%d",&M); 
         

        printf("\n");              
    }

    start = MPI_Wtime();/*Inizio a segnare il tempo*/

    /*Seed del numero random*/
    srand((myrank+1)*10);

    /*Broadcast delle variabili essenziali*/
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&M, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&style, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&print_mode, 1, MPI_INT, 0, MPI_COMM_WORLD);

    /*Calcolo come suddividere il carico tra i processori.*/
    split_domain(numtasks,N,&split,&plus_i);
    int rank_split=((myrank<plus_i) ? (split+1) : (split));

    /*Generazione della foresta delegata al rank 0*/
    char *splitted_forest=malloc((rank_split*M)*sizeof(char));
    //char splitted_forest[rank_split*M];
    

    /*Popolo il bosco*/
    for(int i = 0; i < rank_split; i++)
    {
        for(int j = 0; j < M; j++)
        {
            if(rand()%100<=prob_generate_tree)/* Probabilità che nasca un albero */
                splitted_forest[i * M + j]='T';
            else
                splitted_forest[i * M + j]='-';
        }
    }

    /***FORMATTO SCATTERV E GATHER***/
    int send_counts[numtasks];
    int send_displacements[numtasks];
    if(print_mode){
        //char forest[N*M];
        char *forest=malloc((N*M)*sizeof(char));
        
        /*Porzione di array che riceverò dalla scatterv*/
        /*da quanti elementi è formato l'array che devo mandare ad ogni processore*/        
        for(int i=0;i<numtasks;i++) send_counts[i]=(i<plus_i) ? M*(split+1) : M*(split);

        /*da quale index parto ad ogni processore*/        
        send_displacements[0]=0;
        for(int i=1;i<numtasks;i++) 
            send_displacements[i]=(i<=plus_i) ? send_displacements[i-1]+(M*(split+1)) : send_displacements[i-1]+(M*split);

        
        /*Ricevo i dati per stamparli*/
        MPI_Gatherv(splitted_forest, (myrank<plus_i) ? M*(split+1) : M*(split)/*Quanti elementi invio*/, MPI_CHAR, forest, send_counts, send_displacements, MPI_CHAR, 0, MPI_COMM_WORLD);
        /*Stampo la foresta al Day 0*/
        if(myrank==0){
            printf("Day 0\n");print_forest(forest,N,M,style,split,plus_i);printf("\n\n\n");
        }
        
        free(forest);/*Libero spazio*/
    }
    /*Variabili essenziali per il while*/
    int i=0;
    //char temp_forest[rank_split*M];/*array della tecnica double buffer che conterrà i risultati*/
    char *temp_forest=malloc((rank_split*M)*sizeof(char));;/*array della tecnica double buffer che conterrà i risultati*/
    while(1)
    {   
        //MPI_Scatterv(forest, send_counts, send_displacements, MPI_CHAR, splitted_forest , (myrank<plus_i) ? M*(split+1) : M*(split)/*Quanti elementi ricevo*/, MPI_CHAR, 0, MPI_COMM_WORLD);
        
        //****CODICE SIMULAZIONE
        //Variabili essenziali
        MPI_Request request_send1=MPI_REQUEST_NULL,request_recv1=MPI_REQUEST_NULL,request_send2=MPI_REQUEST_NULL,request_recv2=MPI_REQUEST_NULL;  
        MPI_Status status1,status2,status3,status4;        
        memset(temp_forest,'-',rank_split*M);/*Setto tutti i valori di temp_forest a "-" */
        int count1,count2;
        
        //int bot_send[M],bot_recv[M];
        //int top_send[M],top_recv[M];

        int *bot_send=malloc(M*sizeof(int));
        int *bot_recv=malloc(M*sizeof(int));
        int *top_send=malloc(M*sizeof(int));
        int *top_recv=malloc(M*sizeof(int));

        if(myrank==0)
        {
            //int bot_send[M],bot_recv[M];
            //int top_send[M];/*non ci serve per il calcolo*/
            
            int top,bot;
            

            if(numtasks!=1){/*Esecuzione parallela*/
                /*Aspetto edge da rank 1 */
                MPI_Irecv(bot_recv, M, MPI_INT, 1, i+1, MPI_COMM_WORLD, &request_recv1);
                
                /*Il fuoco si propaga e ritorno se ci sono ai bordi dei fuochi*/
                fire_run_return_set(splitted_forest,temp_forest,rank_split,M,top_send,bot_send,&top,&bot);
                
                MPI_Isend(bot_send, bot, MPI_INT, 1, i, MPI_COMM_WORLD, &request_send1); /*Mando array a rank 1*/
              

                
            }else{/*Esecuzione sequenziale*/
                fire_run(splitted_forest,temp_forest,rank_split,M);/*Il fuoco si propaga*/
            }
                

            if(numtasks!=1){/*Esecuzione parallela*/
                MPI_Wait(&request_send1, &status1);
                MPI_Wait(&request_recv1, &status2); 
                MPI_Get_count(&status2, MPI_INT, &count1);
                if(count1>0)
                    fire_run_bot_edge_pointer(bot_recv,count1,temp_forest,M,rank_split);/*fuoco sul bordo*/


            }else{/*Esecuzione sequenziale*/
                ;
            }            

            forest_run(rank_split,M,temp_forest,prob_generate_fire,prob_generate_seed);/*si accende il fuoco & nasce un seed & cresce un albero*/

            
            
        }
        else if(myrank==numtasks-1)
        {
            int top,bot;        

            MPI_Irecv(top_recv, M, MPI_INT, myrank-1, i, MPI_COMM_WORLD, &request_recv1);
            fire_run_return_set(splitted_forest,temp_forest,rank_split,M,top_send,bot_send,&top,&bot);/*Il fuoco si propaga*/
            
            MPI_Isend(top_send, top, MPI_INT, myrank-1, i+1, MPI_COMM_WORLD, &request_send1);

            
            MPI_Wait(&request_send1, &status1);
            MPI_Wait(&request_recv1, &status2); 
            
            MPI_Get_count(&status2, MPI_INT, &count2);
            if(count2>0)
                fire_run_top_edge_pointer(top_recv,count2,temp_forest,M);            

            forest_run(rank_split,M,temp_forest,prob_generate_fire,prob_generate_seed);
        }
        else
        {
            
            int top,bot;

            MPI_Irecv(bot_recv, M, MPI_INT, myrank+1, i+1, MPI_COMM_WORLD, &request_recv2); /*recv bot from top*/
            MPI_Irecv(top_recv, M, MPI_INT, myrank-1, i, MPI_COMM_WORLD, &request_recv1);

            fire_run_return_set(splitted_forest,temp_forest,rank_split,M,top_send,bot_send,&top,&bot);/*Il fuoco si propaga*/
            /*send dell'array top & bot*/
            MPI_Isend(top_send, top, MPI_INT, myrank-1, i+1, MPI_COMM_WORLD, &request_send1); /*send top to bot^*/     
            MPI_Isend(bot_send, bot, MPI_INT, myrank+1, i, MPI_COMM_WORLD, &request_send2); 
            
            MPI_Wait(&request_send1, &status1);
            MPI_Wait(&request_recv2, &status2);

            MPI_Get_count(&status2, MPI_INT, &count1);
            if(count1>0)
                fire_run_bot_edge_pointer(bot_recv,count1,temp_forest,M,rank_split);

            MPI_Wait(&request_send2, &status3);
            MPI_Wait(&request_recv1, &status4); 

            MPI_Get_count(&status4, MPI_INT, &count2);
            if(count2>0)
                fire_run_top_edge_pointer(top_recv,count2,temp_forest,M);            

            forest_run(rank_split,M,temp_forest,prob_generate_fire,prob_generate_seed);
        }    
        
        if(print_mode){
            //char forest[N*M];
            char *forest=malloc((N*M)*sizeof(char));
            /*Ricevo i dati per stamparli*/
            MPI_Gatherv(temp_forest, (myrank<plus_i) ? M*(split+1) : M*(split)/*Quanti elementi invio*/, MPI_CHAR, forest, send_counts, send_displacements, MPI_CHAR, 0, MPI_COMM_WORLD);
            if(myrank==0){
                printf("Day %d\n",i+1);print_forest(forest,N,M,style,split,plus_i);printf("\n\n");
            }
            free(forest);
        }
        /****************************/
        strcpy(splitted_forest,temp_forest);/*aggiorno i dati sull'array*/

        free(bot_send);
        free(bot_recv);
        free(top_send);
        free(top_recv);

        i++;
        if(i>20) break;

        
    }
    
    /*Fine codice*/
    MPI_Barrier(MPI_COMM_WORLD); 
    end = MPI_Wtime();

    free(splitted_forest);
    free(temp_forest);

    MPI_Finalize();
    if (myrank == 0) { /* Master node scrive su stdout il tempo*/
    printf("Time in ms = %f\n", end-start);
    }

    
    return 0;
}