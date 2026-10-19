#include "directorios.h"
//Hecho por: Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame
int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: ./mi_touch <disco> <permisos> </ruta_fichero>\n" RESET);
        return FALLO;
    }
    
    unsigned char permisos = atoi(argv[2]);
    if (permisos > 7) {
        fprintf(stderr, RED "Error: modo inválido: <<%d>>\n" RESET, permisos);
        return FALLO;
    }

    const char *camino = argv[3];
    if (camino[strlen(camino) - 1] == '/') {
        fprintf(stderr, RED "Error: El camino NO debe terminar en '/' para ser un fichero.\n" RESET);
        return FALLO;
    }

    bmount(argv[1]);
    int error = mi_creat(camino, permisos);
    if (error < 0) mostrar_error_buscar_entrada(error);
    bumount();

    return EXITO;
}