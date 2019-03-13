#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <pthread.h> 
#include <string.h>

#define TRUE 1
#define FALSE 0

void *threadEjec(void *vargp) 
{ 
    size_t len = 0;
    char* line;
    int Encontro = FALSE;                                                                    
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
    fclose(fp);

    pthread_exit(NULL);
}

void main(int argc, char *argv[]){
    pthread_t thread_id_ejec;
    pthread_t thread_id_legis;
    pthread_t thread_id_jud;

    pthread_create(&thread_id_ejec, NULL, threadEjec, NULL);
    pthread_create(&thread_id_legis, NULL, threadLegis, NULL);
    pthread_create(&thread_id_jud, NULL, threadJud, NULL);

    getchar();

}