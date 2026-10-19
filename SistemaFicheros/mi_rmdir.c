#include "directorios.h"
//Hecho por: Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, RED "Sintaxis: ./mi_rmdir disco /ruta_directorio/\n" RESET);
        return FALLO;
    }

    char *camino = argv[2];

    if (strcmp(camino, "/") == 0) {
        fprintf(stderr, RED "Error: No se puede borrar el directorio raíz.\n" RESET);
        return FALLO;
    }

    if (bmount(argv[1]) == FALLO) {
        fprintf(stderr, RED "Error al montar el dispositivo\n" RESET);
        return FALLO;
    }

    int error = mi_unlink(camino);
    if (error < 0) {
        bumount();
        return FALLO; 
    }

    bumount();
    return EXITO;
}