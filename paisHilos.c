#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <pthread.h> 
#include <string.h>

#define TRUE 1
#define FALSE 0

int fd[2];                                                                            //Pipe

pthread_mutex_t mutexApro;                                                            //Mutex para sincronizacion entre subprocesos y el hilo de aprobacion
int fdApro[2];                                                                        //Pipe para enviar la aprobacion/reprobacion

int day=1;                                                                            //Inicializa la variable day que cuenta en que dia esta
int daysMax;                                                                          //int que indica cual es el dia maximo 
int aunTieneAcciones = 3;                                                             //int que indica cuantos poderes aun tienen acciones disponibles
pthread_mutex_t mutex;                                                                //Inicializa Mutex que se usa para el pipe entre hilos y la prensa
pthread_mutex_t mutex1;                                                               //Inicializa Mutex que se usa para modificar day
pthread_t thread_id_ejec;                                                             //Id de Ejecutivo
pthread_t thread_id_legis;                                                            //Id de Legislativo                                                            
pthread_t thread_id_jud;                                                              //Id de Judicial

void deleteAccion(FILE* fp, char* newName, char* Accion){
    size_t len = 0;
    char* line;
    FILE* f = fopen("temp.txt", "w");
    
    while(getline(&line, &len, fp)!=-1){                                              //Itera por todas las lineas del archivo
        if(strstr(line, Accion)){                                                     //Si encuentra la accion, no la copia al archivo temporal
            while(getline(&line, &len, fp)!=-1){                                      //Itera sobre los pasos de la accion
                if(strlen(line)<=2){                                                  //Cuando llegamos al final de la accion, volvemos a copiar linea por linea
                    break;
                }
            }
            if(getline(&line, &len, fp)==-1){                                         //Si la accion no copiada era la ultima en el archivo, sale del ciclo principal
                break;
            }
        }
        fprintf(f, "%s", line);                                                       //Copia la linea a el archivo temporal leida del archivo
    }
    fclose(f);                                                                        //cierra f
    rename("temp.txt", newName);                                                      //Renombra el archivo temporal para aplicar los cambios al archivo original
}

