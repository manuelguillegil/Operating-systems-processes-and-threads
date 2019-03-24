#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <pthread.h> 
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
 
#define TRUE 1
#define FALSE 0

int fd[2];                                                                    //Pipe entre subprocesos y prensa
int fd1[2];                                                                   //Pipe entre subprocesos para compartir variable day
int fd2[2];                                                                   //Pipe entre subprocesos para compartir el array Magistrados
int fdApro[2];                                                                //Pipe para enviar la aprobacion/reprobacion
int fdAproEjec[2];                                                            //Pipe para enviar la aprobacion/reprobacion de Ejecutivo

int day=0;                                                                    //Inicializa la variable day que cuenta en que dia esta
int daysMax;                                                                  //int que indica cual es el dia maximo 
int aunTieneAcciones = 3;  
int *numMagistrados;                                                          //Numero de magistrados actualmente
int Magistrados[20];                                                          //Array con las probabilidades de exito de cada magistrado
int* PorcentajeExitoEjec;                                                     //Probabilidad de exito de Ejecutivo
int* PorcentajeExitoLegis;                                                    //Probabilidad de exito de Legislativo
int* aunEnInclusivoEjec;
int* aunEnInclusivoLegis;
int* aunEnInclusivoJud;
int* aunEnInclusivoArchivo;


/*Funcion para hacer delay, la usamos para cuando hay que hacer esperas menores
a 1 segundo*/
void delay(int delay){
    for(int i=0; i<delay; i++)
    {}
}

/*Funcion utilizada para eliminar acciones de los archivos .acc*/
void deleteAccion(FILE* fp, char* newName, char* Accion, char* temp){
    size_t len = 0;
    char* line;
    size_t read;
    FILE* f = fopen(temp, "w");
    
    while(getline(&line, &len, fp)!=-1){                                              //Itera por todas las lineas del archivo
        if(strstr(line, Accion)){                                                     //Si encuentra la accion, no la copia al archivo temporal
            while(read = getline(&line, &len, fp)!=-1){                               //Itera sobre los pasos de la accion
                if(strlen(line)<=2){                                                  //Cuando llegamos al final de la accion, volvemos a copiar linea por linea
                    break;
                }
            }
            if(!strstr(line, "\n")){                                                  //Si la accion no copiada era la ultima en el archivo, sale del ciclo principal
                break;
            }
            else{
                continue;
            }
        }
        fprintf(f, "%s", line);                                                       //Copia la linea a el archivo temporal leida del archivo
    }
    fclose(f);                                                                        //cierra f
    rename(temp, newName);                                                            //Renombra el archivo temporal para aplicar los cambios al archivo original
}

/*Funcion utilizada para Desbloquear Mutex luego de que ya no se necesite el uso exclusivo de un archivo*/
void abrirMutex(char* ArchivoEx, sem_t* semEjec, sem_t* semLegis, sem_t* semJud, sem_t* semArchivo){
    if(strstr(ArchivoEx, "Legislativo.acc")){
        sem_post(semLegis);
    }
    else if(strstr(ArchivoEx, "Judicial.acc")){
        sem_post(semJud);
    }
    else if(strstr(ArchivoEx, "Ejecutivo.acc")){
        sem_post(semEjec);
    }
    else{
        sem_post(semArchivo);
    }
}

/*Funcion utilizada para Bloquear Mutex cuando una accion requiere uso exclusivo de un archivo*/
void cerrarMutex(char* ArchivoEx, sem_t* semEjec, sem_t* semLegis, sem_t* semJud, sem_t* semArchivo){
    if(strstr(ArchivoEx, "Legislativo.acc")){
        sem_wait(semLegis);
        strcpy(ArchivoEx, "Legislativo.acc");
    }
    else if(strstr(ArchivoEx, "Judicial.acc")){
        sem_wait(semJud);
        strcpy(ArchivoEx, "Judicial.acc");
    }
    else if(strstr(ArchivoEx, "Ejecutivo.acc")){
        sem_wait(semEjec);
        strcpy(ArchivoEx, "Ejecutivo.acc");
    }
    else{
        sem_wait(semArchivo);
    }
}

