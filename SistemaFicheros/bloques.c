//Hecho por: Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame
#include "bloques.h"
#include "semaforo_mutex_posix.h"

static sem_t *mutex;
static unsigned int inside_sc = 0;
static int descriptor = 0; 

int bmount(const char *camino) {
    umask(0111);
    descriptor = open(camino, O_RDWR | O_CREAT, 0666);
    if (descriptor == -1) { 
        fprintf(stderr, RED "Error en bmount (open): %s\n" RESET, strerror(errno));
        return FALLO;
    }
    if (chmod(camino, 0666) == -1) {
        fprintf(stderr, RED "Error en chmod: %s\n" RESET, strerror(errno));
        return FALLO;
    }

    // Inicialización del semáforo
    if (!mutex) { // el semáforo es único en el sistema y sólo se ha de inicializar 1 vez
        mutex = initSem();
        if (mutex == SEM_FAILED) {
            return FALLO;
        }
    }

    if (DN1==1) fprintf(stderr, GRAY "[bmount descriptor=%d]\n" RESET, descriptor);
    return descriptor;
}

int bumount() {
    if (close(descriptor) == -1) {
        fprintf(stderr, RED "Error en bumount (close): %s\n" RESET, strerror(errno));
        return FALLO;
    }

    deleteSem(mutex); // Elimina el semáforo (nota: si el compilador pide argumento, pon deleteSem(mutex))

    if (DN1==1) fprintf(stderr, GRAY "[bumount cerrado]\n" RESET);
    descriptor = 0;
    return EXITO;
}

//bwrite(), escribe un bloque físico en el dispositivo virtual.
int bwrite(unsigned int nbloque, const void *buf) {

    if (lseek(descriptor, nbloque * BLOCKSIZE, SEEK_SET) == -1) {
        fprintf(stderr, RED "Error en bwrite (lseek): %s\n" RESET, strerror(errno));
        return FALLO;
    }

    int escritos = write(descriptor, buf, BLOCKSIZE); //write() devuelve el número de bytes escritos

    if (escritos != BLOCKSIZE) {
        fprintf(stderr, RED "Error en bwrite (write): %s\n" RESET, strerror(errno));
        return FALLO;
    }

    return escritos;
}


//bread(), lee un bloque físico del dispositivo virtual.
int bread(unsigned int nbloque, void *buf) {

    if (lseek(descriptor, nbloque * BLOCKSIZE, SEEK_SET) == -1) {
        fprintf(stderr, RED "Error en bread (lseek): %s\n" RESET, strerror(errno));
        return FALLO;
    }

    int leidos = read(descriptor, buf, BLOCKSIZE); //read() devuelve el número de bytes leídos, o 0 si se ha alcanzado el final del archivo

    if (leidos != BLOCKSIZE) {
        fprintf(stderr, RED "Error en bread (read): %s\n" RESET, strerror(errno));
        return FALLO;
    }

    return leidos;
}

void mi_waitSem() {
    if (!inside_sc) {
        waitSem(mutex);
    }
    inside_sc++;
}

void mi_signalSem() {
    inside_sc--;
    if (!inside_sc) {
        signalSem(mutex);
    }
}