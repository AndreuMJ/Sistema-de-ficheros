//Hecho por: Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame
#include "ficheros.h"

//Función principal para truncar un fichero, dado su número de inodo y la cantidad de bytes a truncar. La función verifica que se hayan proporcionado los argumentos correctos, monta el dispositivo utilizando la función bmount(), convierte el número de inodo y la cantidad de bytes a truncar proporcionados como argumentos a sus respectivos tipos de datos, llama a la función mi_truncar_f() para realizar el truncamiento del fichero especificado, desmonta el dispositivo utilizando la función bumount() y devuelve EXITO si la operación se realizó correctamente o FALLO si ocurre un error.
int main(int argc, char **argv) {

    if (argc != 4) {
        fprintf(stderr, RED "Sintaxis: truncar <disco> <ninodo> <nbytes>\n" RESET);
        return FALLO;
    }

    if (bmount(argv[1]) == FALLO) {
        fprintf(stderr, RED "Error al montar dispositivo\n" RESET);
        return FALLO;
    }

    unsigned int ninodo = atoi(argv[2]);
    unsigned int nbytes = atoi(argv[3]);

    if (nbytes == 0) {

        if (mi_truncar_f(ninodo, 0) == FALLO) {
            fprintf(stderr, RED "Error al truncar\n" RESET);
            bumount();
            return FALLO;
        }

        if (liberar_inodo(ninodo) == FALLO) {
            fprintf(stderr, RED "Error al liberar inodo\n" RESET);
            bumount();
            return FALLO;
        }

    } else {

        if (mi_truncar_f(ninodo, nbytes) == FALLO) {
            fprintf(stderr, RED "Error al truncar\n" RESET);
            bumount();
            return FALLO;
        }
    }

    bumount();

    return EXITO;
}