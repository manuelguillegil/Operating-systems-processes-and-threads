#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <pthread.h> 
#include <string.h>

#define TRUE 1
#define FALSE 0

int fd[2];                                                                            //Pipe
int day=1;                                                                            //Inicializa la variable day que cuenta en que dia esta
int daysMax;                                                                          //int que indica cual es el dia maximo 
pthread_mutex_t mutex;                                                                //Inicializa Mutex que se usa para el pipe entre hilos y prensa
pthread_t thread_id_ejec;                                                             //Id de Ejecutivo
pthread_t thread_id_legis;                                                            //Id de Legislativo                                                            
pthread_t thread_id_jud;                                                              //Id de Judicial

void *threadEjec(void *vargp) 
{ 
    size_t len = 0;
    char* line;
    int Encontro = FALSE;                                                             //Usado para ver cuando se encontro una accion
    char toPrensa[1000];                                                              //Mensaje que se envia a la prensa
    char* Decision;                                                                   //Se usa para revisar que que decision en la accion es
    srand(time(NULL));                                                                   
    FILE* fp = fopen("Ejecutivo.acc", "r");

    //Encuentra una accion, con 20% de probabilidad
    while(TRUE){
        while(getline(&line, &len, fp) != -1){
            if(strlen(line) > 2 && strchr(line, ':') == NULL && rand()%5 == 1){       //Mientras la linea no sea un espacio, no contenga el caracter :
                Encontro=TRUE;                                                        //y con una posibilidad de 20% de ser escogido
                break;                                                                  
            }
        }
        if(!Encontro){
            rewind(fp);
        }
        else{
            break;
        }
    }
    //Ahora ejecuta la accion
    while(getline(&line, &len, fp)!=-1 && strlen(line)>2){
        Decision = strtok(line," ");
        if(strstr(Decision, "aprobacion")){

        }
        else if(strstr(Decision, "inclusivo")){

        }
        else if(strstr(Decision, "exclusivo")){

        }
        else if(strstr(Decision, "exito") && rand()%2 == 1){                          //Si la accion es exitosa
            break;
        }
        else if(strstr(Decision, "fracaso") && rand()%2 == 1){                        //Si la accion fracaza
            break;
        }
    }
    Decision = strtok(NULL,"\0");                                                     //Toma el resto de el mensaje    
    pthread_mutex_lock(&mutex);                                                       //Para evitar inconsistencia en el pipe, se debe tratar como Seccion Critica
    strcpy(toPrensa, Decision);
    write(fd[1], toPrensa, sizeof(toPrensa));                                         //Se escribe la el mensaje de exito/fracaso al pipe que conecta este hilo con la prensa
    pthread_mutex_unlock(&mutex);                                                     //Se libera el mutex

    fclose(fp);
    pthread_exit(NULL);
}

void *threadLegis(void *vargp) 
{ 
    size_t len = 0;
    char* line;
    int Encontro = FALSE;                                                             //Usado para ver cuando se encontro una accion
    char toPrensa[1000];                                                              //Mensaje que se envia a la prensa
    char* Decision;                                                                   //Se usa para revisar que que decision en la accion es
    srand(time(NULL));
    FILE* fp = fopen("Legislativo.acc", "r");

    //Encuentra una accion, con 20% de probabilidad
    while(TRUE){
        while(getline(&line, &len, fp) != -1){
            if(strlen(line) > 2 && strchr(line, ':') == NULL && rand()%5 == 1){       //Mientras la linea no sea un espacio, no contenga el caracter :
                Encontro=TRUE;                                                        //y con una posibilidad de 20% de ser escogido
                break;                                                                  
            }
        }
        if(!Encontro){
            rewind(fp);
        }
        else{
            break;
        }
    }
    //Ahora ejecuta la accion
    while(getline(&line, &len, fp)!=-1 && strlen(line)>2){
        Decision = strtok(line," ");
        if(strstr(Decision, "aprobacion")){

        }
        else if(strstr(Decision, "inclusivo")){

        }
        else if(strstr(Decision, "exclusivo")){ 

        }
        else if(strstr(Decision, "exito") && rand()%2 == 1){                          //Si la accion es exitosa
            break;
        }
        else if(strstr(Decision, "fracaso") && rand()%2 == 1){                        //Si la accion fracaza
            break;
        }
    }
    Decision = strtok(NULL,"\0");                                                     //Toma el resto de el mensaje                          
    pthread_mutex_lock(&mutex);                                                       //Para evitar inconsistencia en el pipe, se debe tratar como Seccion Critica
    strcpy(toPrensa, Decision);
    write(fd[1], toPrensa, sizeof(toPrensa));                                         //Se escribe la el mensaje de exito/fracaso al pipe que conecta este hilo con la prensa
    pthread_mutex_unlock(&mutex);                                                     //Se libera el mutex

    fclose(fp);
    pthread_exit(NULL);
}

