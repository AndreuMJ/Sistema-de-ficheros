#include "semaforo_mutex_posix.h"
//Hecho por: Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame

/* Ejemplo de creación e inicialización de semáforos POSIX para MUTEX con "semáforos con nombre" (named) */


sem_t *initSem() {
   /* name debe ser un nombre de caracteres ascii que comienze con "/", p.e. "/mimutex" */
   sem_t *sem;


   sem = sem_open(SEM_NAME, O_CREAT, S_IRWXU, SEM_INIT_VALUE);
   if (sem == SEM_FAILED) {
       perror("Error sem_open()");
       return NULL;
   }
   return sem;
}


/* void deleteSem() {
   sem_unlink(SEM_NAME);
   // borra el semáforo del sistema y da problemas cuando
   // los hijos en la simulación ejecutan bumount()
} */


void deleteSem(sem_t *sem) {
   //printf("Closing sem in address: %p\n", (void *)sem);
   if (sem_close(sem) == -1) {
       perror("Error deleteSem()");
   }
}


void signalSem(sem_t *sem) {
   sem_post(sem);
}


void waitSem(sem_t *sem) {
   sem_wait(sem);
}
