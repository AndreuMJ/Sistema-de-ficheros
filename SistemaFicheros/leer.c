//Hecho por: Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame
#include "ficheros.h"

//Función para leer el contenido de un fichero, dado su número de inodo, un buffer para almacenar los datos leídos, un desplazamiento y la cantidad de bytes a leer. La función verifica que se tengan permisos de lectura y que el desplazamiento no sea mayor al tamaño lógico del inodo. Luego, calcula el número de bloque lógico a partir del desplazamiento y utiliza la función traducir_bloque_inodo() para traducir el bloque lógico a bloque físico. Si el bloque físico existe, se lee el bloque utilizando la función bread() y se copia la parte correspondiente de los datos leídos al buffer original utilizando memcpy(). Si el bloque físico no existe, se llena el buffer del bloque con ceros utilizando memset() para simular un bloque vacío. Finalmente, se actualiza la cantidad de bytes leídos y se devuelve esta cantidad o FALLO si ocurre un error.
int main(int argc, char **argv){
    int tamBuffer = BLOCKSIZE;
    if (argc != 3){
        fprintf(stderr,"Sintaxis: leer <nombre_dispositivo> <ninodo>\n");
        return FALLO;
    }

    if (bmount(argv[1]) == FALLO) {
        fprintf(stderr, "Error al montar el dispositivo\n");
        return FALLO;
    }

    int ninodo = atoi(argv[2]);
    unsigned char buffer[tamBuffer];

    int leidos = 0;
    int offset = 0;
    int total_leidos = 0; //Acumulador para llevar la cuenta

    //Leemos hasta que mi_read_f devuelva 0 (fin del fichero)
    while ((leidos = mi_read_f(ninodo, buffer, offset, tamBuffer)) > 0){
        
        //Escribimos a la salida estándar (para que funcione > ext1.txt)
        write(1, buffer, leidos);

        offset += leidos;
        total_leidos += leidos;
    }

    //Obtenemos el stat para ver el tamaño en bytes lógico
    struct STAT stat;
    mi_stat_f(ninodo, &stat);

    //Imprimimos por stderr para que NO se meta en el archivo ext1.txt
    fprintf(stderr, "\ntotal_leidos %d\n", total_leidos);
    fprintf(stderr, "tamEnBytesLog %d\n", stat.tamEnBytesLog);

    bumount();

    return EXITO;
}