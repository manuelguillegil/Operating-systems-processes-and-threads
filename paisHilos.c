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
    char lines[1000];                                                                   
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
    printf("%s", line);
    pthread_mutex_lock(&mutex);                                                     //Se hace lock del mutex y se libera cuando Prensa haya terminado de imprimir y agregado a Hemeroteca
    PrensaLine = line;
    fclose(fp);
    
    pthread_exit(NULL);
}

void *threadLegis(void *vargp) 
{ 
    size_t len = 0;
    char* line;
    int Encontro = FALSE;                                                                    
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
    printf("%s", line);
    pthread_mutex_lock(&mutex);                                                      //Se hace lock del mutex y se libera cuando Prensa haya terminado de imprimir y agregado a Hemeroteca
    PrensaLine = line;
    fclose(fp);

    pthread_exit(NULL);
}

void *threadJud(void *vargp) 
{ 
    size_t len = 0;
    char* line;
    int Encontro = FALSE;                                                                   
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
    printf("%s", line);
    pthread_mutex_lock(&mutex);                                                      //Se hace lock del mutex y se libera cuando Prensa haya terminado de imprimir y agregado a Hemeroteca
    PrensaLine = line;
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
        while(PrensaLine==NULL){                                      //Cuando se le pasa un valos a PrensaLine, la prensa lo imprime

        }
        printf("%d.%s", day, PrensaLine);
        strcpy(copy, PrensaLine);                                     //Hace un duplicado de el pointer
        strcpy(Hemeroteca[day-1], copy);                              //Coloca el duplicado en la Hemeroteca
        for(int i=0; i<day; i++){                                     //Revisa cada string en hemeroteca
            printf("Hemero: %s",Hemeroteca[i]);
        }
        memset(copy, 0, sizeof(copy));                                //Se vacia copy
        PrensaLine = NULL;                                            //Se vuelve NULL el apuntador
        day++;
        pthread_mutex_unlock(&mutex);                                 //Aqui se libera el Mutex
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
    pthread_create(&thread_id_jud, NULL, threadPrensa, NULL);

    getchar();

}