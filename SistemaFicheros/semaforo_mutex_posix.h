//Hecho por: Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>


#define SEM_NAME "/mymutex" /* Usamos este nombre para el semáforo mutex */
#define SEM_INIT_VALUE 1 /* Valor inicial de los mutex */


sem_t *initSem();
void deleteSem(sem_t *sem);
void signalSem(sem_t *sem);
void waitSem(sem_t *sem);