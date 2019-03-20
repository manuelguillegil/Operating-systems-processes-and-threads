#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <pthread.h> 
#include <string.h>

#define TRUE 1
#define FALSE 0

int fd[2];                                                                            //Pipe

pthread_mutex_t mutexApro;                                                            //Mutex para sincronizacion entre subprocesos y el hilo de aprobacion
pthread_mutex_t mutexAproEjec;                                                        //Mutex para sincronizacion entre subprocesos y el hilo de aprobacion de Ejecutivo
int fdApro[2];                                                                        //Pipe para enviar la aprobacion/reprobacion
int fdAproEjec[2];                                                                    //Pipe para enviar la aprobacion/reprobacion de Ejecutivo

int day=1;                                                                            //Inicializa la variable day que cuenta en que dia esta
int daysMax;                                                                          //int que indica cual es el dia maximo 
int aunTieneAcciones = 3;                                                             //int que indica cuantos poderes aun tienen acciones disponibles
int numMagistrados = 1;
int Magistrados[20];
int PorcentajeExitoEjec = 66;
int PorcentajeExitoLegis = 66;
pthread_mutex_t mutex;                                                                //Inicializa Mutex que se usa para el pipe entre hilos y la prensa
pthread_mutex_t mutex1;                                                               //Inicializa Mutex que se usa para modificar day
pthread_mutex_t mutexEjec;                                                            //Inicializa Mutex que se usa para el poder ejecutivo y evitar inconsistencia en Ejecutivo.acc
pthread_t thread_id_ejec;                                                             //Id de Ejecutivo
pthread_t thread_id_legis;                                                            //Id de Legislativo                                                            
pthread_t thread_id_jud;                                                              //Id de Judicial


