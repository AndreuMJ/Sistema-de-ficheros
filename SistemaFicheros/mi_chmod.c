//Hecho por: Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame
#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: ./mi_chmod <disco> <permisos> </ruta>\n" RESET);
        return FALLO;
    }

    unsigned char permisos = atoi(argv[2]);
    if (permisos > 7) {
        fprintf(stderr, RED "Error: modo inválido: <<%d>>\n" RESET, permisos);
        return FALLO;
    }

    bmount(argv[1]);
    int error = mi_chmod(argv[3], permisos);
    if (error < 0) mostrar_error_buscar_entrada(error);
    bumount();

    return EXITO;
}