/*Esta funcion contiene la instruccion de abrir de manera exclusiva un Archivo y leer/anular/Escribir. 
Es usada por los tres poderes*/
int Exclusivo(FILE* fp, char* ArchivoEx, char* line, size_t len, sem_t* semEjec, sem_t* semLegis, sem_t* semJud, sem_t* semArchivo){
    int cancel = FALSE;
    char* Decision = (char*)calloc(1, 200);

    //Se busca cual archivo es, si es uno de los poderes o es uno extra
    cerrarMutex(ArchivoEx, semEjec, semLegis, semJud, semArchivo);        //Bloquea el mutex requerido para usar el Arhcivo de manera exclusiva

    int lenght = ftell(fp);                               //Variable que contendra el numero de bytes desde el inicio del archivo inclusivo a la linea actual
    FILE* fExclusivo = fopen(ArchivoEx, "a+");            //Abre el archivo de forma Exclusiva y en modo Append y read (a+)
    fprintf(fExclusivo, "\n"); 

    while(getline(&line, &len, fp)!=-1){
        strcpy(Decision,strtok(line," "));
        //Si la instruccion nos pide leer el archivo y encontrar una linea
        if(strstr(Decision, "leer")){
            strcpy(Decision, strtok(NULL,"\0"));
            int leer = FALSE;
            rewind(fExclusivo); 

            while(getline(&line, &len, fExclusivo)!=-1){                    //Si encuentra una linea que concuerde, no se anula la accion
                if(strstr(line, Decision)){
                    leer = TRUE;
                    break;
                }
            }
            if(leer==FALSE){
                cancel = TRUE;
                fclose(fExclusivo);                                                        //Cierra el archivo
                abrirMutex(ArchivoEx, semEjec, semLegis, semJud, semArchivo);              //Desbloquea el mutex que bloqueo al principio
                break;
            }
        }
        //Si la instruccion nos pide escribir en el archivo
        else if(strstr(Decision, "escribir")){
            strcpy(Decision, strtok(NULL,"\n"));
            fprintf(fExclusivo, "\n");                                      //Escribe un newline
            fprintf(fExclusivo, "%s", Decision);                            //Escribe la instruccion
        }
        //Si la instruccion nos pide leer el archivo y ver si no se encuentra una linea
        else if(strstr(Decision, "anular")){
            strcpy(Decision, strtok(NULL,"\0"));
            int anular = FALSE;
            rewind(fExclusivo);

            while(getline(&line, &len, fExclusivo)!=-1){                    //Si se encuentra una linea que concuerde, se anula la accion
                if(strstr(line, Decision)){
                    anular = TRUE;
                    break;
                }
            }
            if(anular==TRUE){
                cancel = TRUE;
                fclose(fExclusivo);                                                    //Se cierra el Archivo
                abrirMutex(ArchivoEx, semEjec, semLegis, semJud, semArchivo);          //Desbloquea el mutex que bloqueo al principio
                break;
            }
        }
        //Si la instruccion ya no es escribir/leer/anular
        else{
            fseek(fp, lenght, SEEK_SET);                                    //Mueve el apuntador fp a la linea anterior
            fclose(fExclusivo);                                             //Cierra el archivo
            abrirMutex(ArchivoEx, semEjec, semLegis, semJud, semArchivo);   //Desbloquea el mutex que bloqueo al principio 
            break;
        }
        lenght = ftell(fp);                                                 //Obtiene el valor de la posicion (en bytes)
    }
    free(Decision);
    return cancel;
}

