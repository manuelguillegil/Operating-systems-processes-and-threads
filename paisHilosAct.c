#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <pthread.h> 
#include <string.h>

#define TRUE 1
#define FALSE 0

char* PrensaLine=NULL;
int day=1;
pthread_mutex_t mutex;

void *threadEjec(void *vargp) 
{ 
    size_t len = 0;
    char* line;
    int Encontro = FALSE;
    char * toPrensa = (char*)calloc(1, 1000);
    char* Decision;
    srand(time(NULL));                                                                   
    FILE* fp = fopen("Ejecutivo.acc", "r");

    //Encuentra una accion, con 20% de probabilidad
    while(TRUE){
        while(getline(&line, &len, fp) != -1){
            if(strlen(line) > 2 && strchr(line, ':') == NULL && rand()%5 == 1){       //Mientras la linea no sea un espacio, no contenga el caracter :
                Encontro=TRUE;                                                            //y con una posibilidad de 20% de ser escogido
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
        else if(strstr(Decision, "exito") && rand()%2 == 1){                               //Si la accion es exitosa
            break;
        }
        else if(strstr(Decision, "fracaso") && rand()%2 == 1){                             //Si la accion fracaza
            break;
        }
    }
    Decision = strtok(NULL,"\0");                                                  //Toma el resto de el mensaje
    pthread_mutex_lock(&mutex);                                                    //Se hace lock del mutex y se libera cuando Prensa haya terminado de imprimir y agregado a Hemeroteca
    strcpy(toPrensa, Decision);                                                    //Se copia a toPrensa ya que decision cambiara y toPrensa se quedara igual hasta que vuelva a pasar por aqui
    PrensaLine = toPrensa;

    fclose(fp);
    pthread_exit(NULL);
}

void *threadLegis(void *vargp) 
{ 
    size_t len = 0;
    char* line;
    int Encontro = FALSE;  
    char * toPrensa = (char*)calloc(1, 1000);
    char* Decision;                                                                  
    srand(time(NULL));
    FILE* fp = fopen("Legislativo.acc", "r");

    //Encuentra una accion, con 20% de probabilidad
    while(TRUE){
        while(getline(&line, &len, fp) != -1){
            if(strlen(line) > 2 && strchr(line, ':') == NULL && rand()%5 == 1){       //Mientras la linea no sea un espacio, no contenga el caracter :
                Encontro=TRUE;                                                            //y con una posibilidad de 20% de ser escogido
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
        else if(strstr(Decision, "exito") && rand()%2 == 1){                               //Si la accion es exitosa
            break;
        }
        else if(strstr(Decision, "fracaso") && rand()%2 == 1){                             //Si la accion fracaza
            break;
        }
    }
    Decision = strtok(NULL,"\0");                                                  //Toma el resto de el mensaje
    pthread_mutex_lock(&mutex);                                                    //Se hace lock del mutex y se libera cuando Prensa haya terminado de imprimir y agregado a Hemeroteca
    strcpy(toPrensa, Decision);                                                    //Se copia a toPrensa ya que decision cambiara y toPrensa se quedara igual hasta que vuelva a pasar por aqui
    PrensaLine = toPrensa;

    fclose(fp);
    pthread_exit(NULL);
}

void *threadJud(void *vargp) 
{ 
    size_t len = 0;
    char* line;
    int Encontro = FALSE;  
    char * toPrensa = (char*)calloc(1, 1000);
    char* Decision;                                                                  
    srand(time(NULL));
    FILE* fp = fopen("Judicial.acc", "r");

    //Encuentra una accion, con 20% de probabilidad
    while(TRUE){
        while(getline(&line, &len, fp) != -1){
            if(strlen(line) > 2 && strchr(line, ':') == NULL && rand()%5 == 1){       //Mientras la linea no sea un espacio, no contenga el caracter :
                Encontro=TRUE;                                                            //y con una posibilidad de 20% de ser escogido
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
        else if(strstr(Decision, "exito") && rand()%2 == 1){                               //Si la accion es exitosa                                                 //Toma el resto de el mensaje
            break;
        }
        else if(strstr(Decision, "fracaso") && rand()%2 == 1){                             //Si la accion fracaza
            break;
        }
    }
    Decision = strtok(NULL,"\0");                                                  //Toma el resto de el mensaje
    pthread_mutex_lock(&mutex);                                                    //Se hace lock del mutex y se libera cuando Prensa haya terminado de imprimir y agregado a Hemeroteca
    strcpy(toPrensa, Decision);                                                    //Se copia a toPrensa ya que decision cambiara y toPrensa se quedara igual hasta que vuelva a pasar por aqui
    PrensaLine = toPrensa;

    fclose(fp);
    pthread_exit(NULL);
}

/*Este hilo es la prensa y se encarga de hacer print de las acciones fracazadas o logradas*/
void *threadPrensa(void *vargp)
{

    char Hemeroteca[100][1000];
    char copy[1000];
    for(int i=0; i<100; i++){
        memset(Hemeroteca[i], 0, sizeof(Hemeroteca[i]));
    }
    while(TRUE){
        while(PrensaLine==NULL){                                      //Cuando se le pasa un valor a PrensaLine, la prensa lo imprime

        }
        if(strstr(PrensaLine, "\n")){                                 //Esto se hace ya que puede que el string no tenga el "\n" al final
            printf("%d.%s\n", day, PrensaLine);
        }
        else{
            printf("%d.%s\n\n", day, PrensaLine); 
        }

        strcpy(copy, PrensaLine);                                     //Hace un duplicado de el pointer
        strcpy(Hemeroteca[day-1], copy);                              //Coloca el duplicado en la Hemeroteca
        memset(copy, 0, sizeof(copy));                                //Se vacia copy
        PrensaLine = NULL;                                            //Se vuelve NULL el apuntador
        day++;
        pthread_mutex_unlock(&mutex);                                 //Aqui se libera el Mutex
        if(day==4){
            pthread_exit(NULL);
        }
    }
}

void main(int argc, char *argv[]){
    pthread_t thread_id_ejec;
    pthread_t thread_id_legis;
    pthread_t thread_id_jud;
    pthread_t thread_id_prensa;

    pthread_create(&thread_id_ejec, NULL, threadEjec, NULL);
    pthread_create(&thread_id_legis, NULL, threadLegis, NULL);
    pthread_create(&thread_id_jud, NULL, threadJud, NULL);
    pthread_create(&thread_id_prensa, NULL, threadPrensa, NULL);

    pthread_join(thread_id_ejec, NULL);
    pthread_join(thread_id_legis, NULL);
    pthread_join(thread_id_jud, NULL);
    pthread_join(thread_id_prensa, NULL);
}