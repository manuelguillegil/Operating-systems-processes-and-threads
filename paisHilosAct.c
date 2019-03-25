#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <pthread.h> 
#include <string.h>

#define TRUE 1
#define FALSE 0

int fd[2];                                                                    //Pipe

pthread_mutex_t mutexApro;                                                    //Mutex para sincronizacion entre subprocesos y el hilo de aprobacion
pthread_mutex_t mutexAproEjec;                                                //Mutex para sincronizacion entre subprocesos y el hilo de aprobacion de Ejecutivo
int fdApro[2];                                                                //Pipe para enviar la aprobacion/reprobacion
int fdAproEjec[2];                                                            //Pipe para enviar la aprobacion/reprobacion de Ejecutivo

int aunEnInclusivoEjec = 0;                                                   //
int aunEnInclusivoLegis = 0;                                                  // Variables utilizadas para saber cuantos poderes aun esta en inclusivo
int aunEnInclusivoJud = 0;                                                    // (Las usamos para resolver el problema de Lectores/Escritores generado por
int aunEnInclusivoArchivo = 0;                                                // abrir archivos de manera inclusiva o exclusiva)
int day = 1;                                                                  //Variable day que cuenta en que dia esta
int daysMax;                                                                  //Entero que indica cual es el dia maximo 
int aunTieneAcciones = 3;                                                     //Entero que indica cuantos poderes aun tienen acciones disponibles
int numMagistrados = 8;                                                       //Numero de magistrados actualmente
int Magistrados[20];                                                          //Array con las probabilidades de exito de cada magistrado
int PorcentajeExitoEjec = 66;                                                 //Probabilidad de exito de Ejecutivo
int PorcentajeExitoLegis = 66;                                                //Probabilidad de exito de Legislativo
char* direccionEjec;                                                          //
char* direccionLegis;                                                         //Direccion de los archivos de cada Poder
char* direccionJud;                                                           //
pthread_mutex_t mutex;                                                        //Mutex que se usa para el pipe entre hilos y la prensa
pthread_mutex_t mutex1;                                                       //Mutex que se usa para modificar day
pthread_mutex_t mutex2;                                                       //Mutex que se usa para modificar la variable aunEnInlcusivoEjec
pthread_mutex_t mutex3;                                                       //Mutex que se usa para modificar la variable aunEnInlcusivoLegis
pthread_mutex_t mutex4;                                                       //Mutex que se usa para modificar la variable aunEnInlcusivoJud
pthread_mutex_t mutex5;                                                       //Mutex que se usa para modificar la variable aunEnInlcusivoArchivo
pthread_mutex_t mutexEjec;                                                    //Mutex que se usa por el poder ejecutivo y para evitar inconsistencia en Ejecutivo.acc
pthread_mutex_t mutexLegis;                                                   //Mutex que se usa para evitar inconsistencia en Legislativo.acc
pthread_mutex_t mutexJud;                                                     //Mutex que se usa para evitar inconsistencia en Judicial.acc
pthread_mutex_t mutexArchivo;                                                 //Mutex que se usa para evitar inconsistencia en los otros archivos que se utilicen 
pthread_t thread_id_ejec;                                                     //Id de Ejecutivo
pthread_t thread_id_legis;                                                    //Id de Legislativo                                                            
pthread_t thread_id_jud;                                                      //Id de Judicial


/*Funcion para hacer delay, la usamos para cuando hay que hacer esperas menores
a 1 segundo*/
void delay(int delay){
    for(int i=0; i<delay; i++)
    {}
}

/*Funcion utilizada para eliminar acciones de los archivos .acc*/
void deleteAccion(FILE* fp, char* newName, char* Accion){
    size_t len = 0;
    char* line;
    size_t read;
    FILE* f = fopen("temp.txt", "w");
    
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
    rename("temp.txt", newName);                                                      //Renombra el archivo temporal para aplicar los cambios al archivo original
}

/*Funcion utilizada para Desbloquear Mutex luego de que ya no se necesite el uso exclusivo de un archivo*/
void abrirMutex(char* ArchivoEx){
    if(strstr(ArchivoEx, "Legislativo.acc")){
        pthread_mutex_unlock(&mutexLegis);
    }
    else if(strstr(ArchivoEx, "Judicial.acc")){
        pthread_mutex_unlock(&mutexJud);
    }
    else if(strstr(ArchivoEx, "Ejecutivo.acc")){
        pthread_mutex_unlock(&mutexEjec);
    }
    else{
        pthread_mutex_unlock(&mutexArchivo);
    }
}

/*Funcion utilizada para Bloquear Mutex cuando una accion requiere uso exclusivo de un archivo*/
void cerrarMutex(char* ArchivoEx){
    if(strstr(ArchivoEx, "Legislativo.acc")){
        pthread_mutex_lock(&mutexLegis);
        strcpy(ArchivoEx, direccionLegis);
    }
    else if(strstr(ArchivoEx, "Judicial.acc")){
        pthread_mutex_lock(&mutexJud);
        strcpy(ArchivoEx, direccionJud);
    }
    else if(strstr(ArchivoEx, "Ejecutivo.acc")){
        pthread_mutex_lock(&mutexEjec);
        strcpy(ArchivoEx, direccionEjec);
    }
    else{
        pthread_mutex_lock(&mutexArchivo);
    }
}

