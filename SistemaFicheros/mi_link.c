#include "directorios.h"
//Hecho por: Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame
int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: ./mi_link disco /ruta_fichero_original /ruta_enlace\n" RESET);
        return FALLO;
    }

    char *camino1 = argv[2];
    char *camino2 = argv[3];

    //Comprobar que no sean directorios (por la sintaxis)
    if (camino1[strlen(camino1) - 1] == '/' || camino2[strlen(camino2) - 1] == '/') {
        fprintf(stderr, RED "Error: Las rutas no pueden ser directorios\n" RESET);
        return FALLO;
    }

    if (bmount(argv[1]) == FALLO) {
        fprintf(stderr, RED "Error al montar el dispositivo\n" RESET);
        return FALLO;
    }

    int error = mi_link(camino1, camino2);
    if (error < 0) {
        bumount();
        return FALLO; //mi_link ya imprime el error
    }

    bumount();
    return EXITO;
}