void *threadJud(void *vargp) 
{ 
    size_t len = 0;
    char* line;
    int Encontro = FALSE;                                                             //Usado para ver cuando se encontro una accion
    char toPrensa[1000];                                                              //Mensaje que se envia a la prensa
    char* Decision;                                                                   //Se usa para revisar que que decision en la accion es                 
    srand(time(NULL));
    FILE* fp = fopen("Judicial.acc", "r");

    //Encuentra una accion, con 20% de probabilidad
    while(TRUE){
        while(getline(&line, &len, fp) != -1){
            if(strlen(line) > 2 && strchr(line, ':') == NULL && rand()%5 == 1){       //Mientras la linea no sea un espacio, no contenga el caracter :
                Encontro=TRUE;                                                        //y con una posibilidad de 20% de ser escogido
                break;                                                                  
            }
        }
        if(!Encontro){
            rewind(fp);
        }
        else{
            break;
        }
    }
    //Ahora ejecuta la accion
    while(getline(&line, &len, fp)!=-1 && strlen(line)>2){
        Decision = strtok(line," ");
        if(strstr(Decision, "aprobacion")){

        }
        else if(strstr(Decision, "reprobacion")){

        }
        else if(strstr(Decision, "inclusivo")){

        }
        else if(strstr(Decision, "exclusivo")){

        }
        else if(strstr(Decision, "exito") && rand()%2 == 1){                          //Si la accion es exitosa                                                
            break;
        }
        else if(strstr(Decision, "fracaso") && rand()%2 == 1){                        //Si la accion fracaza
            break;
        }
    }
    Decision = strtok(NULL,"\0");                                                     //Toma el resto de el mensaje                
    pthread_mutex_lock(&mutex);                                                       //Para evitar inconsistencia en el pipe, se debe tratar como Seccion Critica
    strcpy(toPrensa, Decision);
    write(fd[1], toPrensa, sizeof(toPrensa));                                         //Se escribe la el mensaje de exito/fracaso al pipe que conecta este hilo con la prensa
    pthread_mutex_unlock(&mutex);                                                     //Se libera el mutex
 
    fclose(fp);
    pthread_exit(NULL);
}

/*Este hilo es la prensa y se encarga de hacer print de las acciones fracazadas o logradas*/
void *threadPrensa(void *vargp)
{
    char Hemeroteca[100][1000];
    char toPrensa[1000];
    for(int i=0; i<100; i++){
        memset(Hemeroteca[i], 0, sizeof(Hemeroteca[i]));
    }
    while(TRUE){
        read(fd[0], toPrensa, sizeof(toPrensa));                                      //Se lee lo que esta en el pipe

        if(strstr(toPrensa, "\n")){                                                   //Esto se hace ya que puede que el string no tenga el "\n" al final
            printf("%d.%s\n", day, toPrensa);
        }
        else{
            printf("%d.%s\n\n", day, toPrensa); 
        }
        strcpy(Hemeroteca[day-1], toPrensa);                                          //Coloca el mensaje en la Hemeroteca
        memset(toPrensa, 0, sizeof(toPrensa));                                        //Se vacia toPrensa
        day++;
        if(day-1==daysMax){
            pthread_exit(NULL);
        }
    }
}

void main(int argc, char *argv[]){

    daysMax = atoi(argv[1]); 
    pthread_t thread_id_prensa;

    if(pipe(fd)<0){                                                                   //Inicializa el pipe
        perror("pipe ");                                              
        exit(1);
    }

    pthread_create(&thread_id_ejec, NULL, threadEjec, NULL);
    pthread_create(&thread_id_legis, NULL, threadLegis, NULL);
    pthread_create(&thread_id_jud, NULL, threadJud, NULL);
    pthread_create(&thread_id_prensa, NULL, threadPrensa, NULL);

    pthread_join(thread_id_ejec, NULL);                                               //
    pthread_join(thread_id_legis, NULL);                                              // Espera que los hilos terminen de ejecutar
    pthread_join(thread_id_jud, NULL);                                                //
    pthread_join(thread_id_prensa, NULL);                                             //
}