#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <pthread.h> 
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>

static void parent(sem_t* sem_id)
{   
    /*sem_wait(sem_id);
    printf("Parent is out\n");
    sem_post(sem_id);*/
}

static void child(sem_t* sem_id)
{
    /*sem_wait(sem_id);
    printf("child is out\n");
    sem_post(sem_id);*/
}

int main(int argc, char *argv[])
{
    /*sem_unlink("mysem");*/
    pid_t pid;
    int *shared = mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);

    pid=fork();
    if(pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if(!pid) {
        /*sem_t * sem_id;
        sem_id=sem_open("mysem", O_CREAT, 0666, 1);                  //FUNCIONA ASI COÑO BIEN
        if(sem_id == SEM_FAILED) {
            perror("parent sem_open");
            exit(1);
        }
        child(sem_id);*/ 
        *shared = 5;
    } else {
        /*int status;
        sem_t * sem_id;
        sem_id=sem_open("mysem", O_CREAT, 0666, 1);                  //FUNCIONA ASI COÑO BIEN
        if(sem_id == SEM_FAILED) {
            perror("parent sem_open");
            exit(1);
        }*/
        sleep(1);
        printf("%d", *shared);
    }
    return 0;
}