/*Esta funcion contiene la instruccion de abrir de manera exclusiva un Archivo y leer/anular/Escribir. 
Es usada por los tres poderes*/
int Exclusivo(FILE* fp, char* ArchivoEx, char* line, size_t len){
    int cancel = FALSE;
    char* Decision = (char*)calloc(1, 200);

    cerrarMutex(ArchivoEx);                               //Bloquea el mutex requerido para usar el Arhcivo de manera exclusiva

    int lenght = ftell(fp);                               //Variable que contendra el numero de bytes desde el inicio del archivo exclusivo a la linea actual
    FILE* fExclusivo = fopen(ArchivoEx, "a+");            //Abre el archivo de forma Exclusiva y en modo Append y read (a+)
    fprintf(fExclusivo, "\n"); 

    while(getline(&line, &len, fp)!=-1){
        strcpy(Decision,strtok(line," "));
        //Si la instruccion nos pide leer el archivo y encontrar una linea
        if(strstr(Decision, "leer")){
            strcpy(Decision, strtok(NULL,"\0"));
            int leer = FALSE; 
            rewind(fExclusivo);

            while(getline(&line, &len, fExclusivo)!=-1){                                //Si encuentra una linea que concuerde, no se anula la accion
                if(strstr(line, Decision)){
                    leer = TRUE;
                    break;
                }
            }
            if(leer==FALSE){                                                            //Si no encuentra una linea que concuerde
                cancel = TRUE;
                fclose(fExclusivo);                                                     //Cierra el archivo
                abrirMutex(ArchivoEx);                                                  //Desbloquea el mutex que bloqueo al principio
                break;
            }
        }
        //Si la instruccion nos pide escribir en el archivo
        else if(strstr(Decision, "escribir")){
            strcpy(Decision, strtok(NULL,"\n"));
            fprintf(fExclusivo, "\n");                                                  //Escribe un newline
            fprintf(fExclusivo, "%s", Decision);                                        //Escribe la instruccion
        }
        //Si la instruccion nos pide leer el archivo y ver si no se encuentra una linea
        else if(strstr(Decision, "anular")){
            strcpy(Decision, strtok(NULL,"\0"));
            int anular = FALSE;
            rewind(fExclusivo);

            while(getline(&line, &len, fExclusivo)!=-1){                                //Si se encuentra una linea que concuerde, se anula la accion
                if(strstr(line, Decision)){
                    anular = TRUE;
                    break;
                }
            }
            if(anular==TRUE){ 
                cancel = TRUE;
                fclose(fExclusivo);                                                     //Se cierra el Archivo
                abrirMutex(ArchivoEx);                                                  //Desbloquea el mutex que bloqueo al principio
                break;
            }
        }
        //Si la instruccion ya no es escribir/leer/anular
        else{
            fseek(fp, lenght, SEEK_SET);                                                 //Mueve el apuntador fp a la linea anterior
            fclose(fExclusivo);                                                          //Cierra el archivo
            abrirMutex(ArchivoEx);                                                       //Desbloquea el mutex que bloqueo al principio
            break;
        }
        lenght = ftell(fp);                                                              //Obtiene el valor de la posicion (en bytes)
    } 
    free(Decision);
    return cancel;                                                                       //Regresa si la accion debe cancelarse o no.
}