/*Funcion para hacer delay, la usamos para cuando hay que hacer esperas menores
a 1 segundo*/
void delay(int delay){
    for(int i=0; i<delay; i++)
    {}
}

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
    srand(time(0)); 
    size_t len = 0;
    char* line;                                                           
    int Encontro;                                                         //Usado para ver cuando se encontro una accion
    int vacio;
    char toPrensa[200];                                                   //Mensaje que se envia a la prensa
    char* Decision = (char*)calloc(1, 200);                               //Se usa para revisar que que decision en la accion es
    char* nombreAccion = (char*)calloc(1, 200);
    int exito;                                                            //Variable para evaluar si la accion fue exitosa o no
    int cancel;
    FILE* fp;                                                             

    while(day-1<daysMax){
        pthread_mutex_lock(&mutexEjec);                                                   //Se bloquea el mutex para evitar inconsistencias en el archivo Ejecutivo.acc mientras se lee
                                                                                          //Ademas, el hilo aprobacionEjec no podra aprobar nada hasta que liberemos el mutex
        pthread_mutex_lock(&mutex1);
        day++;                                                                            //Se aumenta el dia
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
                day--;                                                                    //Se decrementa el dia ya que no es encontro accion
                aunTieneAcciones--;
                pthread_mutex_unlock(&mutex1);     
                pthread_mutex_unlock(&mutexEjec);                                         //Se desbloquea el mutex que se bloqueo al entrar a buscar acciones
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
            if((strstr(Decision, "aprobacion") || strstr(Decision, "reprobacion")) && cancel==FALSE){
                char* to_who = (char*)malloc(200);                              //Quien debera recibir la señal para que apruebe/repruebe
                strcpy(to_who, strtok(NULL,"\0"));
                //Tribunal Supremo aprueba
                if(strstr(to_who, "Tribunal Supremo")){  
                    int tot=0;
                    int i=0;
                    while(Magistrados[i]!=0){
                        tot+=Magistrados[i];
                        i++;
                    }     
                    int num;                                                                        //variable que dira si se aprobo/reprobo la accion
                    pthread_mutex_unlock(&mutexApro);
                    read(fdApro[0], &num, sizeof(num));
                    //Si requiere aprobacion
                    if(strstr(Decision, "aprobacion") && (num > tot/numMagistrados)){               //Si no se aprueba, se cancela la accion
                        cancel=TRUE;
                    }
                    //Si puede ser reprobado
                    else if(strstr(Decision, "reprobacion") && (num <= tot/numMagistrados)){        //Si hubo reprobacion, se cancela la accion
                        cancel=TRUE;
                    }
                }
                //Presidente/Magistrados aprueban
                else if(strstr(to_who, "Congreso")){
                    int num;                                                                        //variable que dira si se aprobo/reprobo la accion
                    pthread_mutex_unlock(&mutexApro);
                    read(fdApro[0], &num, sizeof(num));
                    //Si requiere aprobacion
                    if(strstr(Decision, "aprobacion") && (num > PorcentajeExitoLegis)){             //Si no se aprueba, se cancela la accion
                        cancel=TRUE;
                    }
                    //Si puede ser reprobado
                    else if(strstr(Decision, "reprobacion") && (num <= PorcentajeExitoLegis)){      //Si hubo reprobacion, se cancela la accion
                        cancel=TRUE;
                    }
                }
                free(to_who);
            }
            else if(strstr(Decision, "inclusivo") && cancel==FALSE){

            }
            else if(strstr(Decision, "exclusivo") && cancel==FALSE){

            }
            else if(strstr(Decision, "exito") && rand()%101<=PorcentajeExitoEjec && cancel==FALSE){        //Si la accion es exitosa
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
        pthread_mutex_unlock(&mutexEjec);                                                 //Se desbloquea para poder hacer cambios a Ejecutivo.acc o hacer aprobaciones
        delay(10000);                                                                     //Hace delay para que le de chance a otros hilos de avanzar
        
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
            //La accion requiere aprobacion/reprobacion de otro poder
            if((strstr(Decision, "aprobacion") || strstr(Decision, "reprobacion")) && cancel==FALSE){
                char* to_who = (char*)malloc(200);                              //Quien debera recibir la señal para que apruebe/repruebe
                strcpy(to_who, strtok(NULL,"\0"));
                int num;                                                                            //variable que dira si se aprobo/reprobo la accion
                //Tribunal Supremo aprueba
                if(strstr(to_who, "Tribunal Supremo")){  
                    int tot=0;
                    int i=0;
                    while(Magistrados[i]!=0){
                        tot+=Magistrados[i];
                        i++;
                    }                                                                           
                    pthread_mutex_unlock(&mutexApro);
                    read(fdApro[0], &num, sizeof(num));
                    //Si requiere aprobacion
                    if(strstr(Decision, "aprobacion") && (num > tot/numMagistrados)){               //Si no se aprueba, se cancela la accion
                        cancel=TRUE;
                    }
                    //Si puede ser reprobado
                    else if(strstr(Decision, "reprobacion") && (num <= tot/numMagistrados)){        //Si hubo reprobacion, se cancela la accion
                        cancel=TRUE;
                    }
                }
                //Presidente/Magistrados aprueban
                else{
                    pthread_mutex_unlock(&mutexAproEjec);
                    read(fdAproEjec[0], &num, sizeof(num));
                    //Si requiere aprobacion
                    if(strstr(Decision, "aprobacion") && (num > PorcentajeExitoEjec)){              //Si no se aprueba, se cancela la accion
                        cancel=TRUE;
                    }
                    //Si puede ser reprobado
                    else if(strstr(Decision, "reprobacion") && (num <= PorcentajeExitoEjec)){       //Si hubo reprobacion, se cancela la accion
                        cancel=TRUE;
                    }
                }
            }
            else if(strstr(Decision, "inclusivo") && cancel==FALSE){

            }
            else if(strstr(Decision, "exclusivo") && cancel==FALSE){ 

            }
            else if(strstr(Decision, "exito") && rand()%2 == 1 && cancel==FALSE){                   //Si la accion es exitosa
                exito=TRUE; 
                break;
            }
            else if(strstr(Decision, "fracaso")){                                                   //Si la accion fracaza
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
            //La accion requiere aprobacion/reprobacion de otro poder
            if((strstr(Decision, "aprobacion") || strstr(Decision, "reprobacion")) && cancel==FALSE){
                char* to_who = (char*)malloc(200);                                                    //Quien debera recibir la señal para que apruebe/repruebe
                strcpy(to_who, strtok(NULL,"\0"));
                int num;                                                                              //variable que dira si se aprobo/reprobo la accion
                //Congreso 
                if(strstr(to_who, "Congreso")){                                           
                    pthread_mutex_unlock(&mutexApro);
                    read(fdApro[0], &num, sizeof(num));
                    //Si debe ser aprobado
                    if(strstr(Decision, "aprobacion") && (num > PorcentajeExitoLegis)){               //Si no se aprueba, se cancela la accion
                        cancel=TRUE;
                    }
                    //Si puede ser reprobado
                    else if(strstr(Decision, "reprobacion") && (num <= PorcentajeExitoLegis)){        //Si hubo reprobacion, se cancela la accion
                        cancel=TRUE;
                    }
                }
                //Presidente o Ministros
                else{
                    pthread_mutex_unlock(&mutexAproEjec);
                    read(fdAproEjec[0], &num, sizeof(num));
                    //Si requiere aprobacion
                    if(strstr(Decision, "aprobacion") && (num > PorcentajeExitoEjec)){               //Si no se aprueba, se cancela la accion
                        cancel=TRUE;
                    }
                    //Si puede ser reprobado
                    else if(strstr(Decision, "reprobacion") && (num <= PorcentajeExitoEjec)){        //Si hubo reprobacion, se cancela la accion
                        cancel=TRUE;
                    }

                }
                free(to_who);
            }
            else if(strstr(Decision, "inclusivo") && cancel==FALSE){

            }
            else if(strstr(Decision, "exclusivo") && cancel==FALSE){

            }
            else if(strstr(Decision, "exito") && cancel==FALSE){                        //Si la accion es exitosa  
                int tot=0;
                int i=0;
                while(Magistrados[i]!=0){
                    tot+=Magistrados[i];
                    i++;
                }
                if(rand()%101 <= tot/numMagistrados){
                    exito=TRUE;                                             
                    break;
                }
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


void *aprobacionEjec(void *vargp)
{   
    int num;
    srand(time(0));
    while(TRUE){
        pthread_mutex_lock(&mutexAproEjec);                                            //Cuando le dan la señal, el envia la respuesta

        pthread_mutex_lock(&mutexEjec);
        num = rand()%101;          
        write(fdAproEjec[1], &num, sizeof(num));                                       //Escribe en el pipe la respuesta
        pthread_mutex_unlock(&mutexEjec);
    }
}

/*Hilo utilizado para hacer aprobaciones/reprobaciones del poder Judicial y Legislativo. Es un hilo ya que estos poderes pueden
aprobar/reprobar aun cuando estan ejecutando una accion*/
void *aprobacion(void *vargp)
{   
    pthread_t thread_aprobacionEjec;                                                   //hilo que se encarga de la aprobacion de Ejecutivo
    pthread_create(&thread_aprobacionEjec, NULL, aprobacionEjec, NULL);                //ADVERTENCIA!! Esto ocaciona Seg. Fault   
    int num;
    srand(time(0));
    while(TRUE){
        pthread_mutex_lock(&mutexApro);                                                //Cuando le dan la señal, el envia la respuesta
        num = rand()%101;          
        write(fdApro[1], &num, sizeof(num));                                           //Escribe en el pipe la respuesta
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

    memset(Magistrados, 0, sizeof(Magistrados));
    Magistrados[0] = 66;

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
    if(pipe(fdAproEjec)<0){                                                           //Inicializa el pipe
        perror("pipe ");                                              
        exit(1);
    }

    pthread_create(&thread_id_ejec, NULL, threadEjec, NULL);
    pthread_create(&thread_id_legis, NULL, threadLegis, NULL);
    pthread_create(&thread_id_jud, NULL, threadJud, NULL);
    pthread_create(&thread_id_prensa, NULL, threadPrensa, NULL);

    pthread_join(thread_id_ejec, NULL);                                               
    pthread_join(thread_id_legis, NULL);                                             // Espera que los hilos terminen de ejecutar
    pthread_join(thread_id_jud, NULL);                                              
    if(aunTieneAcciones<=0){
        sleep(1);                                                                    //Si los poderes se quedaron sin acciones antes de terminar los dias, se cancela a la Prensa
        pthread_cancel(thread_id_prensa);
        pthread_cancel(thread_aprobacion);
    }
    pthread_join(thread_id_prensa, NULL);                                             
}