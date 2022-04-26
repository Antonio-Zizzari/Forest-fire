/********************************************************************
UTILS
*********************************************************************/
//Suddivisione dominio per righe
void split_domain(int num_p,int domain, int *split,int *plus_i){
    if(num_p>domain){
        fprintf( stderr, "N matrice non compatibile con numero processori\n");
        MPI_Abort(MPI_COMM_WORLD,1);
    }

    *split = domain/num_p;
    *plus_i =  domain%num_p;//Numero di elementi che avranno un +1 sulla riga
}

//Funzione che applica il fuoco sulla foresta e lo fa runnare
void fire_run(char forest0[],char forest1[],int row,int col){

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
                        forest1[i * col + j]='i';/*La natura vince sugli umani che sono malvagi e un seed sopravvive sempre
                        break;*/
                    case '-':
                        forest1[i * col + j]='-';/*Vuoto ero e vuoto rimango*/
                        break;
                }
        }
    }

}

//Accendo il fuoco sul bordo superiore
void fire_run_top_edge(char from[],char to[],int N){
    for(int i=0;i<N;i++)
    {
        if(from[i]=='*')
        {
            for(int x=-1;x<=1;x++)
            {
                if((i+x)>=0 && (i+x)<N)
                {
                    if(to[i+x]=='T')/*Gli alberi prendono fuoco*/
                        to[i+x]='*';
                }
            }
        }
    }
}

//Accendo il fuoco sul bordo inferiore
void fire_run_bot_edge(char from[],char to[],int N,int split){
    for(int i=0;i<N;i++)
    {
        if(from[i]=='*')
            for(int x=-1;x<=1;x++)
                if((N*(split-1)+i+x)>=(N*(split-1)) && (N*(split-1)+i+x)<N*split)
                    if(to[N*(split-1)+i+x]=='T')/*Gli alberi prendono fuoco*/
                        to[N*(split-1)+i+x]='*';
    }
}
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

//Setto array top da inviare
void edge_top_send_array(char from[],char to[],int N){
    for(int i=0;i<N;i++)
    {
        to[i]=from[i];
    }

}
//Setto array bot da inviare
void edge_bot_send_array(char from[],char to[],int N,int split){
    for(int i=0;i<N;i++)
    {
        to[i]=from[(N*(split-1))+i];
    }
}

//Controlla se ci sono fuochi ai bordi
// 3 entrambi, 2 bot, 1 top, 0 nulla
int if_send_pack(char forest[],int row,int col){

    int top=0,bot=0;

    for(int i=0;i<row;i++)
    {
        if(forest[i]=='*') top++;
    }
    for(int i=0;i<row;i++)
    {
        if(forest[(row-1) * col + i]=='*') bot++;
    }

    if(top && bot){
        return 3;
    }else if(bot && !(top)){
        return 2;
    }else if(top && !(bot)){
        return 1;
    }else{
        return 0;
    }
}


//Funzione che applica il fuoco sulla foresta e ritorna se ci sono fuochi
// 3 entrambi, 2 bot, 1 top, 0 nulla
int fire_run_return(char forest0[],char forest1[],int row,int col){

    int top=0,bot=0;
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
                        if(i==0) top++;
                        else if(i==row-1) bot++;
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
    if(top && bot){
        return 3;
    }else if(bot && !(top)){
        return 2;
    }else if(top && !(bot)){
        return 1;
    }else{
        return 0;
    }

}

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
/********************************************************************
STAMPA
*********************************************************************/
void print_menu(){


    printf("\n\n");
    printf("\033[0;32m░▒█▀▀▀░▄▀▀▄░█▀▀▄░█▀▀░█▀▀░▀█▀░░░░█▀▀░░▀░░█▀▀▄░█▀▀░░░▒█▀▀▀█░░▀░░█▀▄▀█░█░▒█░█░░█▀▀▄░▀█▀░░▀░░▄▀▀▄░█▀▀▄\n");
    printf("░▒█▀▀░░█░░█░█▄▄▀░█▀▀░▀▀▄░░█░░▀▀░█▀░░░█▀░█▄▄▀░█▀▀░░░░▀▀▀▄▄░░█▀░█░▀░█░█░▒█░█░░█▄▄█░░█░░░█▀░█░░█░█░▒█\n");
    printf("░▒█░░░░░▀▀░░▀░▀▀░▀▀▀░▀▀▀░░▀░░░░░▀░░░▀▀▀░▀░▀▀░▀▀▀░░░▒█▄▄▄█░▀▀▀░▀░░▒▀░░▀▀▀░▀▀░▀░░▀░░▀░░▀▀▀░░▀▀░░▀░░▀\033[0m\n");
    printf("\033[0;36m         _                     ___                \n");
    printf("|_ \\/   |_| _ _|_ _  _  . _     _/ . _  _  _   _ .\n");
    printf("|_)/    | || | |_(_)| | |(_)   /__ | /_ /_(_| |  |\033[0m\n\n");
    fflush(stdout);
}

void help_print(int N){
    printf("+-----+");
    for(int i = 0; i < N-1; i++)
    {    
        printf("-----+");
    }
    printf("\n");
}

void help_print_split(int N){
    printf("\033[0;35m+-----+");
    for(int i = 0; i < N-1; i++)
    {    
        printf("-----+");
    }
    printf("\033[0m\n");
}
//Stampo la foresta
void print_forest(char forest[],int N,int M,int style,int split,int plus_i){
    help_print(M);
    for(int i = 0; i < N; i++)
    {
        for(int j = 0; j < M; j++)
        {   
            if(style==0){
                if(forest[i * M + j]=='T') printf("|  \033[0;32m%c\033[0m  ", forest[i * M + j]);
                else if(forest[i * M + j]=='*') printf("|  \033[1;31m%c\033[0m  ", forest[i * M + j]);
                else if(forest[i * M + j]=='i') printf("|  \033[0;33m%c\033[0m  ", forest[i * M + j]);
                else printf("|  %c  ", forest[i * M + j]);
            }
            else{
                if(forest[i * M + j]=='T') printf("| 🌳  ");
                else if(forest[i * M + j]=='*') printf("| 🔥  ");
                else if(forest[i * M + j]=='i') printf("| 🌱  ");
                else printf("|  -  ");
            }            
        }
        printf("|\n");
        if(style==2)
        {
            if(plus_i>0)
            {
                if((i+1)%(split+1)==0)
                {
                    plus_i--;
                    help_print_split(M);
                }else help_print(M);
            }
            else if((i+1)%split==0 && i<N-1) help_print_split(M);
            else help_print(M);
        }               
        else
            help_print(M);

    }
}