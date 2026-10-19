#include "directorios.h"
//Hecho por: Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, RED "Sintaxis: ./mi_stat <disco> </ruta>\n" RESET);
        return FALLO;
    }

    bmount(argv[1]);
    
    struct STAT stat;
    int error = mi_stat(argv[2], &stat);

    if (error < 0) {
        mostrar_error_buscar_entrada(error);
    } else {
        printf("Nº de inodo: %d\n", error); 
        printf("tipo: %c\n", stat.tipo);
        printf("permisos: %d\n", stat.permisos);
        printf("atime: %s", asctime(localtime(&stat.atime)));
        printf("mtime: %s", asctime(localtime(&stat.mtime)));
        printf("ctime: %s", asctime(localtime(&stat.ctime)));
        printf("btime: %s", asctime(localtime(&stat.btime)));
        printf("nlinks: %u\n", stat.nlinks);
        printf("tamEnBytesLog: %u\n", stat.tamEnBytesLog);
        printf("numBloquesOcupados: %u\n", stat.numBloquesOcupados);
    }

    bumount();
    return EXITO;
}