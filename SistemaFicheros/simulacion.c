#include "simulacion.h"
//Hecho por: Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame
int acabados = 0;

void reaper() {
    pid_t ended;
    signal(SIGCHLD, reaper);
    while ((ended = waitpid(-1, NULL, WNOHANG)) > 0) {
        acabados++;
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, RED "Sintaxis: ./simulacion <disco>\n" RESET);
        return FALLO;
    }

    bmount(argv[1]);

    //Crear directorio de simulación con formato simul_aaaammddhhmmss
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char camino_simul[100];
    sprintf(camino_simul, "/simul_%04d%02d%02d%02d%02d%02d/",
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
            tm->tm_hour, tm->tm_min, tm->tm_sec);

    if (mi_creat(camino_simul, 6) < 0) {
        fprintf(stderr, RED "Error al crear el directorio de simulación\n" RESET);
        bumount();
        return FALLO;
    }

    //asociar la señal SIGCHLD al manejador reaper()
    signal(SIGCHLD, reaper);

    printf("Directorio de simulación: %s\n", camino_simul);
    printf("Lanzando procesos...\n");

    for (int proceso = 1; proceso <= NUMPROCESOS; proceso++) {
        pid_t pid = fork();
        
        if (pid == 0) { 
            bmount(argv[1]); //Montar dispositivo en el hijo
            
            //Crear directorio del proceso: /simul_.../proceso_PID/
            char camino_hijo[200];
            sprintf(camino_hijo, "%sproceso_%d/", camino_simul, getpid());
            if (mi_creat(camino_hijo, 6) < 0) {
                bumount();
                exit(0);
            }

            //Crear fichero prueba.dat: /simul_.../proceso_PID/prueba.dat
            char camino_fichero[250];
            sprintf(camino_fichero, "%sprueba.dat", camino_hijo);
            if (mi_creat(camino_fichero, 6) < 0) {
                bumount();
                exit(0);
            }

            //Inicializar semilla de números aleatorios para este hijo
            srand(time(NULL) + getpid());

            for (int nEscritura = 1; nEscritura <= NUMESCRITURAS; nEscritura++) {
                struct REGISTRO registro;
                registro.fecha = time(NULL);
                registro.pid = getpid();
                registro.nEscritura = nEscritura;
                registro.nRegistro = rand() % REGMAX; //[0, 499.999]

                //Escribir el registro utilizando mi_write
                mi_write(camino_fichero, &registro, registro.nRegistro * sizeof(struct REGISTRO), sizeof(struct REGISTRO));
                
                //eperar 0.05 segundos para ceder la CPU a otros procesos
                usleep(50000); 
            }

            printf("[Proceso %d: Completadas %d escrituras en %s]\n", proceso, NUMESCRITURAS, camino_fichero);
            
            bumount();
            exit(0); //El hijo termina
        }
        
        //procesos padres esperan un poco antes de lanzar el siguiente hijo para evitar que todos los hijos se lancen exactamente al mismo tiempo
        //Esperar 0.15 segundos antes de lanzar el siguiente hijo
        usleep(150000); 
    }

    //El padre espera a que todos los procesos hijos terminen
    while (acabados < NUMPROCESOS) {
        pause();
    }

    printf("[Proceso Principal: Total de procesos terminados: %d]\n", acabados);
    bumount();
    return EXITO;
}