/*Esta funcion contiene la instruccion de abrir de manera inclusiva un Archivo y leer/anular/Escribir. 
Es usada por los tres poderes*/
int Inclusivo(FILE* fp, char* ArchivoIn, char* line, size_t len, sem_t* semEjec, sem_t* semLegis, sem_t* semJud, sem_t* semArchivo, sem_t* semMutex, sem_t*semMutex2, sem_t*semMutex3, sem_t*semMutex4){
    int cancel = FALSE;
    char* Decision = (char*)calloc(1, 200);

    if(strstr(ArchivoIn, "Ejecutivo")){                  //Si se abrira Ejecutivo de manera inclusiva, aumentamos el contador 
        sem_wait(semMutex);                              //Bloqueamos el mutex2 para evitar inconsistencia en la variable aunEnInclusivoEjec
        *aunEnInclusivoEjec++;
        if(*aunEnInclusivoEjec==1){
            strcpy(ArchivoIn, "Ejecutivo.acc");
            sem_wait(semEjec);                           //Si es la primera persona abriendolo, se bloquea para que Exclusivo no pueda entrar
        }
        sem_post(semMutex);                              //Desbloqueamos mutex2 ya que hicimos las modificaciones
    }
    else if(strstr(ArchivoIn, "Legislativo")){           //Si se abrira Legislativo de manera inclusiva, aumentamos el contador 
        sem_wait(semMutex2);                             //Bloqueamos el mutex3 para evitar inconsistencia en la variable aunEnInclusivoLegis
        *aunEnInclusivoLegis++;
        if(*aunEnInclusivoLegis==1){
            strcpy(ArchivoIn, "Legislativo.acc");
            sem_wait(semLegis);                          //Si es la primera persona abriendolo, se bloquea para que Exclusivo no pueda entrar
        }
        sem_post(semMutex2);                             //Desbloqueamos mutex3 ya que hicimos las modificaciones
    }
    else if(strstr(ArchivoIn, "Judicial")){              //Si se abrira Judicial de manera inclusiva, aumentamos el contador 
        sem_wait(semMutex3);                             //Bloqueamos el mutex4 para evitar inconsistencia en la variable aunEnInclusivoJud
        *aunEnInclusivoJud++;
        if(*aunEnInclusivoJud==1){
            strcpy(ArchivoIn, "Judicial.acc");
            sem_wait(semJud);                            //Si es la primera persona abriendolo, se bloquea para que Exclusivo no pueda entrar
        }
        sem_post(semMutex3);                             //Desbloqueamos mutex4 ya que hicimos las modificaciones
    }
    else{                                                //Si se abrira otro Archivo de manera inclusiva, aumentamos el contador 
        sem_wait(semMutex4);                             //Bloqueamos el mutex5 para evitar inconsistencia en la variable aunEnInclusivoArchivo
        *aunEnInclusivoArchivo++;
        if(*aunEnInclusivoArchivo==1){
            sem_wait(semArchivo);                        //Si es la primera persona abriendolo, se bloquea para que Exclusivo no pueda entrar
        }
        sem_post(semMutex4);                             //Desbloqueamos mutex5 ya que hicimos las modificaciones
    }

    int lenght = ftell(fp);                              //Variable que contendra el numero de bytes desde el inicio del archivo inclusivo a la linea actual
    FILE* fInclusivo = fopen(ArchivoIn, "a+");
    fprintf(fInclusivo, "\n"); 

    while(getline(&line, &len, fp)!=-1){
        strcpy(Decision,strtok(line," "));
        //Si la instruccion nos pide leer el archivo y encontrar una linea
        if(strstr(Decision, "leer")){
            strcpy(Decision, strtok(NULL,"\0"));
            int leer = FALSE; 
            rewind(fInclusivo);

            while(getline(&line, &len, fInclusivo)!=-1){
                if(strstr(line, Decision)){
                    leer = TRUE;
                    break;
                }
            }
            if(leer==FALSE){
                cancel = TRUE;
                fclose(fInclusivo);
                break;
            }
        }
        //Si la instruccion nos pide escribir en el archivo
        else if(strstr(Decision, "escribir")){
            strcpy(Decision, strtok(NULL,"\n"));
            fprintf(fInclusivo, "\n");  
            fprintf(fInclusivo, "%s", Decision);        
        }
        //Si la instruccion nos pide leer el archivo y ver si no se encuentra una linea
        else if(strstr(Decision, "anular")){
            strcpy(Decision, strtok(NULL,"\0"));
            int anular = FALSE;
            rewind(fInclusivo);

            while(getline(&line, &len, fInclusivo)!=-1){
                if(strstr(line, Decision)){
                    anular = TRUE;
                    break;
                }
            }
            if(anular==TRUE){
                cancel = TRUE;
                fclose(fInclusivo);
                break;
            }
        }
        //Si la instruccion ya no es escribir/leer/anular
        else{
            fseek(fp, lenght, SEEK_SET);                                                     //Mueve el apuntador fp a la linea anterior
            fclose(fInclusivo);
            break;
        }
        lenght = ftell(fp);                                                              //Obtiene el valor de la posicion (en bytes)
    }

    if(strstr(ArchivoIn, "Ejecutivo")){                  //Cuando salga de Ejecutivo.acc, se decrementa la variable
        sem_wait(semMutex);                              //Bloqueamos el mutex2 para evitar inconsistencia en la variable aunEnInclusivoEjec 
        *aunEnInclusivoEjec--;
        if(*aunEnInclusivoEjec==0){
            sem_post(semEjec);                           //Si es la ultima persona cerrandolo, se desbloquea el mutex para que otros puedan usar el Archivo
        }
        sem_post(semMutex);                              //Desbloqueamos mutex2 ya que hicimos las modificaciones
    }
    else if(strstr(ArchivoIn, "Legislativo")){           //Cuando salga de Legislativo.acc, se decrementa la variable
        sem_wait(semMutex2);                             //Bloqueamos el mutex3 para evitar inconsistencia en la variable aunEnInclusivoLegis 
        *aunEnInclusivoLegis--;
        if(*aunEnInclusivoLegis==0){
            sem_post(semLegis);                          //Si es la ultima persona cerrandolo, se desbloquea el mutex para que otros puedan usar el Archivo
        }
        sem_post(semMutex2);                             //Desbloqueamos mutex3 ya que hicimos las modificaciones
    }
    else if(strstr(ArchivoIn, "Judicial")){              //Cuando salga de Judicial.acc, se decrementa la variable
        sem_wait(semMutex3);                             //Bloqueamos el mutex4 para evitar inconsistencia en la variable aunEnInclusivoJud 
        *aunEnInclusivoJud--;
        if(*aunEnInclusivoJud==0){
            sem_post(semJud);                            //Si es la ultima persona cerrandolo, se desbloquea el mutex para que otros puedan usar el Archivo
        }
        sem_post(semMutex3);                             //Desbloqueamos mutex4 ya que hicimos las modificaciones
    }
    else{                                                //Cuando salga de otro Archivo, se decrementa la variable
        sem_wait(semMutex4);                             //Bloqueamos el mutex5 para evitar inconsistencia en la variable aunEnInclusivoArchivo 
        *aunEnInclusivoArchivo--;
        if(*aunEnInclusivoArchivo==0){
            sem_post(semArchivo);                        //Si es la ultima persona cerrandolo, se desbloquea el mutex para que otros puedan usar el Archivo
        }
        sem_post(semMutex4);                             //Desbloqueamos mutex5 ya que hicimos las modificaciones
    }
    free(Decision);
    return cancel;
}