/*Esta funcion contiene la instruccion de abrir de manera inclusiva un Archivo y leer/anular/Escribir. 
Es usada por los tres poderes*/
int Inclusivo(FILE* fp, char* ArchivoIn, char* line, size_t len){
    int cancel = FALSE;
    char* Decision = (char*)calloc(1, 200);

    if(strstr(ArchivoIn, "Ejecutivo")){                  //Si se abrira Ejecutivo de manera inclusiva, aumentamos el contador 
        pthread_mutex_lock(&mutex2);                     //Bloqueamos el mutex2 para evitar inconsistencia en la variable aunEnInclusivoEjec
        aunEnInclusivoEjec++;
        strcpy(ArchivoIn, direccionEjec);
        if(aunEnInclusivoEjec==1){
            pthread_mutex_lock(&mutexEjec);              //Si es la primera persona abriendolo, se bloquea para que Exclusivo no pueda entrar
        }
        pthread_mutex_unlock(&mutex2);                   //Desbloqueamos mutex2 ya que hicimos las modificaciones
    }
    else if(strstr(ArchivoIn, "Legislativo")){           //Si se abrira Legislativo de manera inclusiva, aumentamos el contador 
        pthread_mutex_lock(&mutex3);                     //Bloqueamos el mutex3 para evitar inconsistencia en la variable aunEnInclusivoLegis
        aunEnInclusivoLegis++;
        strcpy(ArchivoIn, direccionLegis);
        if(aunEnInclusivoLegis==1){
            pthread_mutex_lock(&mutexLegis);             //Si es la primera persona abriendolo, se bloquea para que Exclusivo no pueda entrar
        }
        pthread_mutex_unlock(&mutex3);                   //Desbloqueamos mutex3 ya que hicimos las modificaciones
    }
    else if(strstr(ArchivoIn, "Judicial")){              //Si se abrira Judicial de manera inclusiva, aumentamos el contador 
        pthread_mutex_lock(&mutex4);                     //Bloqueamos el mutex4 para evitar inconsistencia en la variable aunEnInclusivoJud
        aunEnInclusivoJud++;
        strcpy(ArchivoIn, direccionJud);
        if(aunEnInclusivoJud==1){
            pthread_mutex_lock(&mutexJud);               //Si es la primera persona abriendolo, se bloquea para que Exclusivo no pueda entrar
        }
        pthread_mutex_unlock(&mutex4);                   //Desbloqueamos mutex4 ya que hicimos las modificaciones
    }
    else{                                                //Si se abrira otro Archivo de manera inclusiva, aumentamos el contador 
        pthread_mutex_lock(&mutex5);                     //Bloqueamos el mutex5 para evitar inconsistencia en la variable aunEnInclusivoArchivo
        aunEnInclusivoArchivo++;
        if(aunEnInclusivoArchivo==1){
            pthread_mutex_lock(&mutexArchivo);           //Si es la primera persona abriendolo, se bloquea para que Exclusivo no pueda entrar
        }
        pthread_mutex_unlock(&mutex5);                   //Desbloqueamos mutex5 ya que hicimos las modificaciones
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

            while(getline(&line, &len, fInclusivo)!=-1){                                //Si encuentra una linea que concuerde, no se anula la accion
                if(strstr(line, Decision)){
                    leer = TRUE;
                    break;
                }
            }
            if(leer==FALSE){                                                            //Si no encuentra una linea que concuerde
                cancel = TRUE;                                   
                fclose(fInclusivo);                                                     //Se cierra el archivo 
                break;
            }
        }
        //Si la instruccion nos pide escribir en el archivo
        else if(strstr(Decision, "escribir")){
            strcpy(Decision, strtok(NULL,"\n"));
            fprintf(fInclusivo, "\n");                                                  //Escribe un newline
            fprintf(fInclusivo, "%s", Decision);                                        //Escribe la instruccion
            fclose(fInclusivo);                                                         //Guardamos los cambios de la nueva línea agregada
        }
        //Si la instruccion nos pide leer el archivo y ver si no se encuentra una linea
        else if(strstr(Decision, "anular")){
            strcpy(Decision, strtok(NULL,"\0"));
            int anular = FALSE;
            rewind(fInclusivo);

            while(getline(&line, &len, fInclusivo)!=-1){                                //Si se encuentra una linea que concuerde, se anula la accion
                if(strstr(line, Decision)){
                    anular = TRUE;
                    break;
                }
            }
            if(anular==TRUE){
                cancel = TRUE;
                fclose(fInclusivo);                                                     //Se cierra el archivo
                break;
            }
        }
        //Si la instruccion ya no es escribir/leer/anular
        else{
            fseek(fp, lenght, SEEK_SET);                                                 //Mueve el apuntador fp a la linea anterior
            fclose(fInclusivo);                                                          //Se cierra el archivo
            break;
        }
        lenght = ftell(fp);                                                              //Obtiene el valor de la posicion (en bytes)
    }

    if(strstr(ArchivoIn, "Ejecutivo")){                  //Cuando salga de Ejecutivo.acc, se decrementa la variable
        pthread_mutex_lock(&mutex2);                     //Bloqueamos el mutex2 para evitar inconsistencia en la variable aunEnInclusivoEjec 
        aunEnInclusivoEjec--;
        if(aunEnInclusivoEjec==0){
            pthread_mutex_unlock(&mutexEjec);            //Si es la ultima persona cerrandolo, se desbloquea el mutex para que otros puedan usar el Archivo
        }
        pthread_mutex_unlock(&mutex2);                   //Desbloqueamos mutex2 ya que hicimos las modificaciones
    }
    else if(strstr(ArchivoIn, "Legislativo")){           //Cuando salga de Legislativo.acc, se decrementa la variable
        pthread_mutex_lock(&mutex3);                     //Bloqueamos el mutex3 para evitar inconsistencia en la variable aunEnInclusivoLegis 
        aunEnInclusivoLegis--;
        if(aunEnInclusivoLegis==0){
            pthread_mutex_unlock(&mutexLegis);           //Si es la ultima persona cerrandolo, se desbloquea el mutex para que otros puedan usar el Archivo
        }
        pthread_mutex_unlock(&mutex3);                   //Desbloqueamos mutex3 ya que hicimos las modificaciones
    }
    else if(strstr(ArchivoIn, "Judicial")){              //Cuando salga de Judicial.acc, se decrementa la variable
        pthread_mutex_lock(&mutex4);                     //Bloqueamos el mutex4 para evitar inconsistencia en la variable aunEnInclusivoJud 
        aunEnInclusivoJud--;
        if(aunEnInclusivoJud==0){
            pthread_mutex_unlock(&mutexJud);             //Si es la ultima persona cerrandolo, se desbloquea el mutex para que otros puedan usar el Archivo
        }
        pthread_mutex_unlock(&mutex4);                   //Desbloqueamos mutex4 ya que hicimos las modificaciones
    }
    else{                                                //Cuando salga de otro Archivo, se decrementa la variable
        pthread_mutex_lock(&mutex5);                     //Bloqueamos el mutex5 para evitar inconsistencia en la variable aunEnInclusivoArchivo 
        aunEnInclusivoArchivo--;
        if(aunEnInclusivoArchivo==0){
            pthread_mutex_unlock(&mutexArchivo);         //Si es la ultima persona cerrandolo, se desbloquea el mutex para que otros puedan usar el Archivo
        }
        pthread_mutex_unlock(&mutex5);                   //Desbloqueamos mutex5 ya que hicimos las modificaciones
    }
    free(Decision);
    return cancel;                                       //Retorna si la accion debe cancelarse o no
}

