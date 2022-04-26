
# MPI - Forest-fire simulation
Il **forest-fire model** è stato creato utilizzando MPI durante il corso di *PROGRAMMAZIONE CONCORRENTE PARALLELA E SU CLOUD* all'università degli studi di Salerno
## Indice
* [Introduzione](#introduzione)
* [Breve descrizione della soluzione](#breve-descrizione-della-soluzione)
* [Istruzioni per l'esecuzione](#istruzioni-per-lesecuzione)
* [Prova della correttezza](#prova-della-correttezza)
* [Benchmarks](#benchmarks)

## Introduzione

Il modello è definito come un automa cellulare su una griglia NxM. Una cella può essere vuota, occupata da un albero, da un seme di albero, o in fiamme. Il modello è definito dalle seguenti sei regole che vengono eseguite contemporaneamente:
 1. Una cella in fiamme si trasforma in una cella vuota;
 2. Un albero brucerà se almeno un vicino sta bruciando;
 3. Un seme di albero si trasforma in un albero;
 4. Un albero si accende con probabilità *f=1%* anche se nessun vicino sta bruciando;
 5. Uno spazio vuoto si accende con probabilità *f*;
 6. Uno spazio vuoto si riempie di un seme d'albero con probabilità *p=6%*;

Lo stato di una cella può essere:
|Emoji|ASCII|Stato|
|----------------|---------------|-----------------|
|🌳|T|Albero|
|🌱|i |Seme albero|
||-|Vuoto|
|🔥|*|Fuoco|

La simulazione è eseguita un numero fissato di volte. Ad ogni fase di simulazione, i processori eseguono le 5 regole precedentemente descritte, e comunicano con i processori vicini per completare il calcolo.

## Breve descrizione della soluzione

Il codice prende in input dall'utente:
1. Se eseguire le stampe;
2. la grandezza della matrice;

Tutti i **processori** generano una porzione di matrice in base alla taglia che gli è stata assegnata, successivamente continuano la propria simulazione indipendentemente, fino a quando, se necessario completeranno il calcolo chiedendo ai worker vicini i dati necessari altrimenti no. Una volta terminata la simulazione sui singoli **workers**, se è stato chiesto di stampare l'andamento della simulazione, il processore **con rank 0** stampa a video lo stato di essa.

## Istruzioni per l'esecuzione

### Pre-requisiti
+ Ubuntu 18.04 LTS (*min: 10 GB storage*);
+ Set-up dell'ambiente seguendo`https://github.com/spagnuolocarmine/ubuntu-openmpi-openmp`;
### Esecuzione locale
+ Avere nella stella directory i file:`mycollection.h` e `forest-fire.c`;
+ Eseguire il comando `mpicc forest-fire.c -o forest` per la compilazione;
+  Eseguire `mpirun --allow-run-as-root -np 4 forest` (4 è un numero di processori indicativo, si può assegnare il valore più consono alla propria macchina).
### Esecuzione distribuita
+ Avere *n* macchine che condividono lo stesso [set-up](#pre-requisiti) stabilito nei pre-requisiti;
+ Ogni macchina deve avere nella stessa directory `hfile`, `mycollection.h` e `forest-fire.c`;
+ Eseguire il comando ``mpicc forest-fire.c -o forest``per la compilazione;
+ Eseguire sulla macchina **master** `mpirun --allow-run-as-root -np 2 --hostfile hfile ./forest`.

### Run-time
Allo start-up del programma viene chiesto all'utente la modalità di stampa (Eseguire la <print-mode\> influisce molto negativamente sulle performance dovuto a 2 cause: *printf()* e *MPI_Gatherv()* )
+ 0 la soluzione viene mostrata con i caratteri ASCII;
+ 1 la soluzione viene mostrata con le emoji;
+ 2 la soluzione viene mostrata in una modalità *"debug"* mostrando visivamente dove comunicano i processori tra loro.

![](https://i.imgur.com/JLG5EW6.png?1)

Successivamente viene chiesta la grandezza della matrice sul quale si andrà a lavorare.

![](https://i.imgur.com/hcGC7NY.png)

Infine si otterrà l'esecuzione desiderata con le relative stampe.


## Prova della correttezza
Ad inizio programma, dopo che son state passate le variabili principali via linea di comando, il programma andrà a calcolare quanta porzione di array dovrà avere ogni processore **worker**, esso è definito da questa funzione:
```c
//Suddivisione dominio per righe
void split_domain(int num_p,int domain, int *split,int *plus_i){
    if(num_p>domain){
        fprintf( stderr, "N matrice non compatibile con numero processori\n");
        MPI_Abort(MPI_COMM_WORLD,1);
    }

    *split = domain/num_p;
    *plus_i =  domain%num_p;//Numero di elementi che avranno un +1 sulla riga
}
```
Dopo aver ottenuto il numero di righe da assegnare ad ogni processore, in modo indipendente il worker genera una porzione della foresta nel seguente modo:
```c
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
```
Quel che abbiamo ottenuto è un array di alberi di questo tipo:
```
Day 0
+-----+-----+-----+-----+-----+-----+-----+-----+
|  -  |  T  |  -  |  T  |  T  |  -  |  -  |  -  |
+-----+-----+-----+-----+-----+-----+-----+-----+
|  T  |  T  |  T  |  -  |  T  |  -  |  T  |  -  |
+-----+-----+-----+-----+-----+-----+-----+-----+
|  T  |  T  |  -  |  -  |  -  |  T  |  T  |  -  |
+-----+-----+-----+-----+-----+-----+-----+-----+
|  -  |  T  |  T  |  T  |  -  |  -  |  T  |  -  |
+-----+-----+-----+-----+-----+-----+-----+-----+
|  T  |  -  |  T  |  T  |  -  |  T  |  -  |  T  |
+-----+-----+-----+-----+-----+-----+-----+-----+
|  T  |  -  |  -  |  T  |  T  |  -  |  T  |  T  |
+-----+-----+-----+-----+-----+-----+-----+-----+
|  T  |  T  |  T  |  -  |  T  |  -  |  T  |  T  |
+-----+-----+-----+-----+-----+-----+-----+-----+
|  -  |  T  |  T  |  -  |  T  |  -  |  T  |  T  |
+-----+-----+-----+-----+-----+-----+-----+-----+
```
Successivamente, viene lanciata la funzione `fire_run_return_set()` che ha il compito di propagare il fuoco alle celle vicine, ma dato che questa foresta è ancora "intatta" cioè non ci sono fuochi, viene  lanciata la funzione `forest_run()`:
```c
//si accende il fuoco & nasce un seed & cresce un albero
void forest_run(int rank_split,int N,char forest[],int prob_generate_fire,int prob_generate_seed){
    
    for(int i = 0; i < rank_split; i++)
    {
        for(int j = 0; j < N; j++)
        {
            switch (forest[i * N + j])
            {
                case 'T':
                    if(rand()%100<=prob_generate_fire)/* Probabilità che un albero prenda fuoco */
                        forest[i * N + j]='*';
                    break;
                case 'i':
                    forest[i * N + j]='T'; /* L'albero cresce*/
                    break;
                case '-':
                    if(rand()%100<=prob_generate_fire)/* Probabilità prenda fuoco la casella vuota */
                        forest[i * N + j]='*';
                    else if(rand()%100<=prob_generate_fire+prob_generate_seed)/* Probabilità che nasca un nuovo albero */
                        forest[i * N + j]='i';
                    break;
            }
        }
    }
    
}
```
Essa farà in modo che: in accordo con le probabilità [assegnate](#introduzione) nascerà un seme d'albero, un seme diventa un albero, e uno spazio vuoto o un albero può prender fuoco. L'output ci genererà la seguente matrice:
```
Day 1
+-----+-----+-----+-----+-----+-----+-----+-----+
|  -  |  T  |  -  |  T  |  T  |  -  |  -  |  -  |
+-----+-----+-----+-----+-----+-----+-----+-----+
|  T  |  T  |  T  |  i  |  T  |  -  |  T  |  -  |
+-----+-----+-----+-----+-----+-----+-----+-----+
|  T  |  T  |  -  |  -  |  *  |  T  |  T  |  -  |
+-----+-----+-----+-----+-----+-----+-----+-----+
|  -  |  T  |  T  |  T  |  -  |  -  |  T  |  -  |
+-----+-----+-----+-----+-----+-----+-----+-----+ 
|  T  |  -  |  T  |  T  |  -  |  T  |  -  |  T  |
+-----+-----+-----+-----+-----+-----+-----+-----+
|  T  |  -  |  -  |  T  |  T  |  *  |  T  |  T  |
+-----+-----+-----+-----+-----+-----+-----+-----+
|  T  |  T  |  T  |  -  |  T  |  -  |  T  |  T  |
+-----+-----+-----+-----+-----+-----+-----+-----+
|  -  |  T  |  T  |  -  |  T  |  -  |  T  |  T  |
+-----+-----+-----+-----+-----+-----+-----+-----+
```
Come è possibile notare sono nati dei fuochi, e un seme di albero.
Al  prossimo ciclo di esecuzione il fuoco verrà propagato attraverso la funzione precedentemente citata `fire_run_return_set()`, inserendo il risultato in una nuova matrice:
```c
//Funzione che applica il fuoco sulla foresta e ritorna se ci sono fuochi
// 3 entrambi, 2 bot, 1 top, 0 nulla
void fire_run_return_set(char forest0[],char forest1[],int row,int col,int top_edge[],int bot_edge[],int *top,int *bot){

    *top=0;
    *bot=0;
    for(int i=0;i<row;i++)
    {
        for(int j=0;j<col;j++)
        {
            switch (forest0[i * col + j])
                {
                    case 'T':/*Se questo albero è stato bruciato da un vicino lo rimango tale altrimenti no*/
                        if(forest1[i * col + j]=='*')
                            ;
                        else
                            forest1[i * col + j]='T';
                            
                        break;

                    case '*':          
                        /*Imposto l'array da ritornare*/              
                        if(i==0){ top_edge[*top]=j;*top=*top+1;} 
                        if(i==row-1){ bot_edge[*bot]=j;*bot=*bot+1;} 

                        /* [x][y]*/                      
                        /*[-1][-1] [-1][0] [-1][+1] [0][-1] [0][0] [0][+1] [+1][-1] [+1][0] [+1][+1]*/
                        forest1[i * col + j]='-';/*Spengo il fuoco che ha bruciato i dintorni*/
                        for(int x = -1; x <=1; x++)/*Brucio nei dintorni gli alberi*/
                        {
                            for(int y = -1; y <= 1; y++)
                            {
                                if( (((i+x)>=0) && ((i+x)<row)) && (((j+y)>=0) && ((j+y)<col)) ){
                                    if(forest0[(i+x) * col + (j+y)]=='T')/*Gli alberi prendono fuoco*/
                                        forest1[(i+x) * col + (j+y)]='*';
                                }
                            }
                        }
                        break;
                    case 'i':
                        forest1[i * col + j]='i';/*La natura vince sugli umani che sono malvagi e un seed sopravvive sempre*/
                        break;
                    case '-':
                        forest1[i * col + j]='-';/*Vuoto ero e vuoto rimango*/
                        break;
                }
        }
    }
}
```
a questo punto viene lanciata `forest_run()`, e ciò che otterremo sarà la seguente matrice:
```
Day 2
+-----+-----+-----+-----+-----+-----+-----+-----+
|  -  |  T  |  -  |  T  |  T  |  *  |  -  |  -  |
+-----+-----+-----+-----+-----+-----+-----+-----+
|  *  |  T  |  T  |  -  |  *  |  -  |  T  |  -  |
+-----+-----+-----+-----+-----+-----+-----+-----+
|  T  |  T  |  -  |  -  |  -  |  *  |  T  |  -  |
+-----+-----+-----+-----+-----+-----+-----+-----+
|  -  |  T  |  T  |  *  |  *  |  -  |  T  |  -  |
+-----+-----+-----+-----+-----+-----+-----+-----+
|  T  |  -  |  T  |  T  |  -  |  *  |  i  |  T  |
+-----+-----+-----+-----+-----+-----+-----+-----+
|  T  |  -  |  -  |  T  |  *  |  -  |  *  |  T  |
+-----+-----+-----+-----+-----+-----+-----+-----+
|  T  |  T  |  T  |  *  |  *  |  i  |  *  |  T  |
+-----+-----+-----+-----+-----+-----+-----+-----+
|  -  |  T  |  *  |  -  |  T  |  -  |  T  |  T  |
+-----+-----+-----+-----+-----+-----+-----+-----+
```
Essa contiene sia il fuoco propagato, che la generazione dei nuovi elementi.
Ricapitolando in successione l'output ottenuto sarà il seguente(Questa volta con una grafica migliore):

![](https://i.imgur.com/dWMkdMp.png)

Dovendo favorire il calcolo su più processori la soluzione proposta è eseguita su più processori che comunicano in modo intelligente.
2 o più processori durante la loro esecuzione condividono delle *"criticità"* dovute al fatto che per completare la loro simulazione hanno bisogno di comunicare con i bordi superiori ed inferiori.
A tal proposito durante l'esecuzione della funzione `fire_run_return_set()` vengono segnati quanti elementi sono presenti al bordo superiore e al bordo inferiore, definiti con il nome di `top` e `bot`, ed in contemporanea vengono salvati in un array l'indice di ogni cella che contiene fuoco.
Sfruttando i dati precedentemente ottenuti, quel che si conosce adesso è se sul bordo ci sono fuochi o meno, e se ci sono a quale indice alloggiano, e si comunica tra processori in modo intelligente lanciando *send* *dinamiche*  
```c
MPI_Isend(top_send, top, MPI_INT, myrank-1, i+1, MPI_COMM_WORLD, &request_send1); 
MPI_Isend(bot_send, bot, MPI_INT, myrank+1, i, MPI_COMM_WORLD, &request_send2);
```
Il processore che riceverà i dati andrà a controllare se ha ricevuto elementi nel seguente modo:
```c
MPI_Get_count(&status2, MPI_INT, &count1);
if(count1>0)
	fire_run_bot_edge_pointer(bot_recv,count1,temp_forest,M,rank_split);

MPI_Get_count(&status4, MPI_INT, &count2);
if(count2>0)
	fire_run_top_edge_pointer(top_recv,count2,temp_forest,M);
```
Se ha ricevuto qualcosa allora semplicemente farà la propagazione del fuoco ai bordi nel seguente modo:
```c
//Accendo il fuoco sul bordo superiore
void fire_run_top_edge_pointer(int from[],int N_top,char to[],int N){
    for(int i=0;i<N_top;i++)
    {
        for(int x=-1;x<=1;x++)
        {
            if((from[i]+x)>=0 && (from[i]+x)<N)
            {
                if(to[from[i]+x]=='T')/*Gli alberi prendono fuoco*/
                    to[from[i]+x]='*';
            }
        }
    }
}

//Accendo il fuoco sul bordo inferiore
void fire_run_bot_edge_pointer(int from[],int N_bot,char to[],int N,int split){
    for(int i=0;i<N_bot;i++)
    {
        for(int x=-1;x<=1;x++){
            if((N*(split-1)+from[i]+x)>=(N*(split-1)) && (N*(split-1)+from[i]+x)<N*split){
                if(to[N*(split-1)+from[i]+x]=='T')/*Gli alberi prendono fuoco*/
                    to[N*(split-1)+from[i]+x]='*';
            }
        }
    }
}
```
Il tutto è iterato un numero di volte finito. Al termine di esso viene stampato il tempo impiegato dall'algoritmo.
Fine.
## Benchmarks
I benchmarks sono stati lanciati su **Google Cloud Platform**, utilizzando 6 macchine **e2-standard-4** aventi ognuno 4 vCPUs.
Con un totale di 24 vCPUs i risultati ottenuti sono:
#### Strong scalability
L'algoritmo è stato eseguito su una matrice 10.000x10.000

|vCPUs|Tempo|Speed-up|
|-|-|-|
|1|32.928257|-|
|2|16.311782|2,019|
|3|15.983904|2,06|
|4|12.809180|2,571|
|5|9.636696|3,417|
|6|8.069048|4,081|
|7|6.912726|4,763|
|8|6.072358|5,423|
|9|5.734646|5,742|
|10|5.303405|6,209|
|11|4.695636|7,013|
|12|4.288977|7,677|
|13|3.737966|8,809|
|14|3.496114|9,419|
|15|3.240169|10,16|
|16|3.182754|10,35|
|17|2.875140|11,45|
|18|2.734069|12,04|
|19|2.623592|12,55|
|20|2.527962|13,03|
|21|2.372710|13,88|
|22|2.235581|14,73|
|23|2.173712|15,15|
|24|2.057124|16,00694|

Come è possibile notare a 24 vCPU si ottiene uno Speed-up di ben 16, sebbene il risultato non è super notevole rimane un ottimo risultato.

#### Weak scalability
L'algoritmo è stato eseguito su una matrice **np**x10.000.000

|vCPUs|Tempo|
|-|-|
|1|2.868079|
|2|3.003850|
|3|4.720806|
|4|4.882781|
|5|4.995334|
|6|4.997530|
|7|5.064155|
|8|5.067452|
|9|5.083282|
|10|5.059520|
|11|5.104069|
|12|5.449976|
|13|5.156495|
|14|5.126553|
|15|5.201446|
|16|5.699584|
|17|5.142877|
|18|5.490366|
|19|5.171884|
|20|5.138652|
|21|5.379677|
|22|5.127093|
|23|5.197929|
|24|5.142240|

E' stato possibile dimostrare che dando lo stesso numero di elementi a n processori il tempo di esecuzione è pressoché identico.