void Ejecutivo() 
{   
    sem_t* semApro=sem_open("Aprobacion", O_CREAT, 0666, 0);
    sem_t* semEjec=sem_open("Ejecutivo", O_CREAT, 0666, 1);
    sem_t* semLegis=sem_open("Legislativo", O_CREAT, 0666, 1);
    sem_t* semJud=sem_open("Judicial", O_CREAT, 0666, 1);
    sem_t* semArchivo=sem_open("Archivo", O_CREAT, 0666, 1);
    sem_t* semMutex=sem_open("Mutex", O_CREAT, 0666, 1);
    sem_t* semMutex2=sem_open("Mutex2", O_CREAT, 0666, 1);
    sem_t* semMutex3=sem_open("Mutex3", O_CREAT, 0666, 1);
    sem_t* semMutex4=sem_open("Mutex4", O_CREAT, 0666, 1);

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

    while(TRUE){
        sem_wait(semEjec);

        read(fd1[0], &day, sizeof(day));                                  //Se lee el valor actualizado de day
        day++;                                                            //Se aumenta el dia
        write(fd1[1], &day, sizeof(day));                                 //Se actualiza el valor de day para todos los subprocesos
        if(day-1>=daysMax){                                               //Si se completaron los dias de ejecucion, se para el subproceso
            return;
        }

        vacio = TRUE;
        Encontro = FALSE;
        cancel = FALSE;
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
            if(vacio==TRUE){                                                      //Si el archivo de entrada esta vacio
                read(fd1[0], &day, sizeof(day));
                day--;                                                            //Se decrementa el dia ya que no es encontro accion
                write(fd1[1], &day, sizeof(day));
                sem_post(semEjec);
                printf("Out Ejec\n");
                return;
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
            if((strstr(Decision, "aprobacion") || strstr(Decision, "reprobacion")) && cancel==FALSE){
                char* to_who = (char*)malloc(200);                                                  //Quien debera recibir la señal para que apruebe/repruebe
                strcpy(to_who, strtok(NULL,"\0"));
                int num;                                                                            //variable que dira si se aprobo/reprobo la accion
                //Tribunal Supremo aprueba
                if(strstr(to_who, "Tribunal Supremo")){  
                    read(fd2[0], Magistrados, 20*sizeof(int));                                      //Actualiza el valor de Magistrados en este poder
                    write(fd2[1], Magistrados, 20*sizeof(int));                                     //
                    int tot=0;
                    int i=0;
                    while(Magistrados[i]!=0){
                        tot+=Magistrados[i];
                        i++;
                    }     
                    sem_post(semApro);                                                              //Manda una señal a el hilo que aprueba
                    read(fdApro[0], &num, sizeof(num));
                    //Si requiere aprobacion
                    if(strstr(Decision, "aprobacion") && (num > tot/ *numMagistrados)){               //Si no se aprueba, se cancela la accion
                        cancel=TRUE;
                    }
                    //Si puede ser reprobado
                    else if(strstr(Decision, "reprobacion") && (num <= tot/ *numMagistrados)){        //Si hubo reprobacion, se cancela la accion
                        cancel=TRUE;
                    }
                }
                //Presidente/Magistrados aprueban
                else if(strstr(to_who, "Congreso")){
                    sem_post(semApro);                                                              //Manda una señal a el hilo que aprueba
                    read(fdApro[0], &num, sizeof(num));
                    //Si requiere aprobacion
                    if(strstr(Decision, "aprobacion") && (num > *PorcentajeExitoLegis)){             //Si no se aprueba, se cancela la accion
                        cancel=TRUE;
                    }
                    //Si puede ser reprobado
                    else if(strstr(Decision, "reprobacion") && (num <= *PorcentajeExitoLegis)){      //Si hubo reprobacion, se cancela la accion
                        cancel=TRUE;
                    }
                }
                free(to_who);
            }
            //La accion pide usar un archivo de forma inclusiva
            else if(strstr(Decision, "inclusivo") && cancel==FALSE){
                char* ArchivoIn = (char*)malloc(200);
                strcpy(ArchivoIn, strtok(NULL,"\n"));

                cancel = Inclusivo(fp, ArchivoIn, line, len, semEjec, semLegis, semJud, semArchivo, semMutex, semMutex2, semMutex3, semMutex4);

                free(ArchivoIn);                                                                     //Liberamos memoria de la variable
            }
            //La accion pide usar un archivo de forma exclusiva
            else if(strstr(Decision, "exclusivo") && cancel==FALSE){
                char* ArchivoEx = (char*)malloc(200);
                strcpy(ArchivoEx, strtok(NULL,"\n"));

                cancel = Exclusivo(fp, ArchivoEx, line, len, semEjec, semLegis, semJud, semArchivo);

                free(ArchivoEx);                                                                     //Liberamos memoria de la variable
            }
            else if(strstr(Decision, "exito") && (rand()%101<=*PorcentajeExitoEjec) && cancel==FALSE){      //Si la accion es exitosa
                exito=TRUE;                        
                break;
            }
            else if(strstr(Decision, "fracaso")){                                        //Si la accion fracaza
                exito=FALSE;
                break;
            }
        }
        strcpy(Decision, strtok(NULL,"\0"));                                             //Toma el resto de el mensaje  
                                       
        if(exito==TRUE){                                                                 //Si la accion es exitosa, se elimina de el archivo
            rewind(fp);
            deleteAccion(fp, "Ejecutivo.acc", nombreAccion, "temp1.txt");
        }
        strcpy(toPrensa, "Presidente ");
        strcat(toPrensa, Decision);
        write(fd[1], toPrensa, sizeof(toPrensa));                                         //Se escribe la el mensaje de exito/fracaso al pipe que conecta este subproceso con la prensa
        
        fclose(fp);                                                                       //Cierra el archivo para abrirlo nuevamente cuando se reinicie el ciclo
        sem_post(semEjec);
        delay(10000);
    } 
    return;
}