void destituirMagistrado(){
    printf("%s\n","Destituirmagistrado fue invocado");
   // pthread_mutex_lock(&mutex3);                     //Bloqueamos el mutex3 para evitar inconsistencia en la variable aunEnInclusivoLegis
    aunEnInclusivoLegis++;
    if(aunEnInclusivoLegis==1){
  //      pthread_mutex_lock(&mutexLegis);             //Si es la primera persona abriendolo, se bloquea para que Exclusivo no pueda entrar
    }
  //  pthread_mutex_unlock(&mutex3);                   //Desbloqueamos mutex3 ya que hicimos las modificaciones

    FILE* fInclusivo = fopen(direccionLegis, "a+");        // Aquí siempre vamos abrir el Legislativo.acc
    fprintf(fInclusivo, "\n"); 

    printf("%s\n", "luego de invocar destitucionMagistrado(). Se va a escribir en legislativo para destituirlo");
    fprintf(fInclusivo, "\n");                                                  //Escribe un newline
    fprintf(fInclusivo, "%s", "Destituir Magistrado");                           //Escribe la instruccion de Destituir Magistrado

    fclose(fInclusivo);

   // pthread_mutex_lock(&mutex3);                     //Bloqueamos el mutex3 para evitar inconsistencia en la variable aunEnInclusivoLegis 
    aunEnInclusivoLegis--;
    if(aunEnInclusivoLegis==0){
   //     pthread_mutex_unlock(&mutexLegis);           //Si es la ultima persona cerrandolo, se desbloquea el mutex para que otros puedan usar el Archivo
    }
  //  pthread_mutex_unlock(&mutex3);                   //Desbloqueamos mutex3 ya que hicimos las modificaciones
}

