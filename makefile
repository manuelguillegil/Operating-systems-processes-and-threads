all:
	gcc -o paisHilosAct.out paisHilosAct.c -lpthread
	gcc -o paisProcesos.out paisProcesos.c -lpthread -lm -lrt