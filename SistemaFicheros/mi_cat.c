//Hecho por: Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame
#include "directorios.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, RED "Sintaxis: ./mi_cat <disco> </ruta_fichero>\n" RESET);
        return FALLO;
    }

    char *camino = argv[2];

    if (camino[strlen(camino) - 1] == '/') {
        fprintf(stderr, RED "Error: la ruta se corresponde a un directorio, no se puede hacer cat.\n" RESET);
        return FALLO;
    }

    if (bmount(argv[1]) == FALLO) {
        fprintf(stderr, RED "Error al montar el dispositivo\n" RESET);
        return FALLO;
    }

    int tambuffer = BLOCKSIZE * 4; // Buffer de lectura optimizado para el test
    unsigned char buffer[tambuffer];
    memset(buffer, 0, tambuffer);

    int leidos = 0;
    int offset = 0;
    int total_leidos = 0;

    // Leemos secuencialmente hasta el final lógico del fichero
    while ((leidos = mi_read(camino, buffer, offset, tambuffer)) > 0) {
        write(1, buffer, leidos); // Volcamos a la salida estándar
        offset += leidos;
        total_leidos += leidos;
        memset(buffer, 0, tambuffer); // Filtrado de basura para la siguiente iteración
    }

    fprintf(stderr, "\nTotal_leidos %d\n", total_leidos);

    bumount();
    return EXITO;
}