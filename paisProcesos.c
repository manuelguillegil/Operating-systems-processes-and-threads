#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <pthread.h> 
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <errno.h>
 
#define TRUE 1
#define FALSE 0

int fd[2];                                                                            //Pipe entre subprocesos y prensa
int fd1[2];                                                                           //Pipe entre subprocesos para compartir variable day
int day=0;                                                                            //Inicializa la variable day que cuenta en que dia esta
int daysMax;                                                                          //int que indica cual es el dia maximo 
int aunTieneAcciones = 3;  

void deleteAccion(FILE* fp, char* newName, char* Accion, char* temp){
    size_t len = 0;
    char* line;
    FILE* f = fopen(temp, "w");
    
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
    rename(temp, newName);                                                            //Renombra el archivo temporal para aplicar los cambios al archivo original
}

void Ejecutivo(sem_t *sem) 
{   
    size_t len = 0;
    char* line;                                                           
    int Encontro;                                                         //Usado para ver cuando se encontro una accion
    int vacio;
    char toPrensa[200];                                                   //Mensaje que se envia a la prensa
    char* Decision = (char*)calloc(1, 200);                               //Se usa para revisar que que decision en la accion es
    char* nombreAccion = (char*)calloc(1, 200);
    int exito;                                                            //Variable para evaluar si la accion fue exitosa o no
    srand(time(0)); 
    FILE* fp;                                                             

    while(TRUE){
        read(fd1[0], &day, sizeof(day));                                  //Se lee el valor actualizado de day
        day++;                                                            //Se aumenta el dia
        write(fd1[1], &day, sizeof(day));                                 //Se actualiza el valor de day para todos los subprocesos
        if(day-1>=daysMax){                                               //Si se completaron los dias de ejecucion, se para el subproceso
            return;
        }

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
            if(vacio==TRUE){                                                      //Si el archivo de entrada esta vacio
                read(fd1[0], &day, sizeof(day));
                day--;                                                            //Se decrementa el dia ya que no es encontro accion
                write(fd1[1], &day, sizeof(day));
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
            if(strstr(Decision, "aprobacion")){

            }
            else if(strstr(Decision, "inclusivo")){

            }
            else if(strstr(Decision, "exclusivo")){

            }
            else if(strstr(Decision, "exito") && rand()%2 == 1){                          //Si la accion es exitosa
                exito=TRUE;                        
                break;
            }
            else if(strstr(Decision, "fracaso") && rand()%2 == 1){                        //Si la accion fracaza
                exito=FALSE;
                break;
            }
        }
        strcpy(Decision, strtok(NULL,"\0"));                                             //Toma el resto de el mensaje  

        sem_wait(sem);                                            
        if(exito==TRUE){                                                                 //Si la accion es exitosa, se elimina de el archivo
            rewind(fp);
            deleteAccion(fp, "Ejecutivo.acc", nombreAccion, "temp1.txt");
        }
        strcpy(toPrensa, "Presidente ");
        strcat(toPrensa, Decision);
        write(fd[1], toPrensa, sizeof(toPrensa));                                         //Se escribe la el mensaje de exito/fracaso al pipe que conecta este subproceso con la prensa
        sem_post(sem);

        fclose(fp);                                                                       //Cierra el archivo para abrirlo nuevamente cuando se reinicie el ciclo
    } 
    return;
}


void Legislativo(sem_t *sem) 
{   
    size_t len = 0;
    char* line;
    int Encontro;                                                                     //Usado para ver cuando se encontro una accion
    int vacio;
    char toPrensa[200];                                                               //Mensaje que se envia a la prensa
    char* Decision = (char*)calloc(1, 200);                                           //Se usa para revisar que que decision en la accion es
    char* nombreAccion = (char*)calloc(1, 200);
    int exito;                                                                        //Variable para evaluar si la accion fue exitosa o no
    srand(time(0));
    FILE* fp;

    while(TRUE){
        read(fd1[0], &day, sizeof(day));                                              //Se lee el valor actualizado de day
        day++;                                                                        //Se aumenta el dia
        write(fd1[1], &day, sizeof(day));                                             //Se actualiza el valor de day para todos los subprocesos
        if(day-1>=daysMax){                                                           //Si se completaron los dias de ejecucion, se para el subproceso
            return;
        }

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
            if(vacio==TRUE){                                                      //Si el archivo de entrada esta vacio
                read(fd1[0], &day, sizeof(day));
                day--;                                                            //Se decrementa el dia ya que no es encontro accion
                write(fd1[1], &day, sizeof(day));
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
            if(strstr(Decision, "aprobacion")){

            }
            else if(strstr(Decision, "inclusivo")){

            }
            else if(strstr(Decision, "exclusivo")){ 

            }
            else if(strstr(Decision, "exito") && rand()%2 == 1){                          //Si la accion es exitosa
                exito=TRUE; 
                break;
            }
            else if(strstr(Decision, "fracaso") && rand()%2 == 1){                        //Si la accion fracaza
                exito=FALSE;
                break;
            }
        }
        strcpy(Decision, strtok(NULL,"\0"));                                              //Toma el resto de el mensaje 

        sem_wait(sem);                       
        if(exito==TRUE){                                                                  //Si la accion es exitosa, se elimina de el archivo
            rewind(fp);
            deleteAccion(fp, "Legislativo.acc", nombreAccion, "temp2.txt");
        }
        strcpy(toPrensa, "Congreso ");
        strcat(toPrensa, Decision);
        write(fd[1], toPrensa, sizeof(toPrensa));                                         //Se escribe la el mensaje de exito/fracaso al pipe que conecta este subproceso con la prensa
        sem_post(sem); 

        fclose(fp);
    }
    return;
}


