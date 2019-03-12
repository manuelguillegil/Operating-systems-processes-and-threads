#include <stdlib.h>	//constantes EXIT_SUCCESS y EXIT_FAILURE
#include <stdio.h>	//función printf
#include <time.h>	//funciones localtime, asctime
#include <sys/stat.h> 
#ifdef CONFIG_BLOCK	//no funciona en todas las versiones de unix
	#include <linux/fs.h>	//función bdget
#endif
#include <pwd.h>	//función getpwuid
#include <grp.h>	//función getgrgid
#include <unistd.h>  // función fork
#include <sys/wait.h>
#include <pthread.h> // librería para los hilos

int main(int argc, char const *argv[]) {

	int p_id; // Identificador de cada proceso
 
	pthread_t hilo1;  // Variable de tipo hilo 
	pthread_create(&hilo1, NULL, funcion_a_ejecutar, NULL); // Inicializa el hilo


	/// No he comenzado nada del proyecto, solo estoy probando y viendo cosas sobre los procesos e hilos att Manuel




	return 0;

}