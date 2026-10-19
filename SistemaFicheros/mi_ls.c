#include "directorios.h"
//Hecho por: Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame
int main(int argc, char **argv) {
    if (argc != 3 && argc != 4) {
        fprintf(stderr, RED "Sintaxis: ./mi_ls <disco> </ruta> O ./mi_ls -l <disco> </ruta>\n" RESET);
        return FALLO;
    }

    char extendido = 0;
    int idx_disco = 1;
    int idx_ruta = 2;

    if (argc == 4) {
        if (strcmp(argv[1], "-l") == 0) {
            extendido = 1;
            idx_disco = 2;
            idx_ruta = 3;
        } else {
            fprintf(stderr, RED "Sintaxis incorrecta.\n" RESET);
            return FALLO;
        }
    }

    const char *camino = argv[idx_ruta];
    char tipo = (camino[strlen(camino) - 1] == '/') ? 'd' : 'f';

    bmount(argv[idx_disco]);
    
    char buffer[TAMBUFFER];
    memset(buffer, 0, TAMBUFFER);

    int num_entradas = mi_dir(camino, buffer, tipo, extendido);

    if (num_entradas >= 0) {
        if (tipo == 'd') printf("Total: %d\n", num_entradas);
        if (extendido) printf("Tipo\tModo\t\tmTime\t\t\tTamaño\tNombre\n");
        printf("--------------------------------------------------------------------------------\n");
        printf("%s", buffer);
    } else if (num_entradas < 0) {
        mostrar_error_buscar_entrada(num_entradas);
    }

    bumount();
    return EXITO;
}