void Judicial(sem_t *sem) 
{   
    size_t len = 0;
    char* line;
    int Encontro;                                                             //Usado para ver cuando se encontro una accion
    int vacio;
    char toPrensa[200];                                                       //Mensaje que se envia a la prensa
    char* Decision = (char*)calloc(1, 200);                                   //Se usa para revisar que que decision en la accion es 
    char* nombreAccion = (char*)calloc(1, 200);   
    int exito;                                                                //Variable para evaluar si la accion fue exitosa o no
    srand(time(0));
    FILE* fp;

    while(TRUE){ 
        read(fd1[0], &day, sizeof(day));                                      //Se lee el valor actualizado de day
        day++;                                                                //Se aumenta el dia
        write(fd1[1], &day, sizeof(day));                                     //Se actualiza el valor de day para todos los subprocesos
        if(day-1>=daysMax){                                                   //Si se completaron los dias de ejecucion, se para el subproceso
            return;
        }

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
                read(fd1[0], &day, sizeof(day));
                day--;                                                        //Se decrementa el dia ya que no es encontro accion
                write(fd1[1], &day, sizeof(day));
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
            if(strstr(Decision, "aprobacion")){

            }
            else if(strstr(Decision, "reprobacion")){

            }
            else if(strstr(Decision, "inclusivo")){

            }
            else if(strstr(Decision, "exclusivo")){

            }
            else if(strstr(Decision, "exito") && rand()%2 == 1){                        //Si la accion es exitosa  
                exito=TRUE;                                             
                break;
            }
            else if(strstr(Decision, "fracaso") && rand()%2 == 1){                      //Si la accion fracaza
                exito=FALSE;
                break;
            }
        }
        strcpy(Decision, strtok(NULL,"\0"));                                         //Toma el resto de el mensaje 

        sem_wait(sem);               
        if(exito==TRUE){                                                                //Si la accion es exitosa, se elimina de el archivo
            rewind(fp);
            deleteAccion(fp, "Judicial.acc", nombreAccion, "temp3.txt");
        }
        strcpy(toPrensa, "Tribunal Supremo ");
        strcat(toPrensa, Decision);
        write(fd[1], toPrensa, sizeof(toPrensa));                                       //Se escribe la el mensaje de exito/fracaso al pipe que conecta este subproceso con la prensa
        sem_post(sem);
        
        fclose(fp);
    }
    return;
}


void main(int argc, char *argv[]){

    daysMax = atoi(argv[1]); 
    if(daysMax<=0){
        exit(1);
    }
    if(pipe(fd)<0){                                                     //Inicializa el pipe fd
        perror("pipe ");                                              
        exit(1);
    }
    if(pipe(fd1)<0){                                                    //Inicializa el pipe fd1
        perror("pipe ");                                              
        exit(1);
    }
    write(fd1[1], &day, sizeof(day));
    sem_t * sem;
    sem=sem_open("mysem", O_CREAT, 0600, 1);
    if(sem == SEM_FAILED) {
        perror("parent sem_open");
        return;
    }
    if (fork() == 0){
        //Subproceso para Ejecutivo 
        Ejecutivo(sem); 
    }
    else{
        if (fork() == 0){ 
            //Subproceso para Legislativo
            Legislativo(sem); 
        }
        else{
            if (fork() == 0){ 
                //Subproceso para Judicial
                Judicial(sem); 
            }
            else{
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
        } 
    }                                      
}