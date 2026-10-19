#include "ficheros_basico.h"
//Hecho por: Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame
int main(int argc, char **argv) {

    if (argc != 3) {
        fprintf(stderr, RED "Uso: %s <nombre_dispositivo> <nbloques>\n" RESET, argv[0]);
        return FALLO;
    }

    const char *nombre = argv[1]; //El nombre del dispositivo virtual a crear
    int nbloques = atoi(argv[2]); //El número de bloques que tendrá el sistema de archivos
    if (nbloques <= 0) {
        fprintf(stderr, RED "Error: nbloques debe ser mayor que 0\n" RESET);
        return FALLO;
    }
    if (DN1==1){
        fprintf(stderr, GRAY "[creando %s con %d bloques]\n" RESET, nombre, nbloques); //Mensaje de depuración indicando el nombre del dispositivo virtual y el número de bloques
    }

    if (bmount(nombre) == FALLO) {
        return FALLO;
    }

    unsigned char buffer[BLOCKSIZE];
    memset(buffer, 0, BLOCKSIZE);

    for (int i = 0; i < nbloques; i++) { //Escribir bloques vacíos en el dispositivo virtual para asegurarse de que el archivo tenga el tamaño correcto y esté completamente formateado.
        if (bwrite(i, buffer) == FALLO) {
            bumount();
            return FALLO;
        }
    }

    unsigned int ninodos = nbloques / 4; //Establecer el número de inodos como una fracción del número total de bloques, por ejemplo, 1 inodo cada 4 bloques. Esto es una heurística común para sistemas de archivos simples.

    if (initSB(nbloques, ninodos) == FALLO) return FALLO;
    if (initMB() == FALLO) return FALLO;
    if (initAI() == FALLO) return FALLO;
    reservar_inodo('d', 7); //crear inodo raíz

    if (bumount() == FALLO) {
        return FALLO;
    }

    if (DN6==1){
        fprintf(stderr, GRAY "[formateo completado]\n" RESET);}

    return EXITO;
}
