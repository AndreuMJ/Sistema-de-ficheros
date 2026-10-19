//Hecho por: Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame
#include <sys/wait.h>
#include <signal.h>
#include "directorios.h"

#define NUMPROCESOS 100
#define NUMESCRITURAS 50
#define REGMAX 500000 //REGMAX = ((12+256+256²+256³)*BLOCKSIZE) /sizeof(struct registro)

struct REGISTRO { //sizeof(struct REGISTRO): 24 bytes
   time_t fecha; //Precisión segundos [opcionalmente microsegundos con struct timeval]
   pid_t pid; //PID del proceso que lo ha creado
   int nEscritura; //Entero con el nº de escritura, de 1 a 50 (orden por tiempo)
   int nRegistro; //Entero con el nº del registro dentro del fichero: [0..REGMAX-1] (orden por posición)
};