void Legislativo() 
{   
    sem_t* semAproEje=sem_open("AprobacionEjec", O_CREAT, 0666, 0);
    sem_t* semApro=sem_open("Aprobacion", O_CREAT, 0666, 0);
    sem_t* semEjec=sem_open("Ejecutivo", O_CREAT, 0666, 1);
    sem_t* semLegis=sem_open("Legislativo", O_CREAT, 0666, 1);
    sem_t* semJud=sem_open("Judicial", O_CREAT, 0666, 1);
    sem_t* semArchivo=sem_open("Archivo", O_CREAT, 0666, 1);
    sem_t* semMutex=sem_open("Mutex", O_CREAT, 0666, 1);
    sem_t* semMutex2=sem_open("Mutex2", O_CREAT, 0666, 1);
    sem_t* semMutex3=sem_open("Mutex3", O_CREAT, 0666, 1);
    sem_t* semMutex4=sem_open("Mutex4", O_CREAT, 0666, 1);

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

    while(TRUE){
        sem_wait(semLegis);

        read(fd1[0], &day, sizeof(day));                                              //Se lee el valor actualizado de day
        day++;                                                                        //Se aumenta el dia
        write(fd1[1], &day, sizeof(day));                                             //Se actualiza el valor de day para todos los subprocesos
        if(day-1>=daysMax){                                                           //Si se completaron los dias de ejecucion, se para el subproceso
            return;
        }

        vacio = TRUE;
        Encontro = FALSE;
        cancel = FALSE;
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
            if(vacio==TRUE){                                                      //Si el archivo de entrada esta vacio
                read(fd1[0], &day, sizeof(day));
                day--;                                                            //Se decrementa el dia ya que no es encontro accion
                write(fd1[1], &day, sizeof(day));
                sem_post(semLegis);
                printf("Out Legis\n");
                return;
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
            if((strstr(Decision, "aprobacion") || strstr(Decision, "reprobacion")) && cancel==FALSE){
                char* to_who = (char*)malloc(200);                                                  //Quien debera recibir la señal para que apruebe/repruebe
                strcpy(to_who, strtok(NULL,"\0"));
                int num;                                                                            //variable que dira si se aprobo/reprobo la accion
                //Tribunal Supremo aprueba
                if(strstr(to_who, "Tribunal Supremo")){  
                    read(fd2[0], Magistrados, 20*sizeof(int));                                      //Actualiza el valor de Magistrados en este poder
                    write(fd2[1], Magistrados, 20*sizeof(int));                                     //
                    int tot=0;
                    int i=0;
                    while(Magistrados[i]!=0){
                        tot+=Magistrados[i];
                        i++;
                    }                                                                           
                    sem_post(semApro);
                    read(fdApro[0], &num, sizeof(num));
                    //Si requiere aprobacion
                    if(strstr(Decision, "aprobacion") && (num > tot/ *numMagistrados)){               //Si no se aprueba, se cancela la accion
                        cancel=TRUE;
                    }
                    //Si puede ser reprobado
                    else if(strstr(Decision, "reprobacion") && (num <= tot/ *numMagistrados)){        //Si hubo reprobacion, se cancela la accion
                        cancel=TRUE;
                    }
                }
                //Presidente/Magistrados aprueban
                else{
                    sem_post(semAproEje);
                    read(fdAproEjec[0], &num, sizeof(num));
                    //Si requiere aprobacion
                    if(strstr(Decision, "aprobacion") && (num > *PorcentajeExitoEjec)){              //Si no se aprueba, se cancela la accion
                        cancel=TRUE;
                    }
                    //Si puede ser reprobado
                    else if(strstr(Decision, "reprobacion") && (num <= *PorcentajeExitoEjec)){       //Si hubo reprobacion, se cancela la accion
                        cancel=TRUE;
                    }
                }
                free(to_who);
            }
            else if(strstr(Decision, "inclusivo") && cancel==FALSE){
                char* ArchivoIn = (char*)malloc(200);
                strcpy(ArchivoIn, strtok(NULL,"\n"));

                cancel = Inclusivo(fp, ArchivoIn, line, len, semEjec, semLegis, semJud, semArchivo, semMutex, semMutex2, semMutex3, semMutex4);

                free(ArchivoIn);                                                                     //Liberamos memoria de la variable
            }
            else if(strstr(Decision, "exclusivo") && cancel==FALSE){ 
                char* ArchivoEx = (char*)malloc(200);
                strcpy(ArchivoEx, strtok(NULL,"\n"));

                cancel = Exclusivo(fp, ArchivoEx, line, len, semEjec, semLegis, semJud, semArchivo);

                free(ArchivoEx);                                                                          //Liberamos memoria de la variable
            }
            else if(strstr(Decision, "exito") && (rand()%101<=*PorcentajeExitoLegis) && cancel==FALSE){       //Si la accion es exitosa
                exito=TRUE; 
                break;
            }
            else if(strstr(Decision, "fracaso")){                                        //Si la accion fracaza
                exito=FALSE;
                break;
            }
        }
        strcpy(Decision, strtok(NULL,"\0"));                                              //Toma el resto de el mensaje 
                   
        if(exito==TRUE){                                                                  //Si la accion es exitosa, se elimina de el archivo
            rewind(fp);
            deleteAccion(fp, "Legislativo.acc", nombreAccion, "temp2.txt");
        }
        strcpy(toPrensa, "Congreso ");
        strcat(toPrensa, Decision);
        write(fd[1], toPrensa, sizeof(toPrensa));                                         //Se escribe la el mensaje de exito/fracaso al pipe que conecta este subproceso con la prensa

        fclose(fp);
        sem_post(semLegis);
        delay(10000);
    }
    return;
}