void *threadEjec(void *vargp) 
{ 
    size_t len = 0;
    char* line;                                                           
    int Encontro;                                                         //Usado para ver cuando se encontro una accion
    int vacio;
    char toPrensa[200];                                                   //Mensaje que se envia a la prensa
    char* Decision = (char*)calloc(1, 200);                               //Se usa para revisar que que decision en la accion es
    char* nombreAccion = (char*)calloc(1, 200);
    int exito;                                                            //Variable para evaluar si la accion fue exitosa o no
    int cancel;
    srand(time(0)); 
    FILE* fp;                                                             

    while(day-1<daysMax){
        pthread_mutex_lock(&mutex1);
        day++;                                                            //Se aumenta el dia
        pthread_mutex_unlock(&mutex1);

        cancel = FALSE;
        vacio = TRUE;
        Encontro = FALSE;
        fp = fopen("Ejecutivo.acc", "r");

        //Encuentra una accion, con 20% de probabilidad
        while(TRUE){
            while(getline(&line, &len, fp) != -1){
                if(strlen(line) > 2 && strchr(line, ':') == NULL){                        //Mientras la linea no sea un espacio, no contenga el caracter :
                    vacio = FALSE;
                    if(rand()%5 == 1){                                                    //y con una posibilidad de 20% de ser escogido
                        Encontro=TRUE;
                        strcpy(nombreAccion, line);
                        break;
                    }                                                                       
                }
            }
            if(vacio==TRUE){
                pthread_mutex_lock(&mutex1);
                day--;                                                            //Se decrementa el dia ya que no es encontro accion
                aunTieneAcciones--;
                pthread_mutex_unlock(&mutex1);
                pthread_exit(NULL);
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
            strcpy(Decision,strtok(line," "));
            //La accion requiere aprobacion de otro poder
            if(strstr(Decision, "aprobacion") && cancel==FALSE){
                strcpy(Decision, strtok(NULL,"\0")); 
                if(strstr(Decision, "Tribunal Supremo") || strstr(Decision, "Congreso")){   
                    int num;                                                    //variable que dira si se aprobo/reprobo la accion
                    pthread_mutex_unlock(&mutexApro);
                    read(fdApro[0], &num, sizeof(num));
                    if(num==0){                                                 //Si no hubo aprobacion, se cancela la accion
                        printf("cancelado");
                        cancel=TRUE;
                    }
                }
            }
            //La accion puede ser reprobada por otro poder
            else if(strstr(Decision, "reprobacion") && cancel==FALSE){
                strcpy(Decision, strtok(NULL,"\0")); 
                if(strstr(Decision, "Tribunal Supremo") || strstr(Decision, "Congreso")){        
                    int num;                                                    //variable que dira si se aprobo/reprobo la accion
                    pthread_mutex_unlock(&mutexApro);
                    read(fdApro[0], &num, sizeof(num));
                    if(num==1){                                                 //Si hubo reprobacion, se cancela la accion
                        printf("cancelado");
                        cancel=TRUE;
                    }
                }
            }
            else if(strstr(Decision, "inclusivo") && cancel==FALSE){

            }
            else if(strstr(Decision, "exclusivo") && cancel==FALSE){

            }
            else if(strstr(Decision, "exito") && rand()%2 == 1 && cancel==FALSE){        //Si la accion es exitosa
                exito=TRUE;                        
                break;
            }
            else if(strstr(Decision, "fracaso")){                                        //Si la accion fracaza
                exito=FALSE;
                break;
            }
        }
        strcpy(Decision, strtok(NULL,"\0"));                                             //Toma el resto de el mensaje    
        pthread_mutex_lock(&mutex);                                                      //Para evitar inconsistencia en el pipe, se debe tratar como Seccion Critica
        if(exito==TRUE){                                                                 //Si la accion es exitosa, se elimina de el archivo
            rewind(fp);
            deleteAccion(fp, "Ejecutivo.acc", nombreAccion);
        }
        strcpy(toPrensa, "Presidente ");
        strcat(toPrensa, Decision);
        write(fd[1], toPrensa, sizeof(toPrensa));                                         //Se escribe la el mensaje de exito/fracaso al pipe que conecta este hilo con la prensa
        pthread_mutex_unlock(&mutex);                                                     //Se libera el mutex

        fclose(fp);                                                                       //Cierra el archivo para abrirlo nuevamente cuando se reinicie el ciclo
    } 
    pthread_exit(NULL);
}


void *threadLegis(void *vargp) 
{ 
    size_t len = 0;
    char* line;
    int Encontro;                                                                     //Usado para ver cuando se encontro una accion
    int vacio;
    char toPrensa[200];                                                               //Mensaje que se envia a la prensa
    char* Decision = (char*)calloc(1, 200);                                           //Se usa para revisar que que decision en la accion es
    char* nombreAccion = (char*)calloc(1, 200);
    int exito;                                                                        //Variable para evaluar si la accion fue exitosa o no
    int cancel;
    srand(time(0));
    FILE* fp;

    while(day-1<daysMax){
        pthread_mutex_lock(&mutex1);
        day++;                                                                        //Se aumenta el dia
        pthread_mutex_unlock(&mutex1);

        cancel = FALSE;
        vacio = TRUE;
        Encontro = FALSE;
        fp = fopen("Legislativo.acc", "r");

        //Encuentra una accion, con 20% de probabilidad
        while(TRUE){
            while(getline(&line, &len, fp) != -1){
                if(strlen(line) > 2 && strchr(line, ':') == NULL){                   //Mientras la linea no sea un espacio, no contenga el caracter :
                    vacio = FALSE;
                    if(rand()%5 == 1){                                               //y con una posibilidad de 20% de ser escogido
                        Encontro=TRUE;
                        strcpy(nombreAccion, line);
                        break;
                    }                                                                                             
                }
            }
            if(vacio==TRUE){
                pthread_mutex_lock(&mutex1);
                day--;                                                            //Se decrementa el dia ya que no es encontro accion
                aunTieneAcciones--;                                               //Como ejecutivo ya no tiene acciones, se decrementa la variable
                pthread_mutex_unlock(&mutex1);
                pthread_exit(NULL);
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
            strcpy(Decision,strtok(line," "));
            //La accion requiere aprobacion de otro poder
            if(strstr(Decision, "aprobacion") && cancel==FALSE){
                strcpy(Decision, strtok(NULL,"\0")); 
                if(strstr(Decision, "Tribunal Supremo") || strstr(Decision, "Congreso")){        //IMPORTANTE: Quitar Congreso luego
                    int num;                                                    //variable que dira si se aprobo/reprobo la accion
                    pthread_mutex_unlock(&mutexApro);
                    read(fdApro[0], &num, sizeof(num));
                    if(num==0){                                                 //Si no hubo aprobacion, se cancela la accion
                        printf("cancelado");
                        cancel=TRUE;
                    }
                }
            }
            //La accion puede ser reprobada por otro poder
            else if(strstr(Decision, "reprobacion") && cancel==FALSE){
                strcpy(Decision, strtok(NULL,"\0")); 
                if(strstr(Decision, "Tribunal Supremo") || strstr(Decision, "Congreso")){        //IMPORTANTE: Quitar Congreso luego
                    int num;                                                    //variable que dira si se aprobo/reprobo la accion
                    pthread_mutex_unlock(&mutexApro);
                    read(fdApro[0], &num, sizeof(num));
                    if(num==1){                                                 //Si hubo reprobacion, se cancela la accion
                        printf("cancelado");
                        cancel=TRUE;
                    }
                }
            }
            else if(strstr(Decision, "inclusivo") && cancel==FALSE){

            }
            else if(strstr(Decision, "exclusivo") && cancel==FALSE){ 

            }
            else if(strstr(Decision, "exito") && rand()%2 == 1 && cancel==FALSE){        //Si la accion es exitosa
                exito=TRUE; 
                break;
            }
            else if(strstr(Decision, "fracaso")){                                        //Si la accion fracaza
                exito=FALSE;
                break;
            }
        }
        strcpy(Decision, strtok(NULL,"\0"));                                             //Toma el resto de el mensaje                          
        pthread_mutex_lock(&mutex);                                                      //Para evitar inconsistencia en el pipe, se debe tratar como Seccion Critica
        if(exito==TRUE){                                                                 //Si la accion es exitosa, se elimina de el archivo
            rewind(fp);
            deleteAccion(fp, "Legislativo.acc", nombreAccion);
        }
        strcpy(toPrensa, "Congreso ");
        strcat(toPrensa, Decision);
        write(fd[1], toPrensa, sizeof(toPrensa));                                        //Se escribe la el mensaje de exito/fracaso al pipe que conecta este hilo con la prensa
        pthread_mutex_unlock(&mutex);                                                    //Se libera el mutex

        fclose(fp);
    }
    pthread_exit(NULL);
}


void *threadJud(void *vargp) 
{ 
    size_t len = 0;
    char* line;
    int Encontro;                                                             //Usado para ver cuando se encontro una accion
    int vacio;
    char toPrensa[200];                                                       //Mensaje que se envia a la prensa
    char* Decision = (char*)calloc(1, 200);                                   //Se usa para revisar que que decision en la accion es 
    char* nombreAccion = (char*)calloc(1, 200);   
    int exito;                                                                //Variable para evaluar si la accion fue exitosa o no
    int cancel;
    srand(time(0));
    FILE* fp;

    while(day-1<daysMax){ 
        pthread_mutex_lock(&mutex1); 
        day++;                                                                //Se aumenta el dia
        pthread_mutex_unlock(&mutex1);

        cancel = FALSE;
        Encontro = FALSE; 
        vacio = TRUE;   
        fp = fopen("Judicial.acc", "r");

        //Encuentra una accion, con 20% de probabilidad
        while(TRUE){
            while(getline(&line, &len, fp) != -1){
                if(strlen(line) > 2 && strchr(line, ':') == NULL){            //Mientras la linea no sea un espacio, no contenga el caracter :
                    vacio = FALSE;
                    if(rand()%5 == 1){                                        //y con una posibilidad de 20% de ser escogido
                        Encontro=TRUE;
                        strcpy(nombreAccion, line);
                        break;
                    }                                                                                                       
                }
            }
            if(vacio==TRUE){
                pthread_mutex_lock(&mutex1);
                day--;                                                        //Se decrementa el dia ya que no es encontro accion
                aunTieneAcciones--;                                           //Como ejecutivo ya no tiene acciones, se decrementa la variable
                pthread_mutex_unlock(&mutex1);
                pthread_exit(NULL);
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
            strcpy(Decision,strtok(line," "));
            //La accion requiere aprobacion de otro poder
            if(strstr(Decision, "aprobacion") && cancel==FALSE){
                strcpy(Decision, strtok(NULL,"\0")); 
                if(strstr(Decision, "Tribunal Supremo") || strstr(Decision, "Congreso")){    //IMPORTANTE: Quitar Tribunal Supremo luego
                    int num;                                                    //variable que dira si se aprobo/reprobo la accion
                    pthread_mutex_unlock(&mutexApro);
                    read(fdApro[0], &num, sizeof(num));
                    if(num==0){
                        printf("cancelado");
                        cancel=TRUE;
                    }
                }
            }
            //La accion puede ser reprobada por otro poder
            else if(strstr(Decision, "reprobacion") && cancel==FALSE){
                strcpy(Decision, strtok(NULL,"\0")); 
                if(strstr(Decision, "Tribunal Supremo") || strstr(Decision, "Congreso")){        //IMPORTANTE: Quitar Congreso luego
                    int num;                                                    //variable que dira si se aprobo/reprobo la accion
                    pthread_mutex_unlock(&mutexApro);
                    read(fdApro[0], &num, sizeof(num));
                    if(num==1){                                                 //Si hubo reprobacion, se cancela la accion
                        printf("cancelado");
                        cancel=TRUE;
                    }
                }

            }
            else if(strstr(Decision, "inclusivo") && cancel==FALSE){

            }
            else if(strstr(Decision, "exclusivo") && cancel==FALSE){

            }
            else if(strstr(Decision, "exito") && rand()%2 == 1 && cancel==FALSE){       //Si la accion es exitosa  
                exito=TRUE;                                             
                break;
            }
            else if(strstr(Decision, "fracaso")){                                       //Si la accion fracaza
                exito=FALSE;
                break;
            }
        }
        strcpy(Decision, strtok(NULL,"\0"));                                            //Toma el resto de el mensaje                
        pthread_mutex_lock(&mutex);                                                     //Para evitar inconsistencia en el pipe, se debe tratar como Seccion Critica
        if(exito==TRUE){                                                                //Si la accion es exitosa, se elimina de el archivo
            rewind(fp);
            deleteAccion(fp, "Judicial.acc", nombreAccion);
        }
        strcpy(toPrensa, "Tribunal Supremo ");
        strcat(toPrensa, Decision);
        write(fd[1], toPrensa, sizeof(toPrensa));                                       //Se escribe la el mensaje de exito/fracaso al pipe que conecta este hilo con la prensa
        pthread_mutex_unlock(&mutex);                                                   //Se libera el mutex

        fclose(fp);
    }
    pthread_exit(NULL);
}

/*Hilo utilizado para hacer aprobaciones/reprobaciones del poder Judicial y Legislativo. Es un hilo ya que estos poderes pueden
aprobar/reprobar aun cuando estan ejecutando una accion*/
void *aprobacion(void *vargp)
{   
    int num;
    srand(time(0));
    while(TRUE){
        pthread_mutex_lock(&mutexApro);                                                 //Cuando le dan la señal, el envia la respuesta
        num = rand()%2;          
        write(fdApro[1], &num, sizeof(num));                                            //Escribe en el pipe la respuesta
    }
}


/*Este hilo es la prensa y se encarga de hacer print de las acciones fracazadas o logradas*/
void *threadPrensa(void *vargp)
{   
    char Hemeroteca[daysMax][200];
    char toPrensa[200];
    int print=1;                                                                       //Variable para saber que dia se hizo la accion
    for(int i=0; i<daysMax; i++){
        memset(Hemeroteca[i], 0, sizeof(Hemeroteca[i]));
    }
    while(print-1!=daysMax){
        read(fd[0], toPrensa, sizeof(toPrensa));                                      //Se lee lo que esta en el pipe

        if(strstr(toPrensa, "\n")){                                                   //Esto se hace ya que puede que el string no tenga el "\n" al final
            printf("%d.%s\n", print, toPrensa);
        }
        else{
            printf("%d.%s\n\n", print, toPrensa); 
        }
        strcpy(Hemeroteca[print-1], toPrensa);                                        //Coloca el mensaje en la Hemeroteca
        memset(toPrensa, 0, sizeof(toPrensa));                                        //Se vacia toPrensa
        print++;
    }
    close(fd[0]);
    close(fd[1]);
    pthread_exit(NULL);
}

void main(int argc, char *argv[]){

    daysMax = atoi(argv[1]); 
    if(daysMax<=0){
        exit(1);
    }
    pthread_t thread_aprobacion;                                                      //hilo que se encarga de la aprobacion de Judicial y Legislativo
    pthread_create(&thread_aprobacion, NULL, aprobacion, NULL);
    pthread_mutex_lock(&mutexApro);

    pthread_t thread_id_prensa;
    if(pipe(fd)<0){                                                                   //Inicializa el pipe
        perror("pipe ");                                              
        exit(1);
    }
    if(pipe(fdApro)<0){                                                               //Inicializa el pipe
        perror("pipe ");                                              
        exit(1);
    }

    pthread_create(&thread_id_ejec, NULL, threadEjec, NULL);
    pthread_create(&thread_id_legis, NULL, threadLegis, NULL);
    pthread_create(&thread_id_jud, NULL, threadJud, NULL);
    pthread_create(&thread_id_prensa, NULL, threadPrensa, NULL);

    pthread_join(thread_id_ejec, NULL);                                               
    pthread_join(thread_id_legis, NULL);                                              // Espera que los hilos terminen de ejecutar
    pthread_join(thread_id_jud, NULL);                                                
    if(aunTieneAcciones==0){
        sleep(1);                                                                    //Si los poderes se quedaron sin acciones antes de terminar los dias, se cancela a la Prensa
        pthread_cancel(thread_id_prensa);
    }
    pthread_join(thread_id_prensa, NULL);                                             
}