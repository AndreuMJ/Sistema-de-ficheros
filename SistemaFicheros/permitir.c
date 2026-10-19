#include "ficheros.h"
//Hecho por: Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame
int main(int argc, char **argv) {

    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: permitir <disco> <ninodo> <permisos>\n" RESET);
        return FALLO;
    }

    bmount(argv[1]); 

    unsigned int ninodo = atoi(argv[2]);
    unsigned char permisos = atoi(argv[3]);

    mi_chmod_f(ninodo, permisos);

    bumount();

    return EXITO;
}