void Judicial() 
{   
    sem_t* semAproEje=sem_open("AprobacionEjec", O_CREAT, 0666, 0);
    sem_t* semApro=sem_open("Aprobacion", O_CREAT, 0666, 0);
    sem_t* semEjec=sem_open("Ejecutivo", O_CREAT, 0666, 1);
    sem_t* semLegis=sem_open("Legislativo", O_CREAT, 0666, 1);
    sem_t* semJud=sem_open("Judicial", O_CREAT, 0666, 1);
    sem_t* semArchivo=sem_open("Archivo", O_CREAT, 0666, 1);
    sem_t* semMutex=sem_open("Mutex", O_CREAT, 0666, 1);
    sem_t* semMutex2=sem_open("Mutex2", O_CREAT, 0666, 1);
    sem_t* semMutex3=sem_open("Mutex3", O_CREAT, 0666, 1);
    sem_t* semMutex4=sem_open("Mutex4", O_CREAT, 0666, 1);

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

    while(TRUE){ 
        if(numMagistrados<=0){                                                //Si el numero de magistrados es menor que 0, debe esperar en su paso
            while(TRUE){}
        }

        sem_wait(semJud);

        read(fd1[0], &day, sizeof(day));                                      //Se lee el valor actualizado de day
        day++;                                                                //Se aumenta el dia
        write(fd1[1], &day, sizeof(day));                                     //Se actualiza el valor de day para todos los subprocesos
        if(day-1>=daysMax){                                                   //Si se completaron los dias de ejecucion, se para el subproceso
            return;
        }

        Encontro = FALSE; 
        vacio = TRUE;   
        cancel = FALSE;
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
                read(fd1[0], &day, sizeof(day));
                day--;                                                        //Se decrementa el dia ya que no es encontro accion
                write(fd1[1], &day, sizeof(day));
                sem_post(semJud);
                printf("Out Jud\n");
                return;
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
                    sem_post(semApro);
                    read(fdApro[0], &num, sizeof(num));
                    //Si debe ser aprobado
                    if(strstr(Decision, "aprobacion") && (num > *PorcentajeExitoLegis)){               //Si no se aprueba, se cancela la accion
                        cancel=TRUE;
                    }
                    //Si puede ser reprobado
                    else if(strstr(Decision, "reprobacion") && (num <= *PorcentajeExitoLegis)){        //Si hubo reprobacion, se cancela la accion
                        cancel=TRUE;
                    }
                }
                //Presidente o Ministros
                else{
                    sem_post(semAproEje);
                    read(fdAproEjec[0], &num, sizeof(num));
                    //Si requiere aprobacion
                    if(strstr(Decision, "aprobacion") && (num > *PorcentajeExitoEjec)){               //Si no se aprueba, se cancela la accion
                        cancel=TRUE;
                    }
                    //Si puede ser reprobado
                    else if(strstr(Decision, "reprobacion") && (num <= *PorcentajeExitoEjec)){        //Si hubo reprobacion, se cancela la accion
                        cancel=TRUE;
                    }

                }
                free(to_who);                                                                        //Liberamos memoria de la variable
            }
            else if(strstr(Decision, "inclusivo") && cancel==FALSE){
                char* ArchivoIn = (char*)malloc(200);
                strcpy(ArchivoIn, strtok(NULL,"\n"));

                cancel = Inclusivo(fp, ArchivoIn, line, len, semEjec, semLegis, semJud, semArchivo, semMutex, semMutex2, semMutex3, semMutex4);

                free(ArchivoIn);                                                                     //Liberamos memoria de la variable
            }
            else if(strstr(Decision, "exclusivo") && cancel==FALSE){
                char* ArchivoEx = (char*)malloc(200);                                   //Variable que tendra el nombre del archivo que se usara de manera Exclusiva
                strcpy(ArchivoEx, strtok(NULL,"\n"));

                cancel = Exclusivo(fp, ArchivoEx, line, len, semEjec, semLegis, semJud, semArchivo);

                free(ArchivoEx);                                                        //Liberamos memoria de la variable
            }
            else if(strstr(Decision, "exito") && cancel==FALSE){                        //Si la accion es exitosa  
                read(fd2[0], Magistrados, 20*sizeof(int));                              //Actualiza el valor de Magistrados en este poder
                write(fd2[1], Magistrados, 20*sizeof(int));                             //
                int tot=0;
                int i=0;
                while(Magistrados[i]!=0){                                               //Se calcula la media aritmetica de los Magistrados
                    tot+=Magistrados[i];
                    i++;
                }
                if(rand()%101 <= tot/ *numMagistrados){
                    exito=TRUE;                                             
                    break;
                }
            }
            else if(strstr(Decision, "fracaso")){                                        //Si la accion fracaza
                exito=FALSE;
                break;
            }
        }
        strcpy(Decision, strtok(NULL,"\0"));                                            //Toma el resto de el mensaje 
             
        if(exito==TRUE){                                                                //Si la accion es exitosa, se elimina de el archivo
            rewind(fp);
            deleteAccion(fp, "Judicial.acc", nombreAccion, "temp3.txt");
        }
        strcpy(toPrensa, "Tribunal Supremo ");
        strcat(toPrensa, Decision);
        write(fd[1], toPrensa, sizeof(toPrensa));                                       //Se escribe la el mensaje de exito/fracaso al pipe que conecta este subproceso con la prensa
        
        fclose(fp);
        sem_post(semJud);
        delay(10000);
    }
    return;
}