/*Funcion Thread del Ejecutivo*/
void *threadEjec(void *vargp) 
{ 
    srand(time(0));                                                       //Seed del rand()
    size_t len = 0;
    char* line;                                                           
    int Encontro;                                                         //Usado para ver cuando se encontro una accion
    int vacio;                                                            //Entero que señalara si el archivo Ejecutivo.acc ya no tiene acciones
    char toPrensa[200];                                                   //Mensaje que se envia a la prensa
    char* Decision = (char*)calloc(1, 200);                               //Se usa para revisar que instruccion en la accion es
    char* nombreAccion = (char*)calloc(1, 200);                           //Variable que contendra el nombre de la accion actual
    int exito;                                                            //Variable para evaluar si la accion fue exitosa o no
    int cancel;                                                           //Se usara para cancelar la ejecucion de una accion
    FILE* fp;                                                             

    while(day-1<daysMax){                                               
        pthread_mutex_lock(&mutexEjec);                                   //Se bloquea el mutex ya que no se podra usar Ejecutivo.acc mientras ejecute una accion
                                                                          
        pthread_mutex_lock(&mutex1);
        day++;                                                            //Se aumenta el dia
        pthread_mutex_unlock(&mutex1);

        cancel = FALSE;                                                   //
        vacio = TRUE;                                                     //Inicializa las variables
        Encontro = FALSE;                                                 //
        fp = fopen(direccionEjec, "r");

        //Encuentra una accion, con 20% de probabilidad
        while(TRUE){
            while(getline(&line, &len, fp) != -1){
                if(strlen(line) > 2 && strchr(line, ':') == NULL){        //Mientras la linea no sea un espacio, no contenga el caracter :
                    vacio = FALSE;
                    if(rand()%5 == 1){                                    //y con una posibilidad de 20% de ser escogido
                        Encontro=TRUE;
                        strcpy(nombreAccion, line);
                        break;
                    }                                                                       
                }
            }
            if(vacio==TRUE){
                pthread_mutex_lock(&mutex1);                              //Se bloquea el mutex para evitar inconsistencia en la variable day y aunTieneAcciones
                day--;                                                    //Se decrementa el dia ya que no es encontro accion
                aunTieneAcciones--;                                       //Como Ejecutivo.acc ya no tiene acciones, se decrementa la variable
                pthread_mutex_unlock(&mutex1);                            //Se desbloquea el mutex
                free(Decision);                                           //Liberamos memoria de la variable
                free(nombreAccion);                                       //Liberamos memoria de la variable
                pthread_mutex_unlock(&mutexEjec);                         //Se desbloquea el mutex que se bloqueo al entrar a buscar acciones
                pthread_exit(NULL);
            }
            if(!Encontro){
                rewind(fp);                                               //Si no se decidio por una accion, el apuntador vuelve al inicio del archivo
            }
            else{
                break;                                                    //!!!!!!!!!!!!!!!!!!!!!!Tengo que ver porque puse esto aqui!!!!!!!!!!!!!!!!!!!!!!!!!!
            }
        }
        //Ahora ejecuta la accion
        while(getline(&line, &len, fp)!=-1 && strlen(line)>2){
            strcpy(Decision,strtok(line," "));
            //La accion requiere aprobacion de otro poder
            if((strstr(Decision, "aprobacion") || strstr(Decision, "reprobacion")) && cancel==FALSE){
                char* to_who = (char*)malloc(200);                                                  //Quien debera recibir la señal para que apruebe/repruebe
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
                    pthread_mutex_unlock(&mutexApro);                                               //Manda una señal a el hilo que aprueba
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
                    pthread_mutex_unlock(&mutexApro);                                               //Manda una señal a el hilo que aprueba
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
                free(to_who);                                                                       //Se libera el espacio de memoria de la variable
            }
            //La accion pide usar un archivo de forma inclusiva
            else if(strstr(Decision, "inclusivo") && cancel==FALSE){
                char* ArchivoIn = (char*)calloc(1, 200);                                     //Nombre del archivo a ser abierto de manera inclusiva
                strcpy(ArchivoIn, strtok(NULL,"\n"));

                cancel = Inclusivo(fp, ArchivoIn, line, len);                                //Llama a la funcion que se encarga de esta instruccion

                free(ArchivoIn);                                                             //Libera el espacio de memoria de la variable
            }
            //La accion pide usar un archivo de forma exclusiva
            else if(strstr(Decision, "exclusivo") && cancel==FALSE){
                char* ArchivoEx = (char*)calloc(1, 200);                                     //Nombre del archivo a ser abierto de manera exclusiva
                strcpy(ArchivoEx, strtok(NULL,"\n"));

                cancel = Exclusivo(fp, ArchivoEx, line, len);                                //Llama a la funcion que se encarga de esta instruccion

                free(ArchivoEx);                                                             //Liberamos memoria de la variable
            }
            else if(strstr(Decision, "exito") && (rand()%101<=PorcentajeExitoEjec) && cancel==FALSE){        //Si la accion es exitosa
                exito=TRUE;                        
                break;
            }
            else if(strstr(Decision, "fracaso")){                                                            //Si la accion fracaza
                exito=FALSE;
                break;
            }
        }
        strcpy(Decision, strtok(NULL,"\0"));                                  //Toma el resto de el mensaje
        pthread_mutex_lock(&mutex);                                           //Para evitar inconsistencia en el pipe, se debe tratar como Seccion Critica
        if(exito==TRUE){                                                      //Si la accion es exitosa, se elimina de el archivo
            rewind(fp);
            deleteAccion(fp, direccionEjec, nombreAccion);
        }
        strcpy(toPrensa, "Presidente ");
        strcat(toPrensa, Decision);
        write(fd[1], toPrensa, sizeof(toPrensa));                             //Se escribe la el mensaje de exito/fracaso al pipe que conecta este hilo con la prensa
        pthread_mutex_unlock(&mutex);                                         //Se libera el mutex

        fclose(fp);                                                           //Cierra el archivo para abrirlo nuevamente cuando se reinicie el ciclo
        pthread_mutex_unlock(&mutexEjec);                                     //Se desbloquea para poder hacer cambios a Ejecutivo.acc o hacer aprobaciones
        delay(10000);                                                         //Hace delay para que le de chance a otros hilos de avanzar
    } 
    free(Decision);                                                           //Libera el espacio en memoria de la variable
    free(nombreAccion);                                                       //Libera el espacio en memoria de la variable
    pthread_exit(NULL);                                                       //Termina la ejecucion del Hilo
}


