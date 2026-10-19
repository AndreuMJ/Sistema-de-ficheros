//Hecho por: Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame
#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, RED "Sintaxis: ./mi_escribir <disco> </ruta_fichero> <texto> <offset>\n" RESET);
        return FALLO;
    }

    char *camino = argv[2];
    char *texto = argv[3];
    unsigned int offset = atoi(argv[4]);
    int longitud = strlen(texto);

    // Comprobar que es un fichero (no termina en '/')
    if (camino[strlen(camino) - 1] == '/') {
        fprintf(stderr, RED "Error: la ruta se corresponde a un directorio\n" RESET);
        return FALLO;
    }

    if (bmount(argv[1]) == FALLO) {
        fprintf(stderr, RED "Error al montar el dispositivo\n" RESET);
        return FALLO;
    }

    printf("longitud texto: %d\n", longitud);

    int escritos = mi_write(camino, texto, offset, longitud);

    if (escritos >= 0) {
        printf("Bytes escritos: %d\n", escritos);
    } else {
        printf("Bytes escritos: 0\n");
    }

    bumount();
    return EXITO;
}