void Prensa(){
    //Proceso padre es el proceso Prensa
    char Hemeroteca[daysMax][200];
    char toPrensa[200];
    int print=1;                                                                      //Variable para saber que dia se hizo la accion
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
}

void aprobacionEjec(){
    sem_t* semAproEjec=sem_open("AprobacionEjec", O_CREAT, 0666, 0);
    sem_t* semEjec=sem_open("Ejecutivo", O_CREAT, 0666, 1);
    int num;
    srand(time(0));
    while(TRUE){
        sem_wait(semAproEjec);                                                         //Cuando le dan la señal, el envia la respuesta

        sem_wait(semEjec);                                                             //Espera a que el Presidente este libre
        num = rand()%101;          
        write(fdAproEjec[1], &num, sizeof(num));                                       //Escribe en el pipe la respuesta
        sem_post(semEjec);                                              
    }
}


void aprobacion()
{   
    if(fork() == 0){
        aprobacionEjec();
    }
    else{
        sem_t* semApro=sem_open("Aprobacion", O_CREAT, 0666, 0);
        int num;
        srand(time(0));
        while(TRUE){
            sem_wait(semApro);                                                             //Cuando le dan la señal, el envia la respuesta
            num = rand()%101;          
            write(fdApro[1], &num, sizeof(num));                                           //Escribe en el pipe la respuesta
        }
    }
}