void *threadLegis(void *vargp) 
{ 
    size_t len = 0;
    char* line;
    int Encontro;                                                                 //Usado para ver cuando se encontro una accion
    int vacio;                                                                    //Entero que señalara si el archivo Legislativo.acc ya no tiene acciones
    char toPrensa[200];                                                           //Mensaje que se envia a la prensa
    char* Decision = (char*)calloc(1, 200);                                       //Se usa para revisar que que decision en la accion es
    char* nombreAccion = (char*)calloc(1, 200);                                   //Variable que contendra el nombre de la accion actual
    int exito;                                                                    //Variable para evaluar si la accion fue exitosa o no
    int cancel;                                                                   //Se usara para cancelar la ejecucion de una accion
    srand(time(0));                                                               //Seed del rand()
    FILE* fp;

    while(day-1<daysMax){
        pthread_mutex_lock(&mutexLegis);                                          //Se bloquea el mutex ya que no se podra usar Legislativo.acc mientras ejecute una accion

        pthread_mutex_lock(&mutex1);
        day++;                                                                    //Se aumenta el dia
        pthread_mutex_unlock(&mutex1);

        cancel = FALSE;                                                           //
        vacio = TRUE;                                                             //Inicializa las variables
        Encontro = FALSE;                                                         //
        fp = fopen(direccionLegis, "r");

        //Encuentra una accion, con 20% de probabilidad
        while(TRUE){
            while(getline(&line, &len, fp) != -1){
                if(strlen(line) > 2 && strchr(line, ':') == NULL){                //Mientras la linea no sea un espacio, no contenga el caracter :
                    vacio = FALSE;
                    if(rand()%5 == 1){                                            //y con una posibilidad de 20% de ser escogido
                        Encontro=TRUE;
                        strcpy(nombreAccion, line);
                        break;
                    }                                                                                             
                }
            }
            if(vacio==TRUE){
                pthread_mutex_lock(&mutex1);                                      //Bloqueamos el mutex para evitar inconsistecia en day y aunTieneAcciones
                day--;                                                            //Se decrementa el dia ya que no es encontro accion
                aunTieneAcciones--;                                               //Como ejecutivo ya no tiene acciones, se decrementa la variable
                pthread_mutex_unlock(&mutex1);                                    //Desbloqueamos el mutex
                free(Decision);                                                   //Liberamos memoria de la variable
                free(nombreAccion);                                               //Liberamos memoria de la variable
                pthread_mutex_unlock(&mutexLegis);                                //Se desbloquea el mutex que se bloqueo al entrar a buscar acciones
                pthread_exit(NULL);
            }
            if(!Encontro){
                rewind(fp);                                                       //Si no se decidio por una accion, el apuntador vuelve al inicio del archivo
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
                char* to_who = (char*)malloc(200);                                                  //Quien debera recibir la señal para que apruebe/repruebe
                strcpy(to_who, strtok(NULL,"\0"));
                int num;                                                                            //variable que dira si se aprobo/reprobo la accion
                //Tribunal Supremo aprueba
                if(strstr(to_who, "Tribunal Supremo")){  
                    int tot=0;
                    int i=0;
                    while(Magistrados[i]!=0){                                                       //Se calcula la media aritmetica de los magistrados
                        tot+=Magistrados[i];
                        i++;
                    }                                                                           
                    pthread_mutex_unlock(&mutexApro);                                               //Manda una señal a el hilo que aprueba
                    read(fdApro[0], &num, sizeof(num));
                    //Si requiere aprobacion

                    if(strstr(Decision, "aprobacion") && (num > tot/numMagistrados)){               //Si no se aprueba, se cancela la accion
                        cancel=TRUE;
                        destituirMagistrado();
                    }
                    //Si puede ser reprobado
                    else if(strstr(Decision, "reprobacion") && (num <= tot/numMagistrados)){        //Si hubo reprobacion, se cancela la accion
                        cancel=TRUE;
                        destituirMagistrado();
                    }
                }
                //Presidente/Magistrados aprueban
                else{
                    pthread_mutex_unlock(&mutexAproEjec);                                           //Manda una señal a el hilo que aprueba
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
                free(to_who);                                                                       //Liberamos espacio en memoria de la variable
            }
            else if(strstr(Decision, "inclusivo") && cancel==FALSE){
                char* ArchivoIn = (char*)calloc(1, 200);                                //Nombre de el Archivo a abrir de manera inclusiva
                strcpy(ArchivoIn, strtok(NULL,"\n"));

                cancel = Inclusivo(fp, ArchivoIn, line, len);                           //Llama a la funcion que se encarga de esta instruccion

                free(ArchivoIn);                                                        //Libera el espacio en memoria de la variable
            }
            else if(strstr(Decision, "exclusivo") && cancel==FALSE){
                char* ArchivoEx = (char*)calloc(1, 200);                                //Nombre de el Archivo a abrir de manera exclusiva
                strcpy(ArchivoEx, strtok(NULL,"\n")); 
 
                cancel = Exclusivo(fp, ArchivoEx, line, len);                           //Llama a la funcion que se encarga de esta instruccion

                free(ArchivoEx);                                                        //Libera el espacio en memoria de la variable
            }
            else if(strstr(Decision, "exito") && (rand()%101<=PorcentajeExitoLegis) && cancel==FALSE){    //Si la accion es exitosa
                exito=TRUE; 
                break;
            }
            else if(strstr(Decision, "fracaso")){                                                         //Si la accion fracaza
                exito=FALSE;
                break;
            }
        }
        strcpy(Decision, strtok(NULL,"\0"));                                             //Toma el resto de el mensaje                          
        pthread_mutex_lock(&mutex);                                                      //Para evitar inconsistencia en el pipe, se debe tratar como Seccion Critica
        if(exito==TRUE){                                                                 //Si la accion es exitosa, se elimina de el archivo
            rewind(fp);
            deleteAccion(fp, direccionLegis, nombreAccion);
        }
        strcpy(toPrensa, "Congreso ");
        strcat(toPrensa, Decision);                                                      
        write(fd[1], toPrensa, sizeof(toPrensa));                                        //Se escribe la el mensaje de exito/fracaso al pipe que conecta este hilo con la prensa
        pthread_mutex_unlock(&mutex);                                                    //Se libera el mutex

        fclose(fp);                                                                      //Cierra el archivo Legislativo.acc
        pthread_mutex_unlock(&mutexLegis);                                               //Desbloquea el mutex para que otros hilos usen Legislativo.acc
        delay(10000);                                                                    //Hace delay para que le de chance a otros de poder usar Legislativo.acc
    } 
    free(Decision);                                                                      //Libera el espacio en memoria de la variable
    free(nombreAccion);                                                                  //Libera el espacio en memoria de la variable
    pthread_exit(NULL);                                                                  //Termina la ejecucion del hilo
}


void *threadJud(void *vargp) 
{ 
    size_t len = 0;
    char* line;
    int Encontro;                                                             //Usado para ver cuando se encontro una accion
    int vacio;                                                                //Entero que señalara si el archivo Judicial.acc ya no tiene acciones
    char toPrensa[200];                                                       //Mensaje que se envia a la prensa
    char* Decision = (char*)calloc(1, 200);                                   //Se usa para revisar que que decision en la accion es 
    char* nombreAccion = (char*)calloc(1, 200);                               //Variable que contendra el nombre de la accion actual
    int exito;                                                                //Variable para evaluar si la accion fue exitosa o no
    int cancel;                                                               //Se usara para cancelar la ejecucion de una accion
    srand(time(0));                                                           //Seed del rand()
    FILE* fp;                                                                 

    while(day-1<daysMax){ 
        pthread_mutex_lock(&mutexJud);                                        //Se bloquea el mutex ya que no se podra usar Judicial.acc mientras ejecute una accion

        pthread_mutex_lock(&mutex1); 
        day++;                                                                //Se aumenta el dia
        pthread_mutex_unlock(&mutex1);

        cancel = FALSE;                                                       //
        Encontro = FALSE;                                                     //Inicializa variables
        vacio = TRUE;                                                         //
        fp = fopen(direccionJud, "r");

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
                pthread_mutex_lock(&mutex1);                                  //Bloquea el mutex para evitar inconsistencias al modificar la variable
                day--;                                                        //Se decrementa el dia ya que no es encontro accion
                aunTieneAcciones--;                                           //Como ejecutivo ya no tiene acciones, se decrementa la variable
                pthread_mutex_unlock(&mutex1);                                //Desbloquea el mutex
                free(Decision);                                               //Liberamos memoria de la variable
                free(nombreAccion);                                           //Liberamos memoria de la variable
                pthread_mutex_unlock(&mutexJud);                              //Se desbloquea el mutex que se bloqueo al entrar a buscar acciones
                pthread_exit(NULL);
            }
            if(!Encontro){
                rewind(fp);                                                   //Si no se decidio por una accion, el apuntador vuelve al inicio del archivo
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
                free(to_who);                                                                        //Liberamos memoria de la variable
            }
            else if(strstr(Decision, "inclusivo") && cancel==FALSE){
                char* ArchivoIn = (char*)calloc(1, 200);                                //Nombre del archivo a abrir de manera inclusiva
                strcpy(ArchivoIn, strtok(NULL,"\n"));

                cancel = Inclusivo(fp, ArchivoIn, line, len);                           //Se llama a la funcion que se encarga de esta instruccion

                free(ArchivoIn);                                                        //Libera el espacio en memoria de la variable
            }
            else if(strstr(Decision, "exclusivo") && cancel==FALSE){
                char* ArchivoEx = (char*)calloc(1, 200);                                //Nombre del archivo a abrir de manera exclusiva
                strcpy(ArchivoEx, strtok(NULL,"\n"));

                cancel = Exclusivo(fp, ArchivoEx, line, len);                           //Se llama a la funcion que se encarga de esta instruccion

                free(ArchivoEx);                                                        //Libera el espacio en memoria de la variable
            }
            else if(strstr(Decision, "exito") && cancel==FALSE){                        //Si la accion es exitosa  
                int tot=0;
                int i=0;
                while(Magistrados[i]!=0){                                               //Se calcula la media aritmetica de los Magistrados
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
            deleteAccion(fp, direccionJud, nombreAccion);
        }
        strcpy(toPrensa, "Tribunal Supremo ");
        strcat(toPrensa, Decision);
        write(fd[1], toPrensa, sizeof(toPrensa));                                       //Se escribe la el mensaje de exito/fracaso al pipe que conecta este hilo con la prensa
        pthread_mutex_unlock(&mutex);                                                   //Se libera el mutex

        fclose(fp);                                                                     //Cierra el archivo Judicial.acc
        pthread_mutex_unlock(&mutexJud);                                                //Desbloquea el mutex para que otros hilos usen Judicial.acc
        delay(10000);                                                                   //Hace delay para que otros usen el archivo Judicial.acc
    }
    free(Decision);                                                                     //Libera el espacio en memoria de la variable
    free(nombreAccion);                                                                 //Libera el espacio en memoria de la variable
    pthread_exit(NULL);                                                                 //Termina la ejecucion del hilo
}

/*Este hilo se usara para las aprobaciones por parte del Ejecutivo. Hara aprobaciones cuando el Presidente este libre 
(No este ejecutando una accion)*/
void *aprobacionEjec(void *vargp)
{   
    int num;
    srand(time(0));
    while(TRUE){
        pthread_mutex_lock(&mutexAproEjec);                                            //Cuando le dan la señal, el envia la respuesta

        pthread_mutex_lock(&mutexEjec);                                                //Espera a que el Presidente este libre
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
    char Hemeroteca[daysMax][200];                                                    //Array de Strings que tendra todas las acciones imprimidas
    char toPrensa[200];                                                               //Variable para imprimir la accion
    int print=1;                                                                      //Variable para saber que dia se hizo la accion

    for(int i=0; i<daysMax; i++){                                                     //Limpia el array
        memset(Hemeroteca[i], 0, sizeof(Hemeroteca[i]));
    }
    while(print-1!=daysMax){                                                          //Mientras aun queden dias
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
    close(fd[0]);                                                                     //Cuando termina, cierra el lado Read del pipe
    close(fd[1]);                                                                     //Tambien cierra el lado write del pipe
    pthread_exit(NULL);                                                               //Termina la ejecucion del hilo
}

void main(int argc, char *argv[]){

    if(argc < 2){
        printf("Faltan argumentos\n");
        exit(1);
    }

    memset(Magistrados, 0, sizeof(Magistrados));
    for(int i=0; i<8; i++){                                                           //Inicializa los 8 magistrados
        Magistrados[i] = 66;
    }

    daysMax = atoi(argv[1]); 
    if(daysMax<=0){      
        exit(1);
    }

    direccionEjec = (char*)calloc(1, 200);
    direccionLegis = (char*)calloc(1, 200);
    direccionJud = (char*)calloc(1, 200);
    if(argc>=3){
        strcpy(direccionEjec, (char*)argv[2]);
        strcpy(direccionLegis, (char*)argv[2]);
        strcpy(direccionJud, (char*)argv[2]);
        strcat(direccionEjec, "Ejecutivo.acc");
        strcat(direccionLegis, "Legislativo.acc");
        strcat(direccionJud, "Judicial.acc");
    }
    else{
        strcpy(direccionEjec, "Ejecutivo.acc");
        strcpy(direccionLegis, "Legislativo.acc");
        strcpy(direccionJud, "Judicial.acc");
    }

    pthread_t thread_aprobacion;                                                      //hilo que se encarga de la aprobacion de Judicial y Legislativo
    pthread_create(&thread_aprobacion, NULL, aprobacion, NULL);
    pthread_mutex_lock(&mutexApro);

    pthread_t thread_id_prensa;
    if(pipe(fd)<0){                                                                   //Inicializa el pipe fd
        perror("pipe ");                                              
        exit(1);
    }
    if(pipe(fdApro)<0){                                                               //Inicializa el pipe fdApro
        perror("pipe ");                                              
        exit(1);
    }
    if(pipe(fdAproEjec)<0){                                                           //Inicializa el pipe fdAproEjec
        perror("pipe ");                                              
        exit(1);
    }

    pthread_create(&thread_id_ejec, NULL, threadEjec, NULL);                          //
    pthread_create(&thread_id_legis, NULL, threadLegis, NULL);                        //Crea los hilos
    pthread_create(&thread_id_jud, NULL, threadJud, NULL);                            //
    pthread_create(&thread_id_prensa, NULL, threadPrensa, NULL);                      //

    pthread_join(thread_id_ejec, NULL);                                               //
    pthread_join(thread_id_legis, NULL);                                              // Espera que los hilos terminen de ejecutar
    pthread_join(thread_id_jud, NULL);                                                //
    if(aunTieneAcciones<=0){
        sleep(1);                                                                     
        pthread_cancel(thread_id_prensa);                                             //Si los poderes se quedaron sin acciones antes de terminar los dias, se cancela a la Prensa
        pthread_cancel(thread_aprobacion);                                            //Y a los hilos de aprobaciones
    }
    pthread_join(thread_id_prensa, NULL);   
    free(direccionEjec);     
    free(direccionLegis); 
    free(direccionJud);                                      
}