void main(int argc, char *argv[]){

    sem_unlink("Aprobacion");                                           //
    sem_unlink("AprobacionEjec");                                       //
    sem_unlink("Ejecutivo");                                            //
    sem_unlink("Legislativo");                                          //
    sem_unlink("Judicial");                                             //Si el semaforo existia antes de iniciar el programa, lo elimina
    sem_unlink("Mutex");                                                //
    sem_unlink("Mutex2");                                               //
    sem_unlink("Mutex3");                                               //
    sem_unlink("Mutex4");                                               //

    daysMax = atoi(argv[1]); 
    if(daysMax<=0){
        exit(1);
    }

    PorcentajeExitoEjec = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);   //
    PorcentajeExitoLegis = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);  //
    numMagistrados = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);        //
    aunEnInclusivoEjec = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);    //Comparte la variable entre padre e Hijo
    aunEnInclusivoLegis = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);   //
    aunEnInclusivoJud = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);     //
    aunEnInclusivoArchivo = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0); //
    *PorcentajeExitoEjec = 66;                                          //
    *PorcentajeExitoLegis = 66;                                         //
    *numMagistrados = 8;                                                //
    *aunEnInclusivoEjec = 0;                                            //Inicializa las variables
    *aunEnInclusivoLegis = 0;                                           //
    *aunEnInclusivoJud = 0;                                             //
    *aunEnInclusivoArchivo = 0;                                         //

    memset(Magistrados, 0, sizeof(Magistrados));
    for(int i=0; i<8; i++){                                             //Inicializa los 8 magistrados
        Magistrados[i] = 66;
    }

    if(pipe(fd)<0){                                                     //Inicializa el pipe fd
        perror("pipe ");                                              
        exit(1);
    }
    if(pipe(fd1)<0){                                                    //Inicializa el pipe fd1
        perror("pipe ");                                              
        exit(1);
    }
    if(pipe(fd2)<0){                                                    //Inicializa el pipe fd2
        perror("pipe ");                                              
        exit(1);
    }
    if(pipe(fdApro)<0){                                                 //Inicializa el pipe fdApro
        perror("pipe ");                                              
        exit(1);
    }
    if(pipe(fdAproEjec)<0){                                             //Inicializa el pipe fdAproEjec
        perror("pipe ");                                              
        exit(1);
    }

    write(fd1[1], &day, sizeof(day));
    write(fd2[1], Magistrados, 20*sizeof(int));
    if (fork() == 0){
        //Subproceso para Ejecutivo 
        Ejecutivo(); 
    }
    else{
        if (fork() == 0){ 
            //Subproceso para Legislativo
            Legislativo(); 
        }
        else{
            if (fork() == 0){ 

                //Subproceso para Judicial
                Judicial(); 
            }
            else{
                if(fork() == 0){
                    //Subproceso para Prensa
                    Prensa();
                }
                else{
                    if(fork() == 0){
                        //Subproceso para las aprobaciones
                        aprobacion();
                    }
                    else{
                        getchar();
                    }
                }                                                                  
            }
        } 
    }                                      
}