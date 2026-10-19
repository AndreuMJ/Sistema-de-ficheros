//Hecho por: Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame
#include "ficheros.h"
#include <string.h>
#include <time.h>

//Función para traducir un bloque lógico de un inodo a un bloque físico, dado el número de inodo, el número de bloque lógico y un indicador de si se debe reservar el bloque si no existe. La función recorre los niveles de punteros del inodo desde el nivel más alto hasta el nivel del bloque lógico especificado, utilizando las funciones auxiliares obtener_nRangoBL() y obtener_indice() para determinar el rango de bloques y el índice dentro del bloque de punteros. Si se encuentra un bloque de punteros que no está asignado y se ha indicado que se debe reservar, se reserva un nuevo bloque, se actualiza el bloque de punteros correspondiente con el nuevo bloque asignado y se escribe el bloque de punteros actualizado en el dispositivo. Finalmente, la función devuelve el número de bloque físico correspondiente al bloque lógico especificado o FALLO si ocurre un error.
int mi_write_f(unsigned int ninodo, const void *buf_original,
               unsigned int offset, unsigned int nbytes) {

    struct inodo inodo;
    mi_waitSem();

    if (leer_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error leer inodo\n" RESET);
        mi_signalSem();
        return FALLO;
    }

    if ((inodo.permisos & 2) != 2) {
        fprintf(stderr, RED "No hay permisos de escritura\n" RESET);
        mi_signalSem(); 
        return FALLO;
    }

    unsigned int primerBL = offset / BLOCKSIZE;
    unsigned int ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;

    unsigned int desp1 = offset % BLOCKSIZE;
    unsigned int desp2 = (offset + nbytes - 1) % BLOCKSIZE;

    unsigned char buf_bloque[BLOCKSIZE];

    unsigned int bytes_escritos = 0;

    for (unsigned int i = primerBL; i <= ultimoBL; i++) {

        int nbloque = traducir_bloque_inodo(ninodo, i, 1);
        if (nbloque == FALLO) {
            mi_signalSem();
            return FALLO;
        }

        if (bread(nbloque, buf_bloque) == FALLO) {
            fprintf(stderr, RED "Error bread\n" RESET);
            mi_signalSem(); 
            return FALLO;
        }

        int inicio = 0;
        int fin = BLOCKSIZE;

        if (i == primerBL) inicio = desp1;
        if (i == ultimoBL) fin = desp2 + 1;

        int tam = fin - inicio;

        memcpy(buf_bloque + inicio, buf_original + bytes_escritos, tam);

        if (bwrite(nbloque, buf_bloque) == FALLO) {
            fprintf(stderr, RED "Error bwrite\n" RESET);
            mi_signalSem();
            return FALLO;
        }

        bytes_escritos += tam;
    }

    if (leer_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error leer inodo\n" RESET);
        mi_signalSem();
        return FALLO;
    }

    if (offset + nbytes > inodo.tamEnBytesLog) {
        inodo.tamEnBytesLog = offset + nbytes;
    }

    inodo.mtime = time(NULL); // El contenido siempre cambia al escribir
    
    // Solo actualizamos ctime aquí si el tamaño del fichero crece
    if (offset + nbytes > inodo.tamEnBytesLog) {
        inodo.tamEnBytesLog = offset + nbytes;
        inodo.ctime = time(NULL); 
    }

    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error escribir inodo\n" RESET);
        mi_signalSem();
        return FALLO;
    }
    mi_signalSem();

    return bytes_escritos;
}

//Función para leer datos de un inodo, dado el número de inodo, un buffer para almacenar los datos leídos, el offset desde el cual comenzar a leer y la cantidad de bytes a leer. La función verifica los permisos de lectura del inodo, ajusta la cantidad de bytes a leer si el offset y la cantidad de bytes solicitada exceden el tamaño lógico del inodo, y luego lee los datos correspondientes a los bloques lógicos afectados utilizando la función traducir_bloque_inodo() para traducir los bloques lógicos a bloques físicos y la función bread() para leer los bloques físicos del dispositivo. Finalmente, se actualiza la cantidad de bytes leídos y se devuelve esta cantidad o FALLO si ocurre un error.
int mi_read_f(unsigned int ninodo, void *buf_original,
              unsigned int offset, unsigned int nbytes) {

    struct inodo inodo;
    leer_inodo(ninodo, &inodo);

    if ((inodo.permisos & 4) != 4) {
        fprintf(stderr, RED "Error: no hay permisos de lectura\n" RESET);
        return FALLO;
    }

    if (offset >= inodo.tamEnBytesLog) return 0;

    if ((offset + nbytes) > inodo.tamEnBytesLog)
        nbytes = inodo.tamEnBytesLog - offset;
    
    if (nbytes == 0)
    return 0;

    //Calcular el número de bloque lógico del primer bloque a leer y el número de bloque lógico del último bloque a leer, así como los desplazamientos dentro de estos bloques para el primer byte a leer y el último byte a leer. Luego, se leen los datos correspondientes a los bloques lógicos afectados utilizando la función traducir_bloque_inodo() para traducir los bloques lógicos a bloques físicos y la función bread() para leer los bloques físicos del dispositivo. Finalmente, se actualiza la cantidad de bytes leídos y se devuelve esta cantidad o FALLO si ocurre un error.
    unsigned int primerBL = offset / BLOCKSIZE;
    unsigned int ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;

    unsigned int desp1 = offset % BLOCKSIZE;
    unsigned int desp2 = (offset + nbytes - 1) % BLOCKSIZE;

    unsigned char buf_bloque[BLOCKSIZE];

    int bytes_leidos = 0;
    int nbfisico;

    if (primerBL == ultimoBL) {

        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 0);

    if (nbfisico != -1) { //Si el bloque físico correspondiente al bloque lógico a leer existe (es decir, si el número de bloque físico es diferente de -1), se lee el bloque físico utilizando la función bread(). Si el bloque físico no existe, se llena el buffer del bloque con ceros utilizando memset() para simular un bloque vacío. Luego, se copia la parte correspondiente de los datos leídos al buffer original utilizando memcpy() y se actualiza la cantidad de bytes leídos.
        bread(nbfisico, buf_bloque);
    } else {
        memset(buf_bloque, 0, BLOCKSIZE);
    }

    memcpy(buf_original, buf_bloque + desp1, nbytes);

        bytes_leidos = nbytes;

    } else { //Sino, se realiza un proceso similar al caso anterior pero teniendo en cuenta que se deben leer los datos de varios bloques físicos correspondientes a los bloques lógicos afectados. Se traduce el primer bloque lógico a bloque físico y se lee la parte correspondiente de los datos a leer de ese bloque físico. Luego, se itera sobre los bloques lógicos intermedios, traduciendo cada bloque lógico a bloque físico y leyendo la parte correspondiente de los datos a leer de cada bloque físico. Finalmente, se traduce el último bloque lógico a bloque físico y se lee la parte correspondiente de los datos a leer de ese bloque físico. La cantidad total de bytes leídos se actualiza en consecuencia.

        nbfisico = traducir_bloque_inodo(ninodo, primerBL, 0);

        if (nbfisico != -1) {
            bread(nbfisico, buf_bloque);
        } else {
            memset(buf_bloque, 0, BLOCKSIZE);
        }

        memcpy(buf_original, buf_bloque + desp1, BLOCKSIZE - desp1);

        bytes_leidos += BLOCKSIZE - desp1;

        for (int i = primerBL + 1; i < ultimoBL; i++) {

            nbfisico = traducir_bloque_inodo(ninodo, i, 0);

            if (nbfisico != -1){
                bread(nbfisico, (char*)buf_original + bytes_leidos);
            }else{
                memset(buf_original + bytes_leidos,0,BLOCKSIZE);
            }

            bytes_leidos += BLOCKSIZE;
        }

        nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 0);

        if (nbfisico != -1) {
            bread(nbfisico, buf_bloque);
        }else{
            memset(buf_bloque,0,BLOCKSIZE);
        }

        memcpy(buf_original + bytes_leidos, buf_bloque, desp2 + 1);

        bytes_leidos += desp2 + 1;
    }
    mi_waitSem();

     if (leer_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error leer inodo\n" RESET);
        mi_signalSem();
        return FALLO;
    }

    inodo.atime = time(NULL);
    escribir_inodo(ninodo, &inodo);
    mi_signalSem();

    if (bytes_leidos > nbytes) {
        bytes_leidos = nbytes;
    }

    if (DN5==1){
        fprintf(stderr, GRAY "[mi_read_f → %d bytes leidos]\n" RESET, bytes_leidos);
    }

    return bytes_leidos;
}

//Función para obtener la información de un inodo, dado su número de inodo y un puntero a una estructura STAT para almacenar la información del inodo. La función lee el inodo correspondiente utilizando la función leer_inodo() y luego copia la información relevante del inodo a la estructura STAT proporcionada. Finalmente, devuelve EXITO si la operación se realizó correctamente o FALLO si ocurre un error.
int mi_stat_f(unsigned int ninodo, struct STAT *p_stat) {

    struct inodo inodo;

    if (leer_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error leyendo inodo\n" RESET);
        return FALLO;
    }

    p_stat->tipo = inodo.tipo;
    p_stat->permisos = inodo.permisos;
    p_stat->nlinks = inodo.nlinks;
    p_stat->tamEnBytesLog = inodo.tamEnBytesLog;
    p_stat->atime = inodo.atime;
    p_stat->mtime = inodo.mtime;
    p_stat->ctime = inodo.ctime;
    p_stat->btime = inodo.btime;
    p_stat->numBloquesOcupados = inodo.numBloquesOcupados;

    return EXITO;
}

//Función para cambiar los permisos de un inodo, dado su número de inodo y los nuevos permisos. La función lee el inodo correspondiente utilizando la función leer_inodo(), actualiza el campo de permisos del inodo con los nuevos permisos proporcionados y actualiza el campo ctime del inodo con la fecha y hora actual. Luego, escribe el inodo actualizado de nuevo en el dispositivo utilizando la función escribir_inodo(). Finalmente, devuelve EXITO si la operación se realizó correctamente o FALLO si ocurre un error.
int mi_chmod_f(unsigned int ninodo, unsigned char permisos) {

    struct inodo inodo;
    mi_waitSem();

    if (leer_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error leyendo inodo\n" RESET);
        mi_signalSem();
        return FALLO;
    }

    inodo.permisos = permisos;
    inodo.ctime = time(NULL);

    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error escribiendo inodo\n" RESET);
        mi_signalSem();
        return FALLO;
    }

    if (DN5==1){
        fprintf(stderr, GRAY "[mi_chmod_f → permisos cambiados]\n" RESET);
    }

    mi_signalSem();
    return EXITO;
}

//Función para truncar un inodo, dado su número de inodo y el nuevo tamaño en bytes. La función lee el inodo correspondiente utilizando la función leer_inodo(), verifica que se tengan permisos de escritura y que el nuevo tamaño no sea mayor al tamaño lógico actual del inodo. Luego, calcula el número de bloque lógico a partir del nuevo tamaño y utiliza la función liberar_bloques_inodo() para liberar los bloques ocupados por el inodo a partir del bloque lógico calculado. Actualiza la información del inodo para reflejar el nuevo tamaño, la cantidad de bloques ocupados y las fechas de modificación, y escribe el inodo actualizado de nuevo en el dispositivo utilizando la función escribir_inodo(). Finalmente, devuelve la cantidad de bloques liberados o FALLO si ocurre un error.
int mi_truncar_f(unsigned int ninodo, unsigned int nbytes) {

    struct inodo inodo;

    if (leer_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error leer inodo\n" RESET);
        return FALLO;
    }

    if ((inodo.permisos & 2) != 2) {
        fprintf(stderr, RED "No hay permisos de escritura\n" RESET);
        return FALLO;
    }

    if (nbytes > inodo.tamEnBytesLog) {
        fprintf(stderr, RED "No se puede truncar más allá del EOF\n" RESET);
        return FALLO;
    }

    unsigned int primerBL;

    if (nbytes % BLOCKSIZE == 0)
        primerBL = nbytes / BLOCKSIZE;
    else
        primerBL = nbytes / BLOCKSIZE + 1;

    int liberados = liberar_bloques_inodo(primerBL, &inodo);

    inodo.numBloquesOcupados -= liberados;
    inodo.tamEnBytesLog = nbytes;

    inodo.mtime = time(NULL);
    inodo.ctime = time(NULL);

    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error escribir inodo\n" RESET);
        return FALLO;
    }

    if (DN6==1){
        fprintf(stderr, GRAY "[mi_truncar_f()→ liberados %d bloques]\n" RESET, liberados);
    }

    struct inodo inodo_leido;

    if (leer_inodo(ninodo, &inodo_leido) == FALLO) {
        fprintf(stderr, RED "Error leyendo inodo tras truncar\n" RESET);
        return FALLO;
    }

    return liberados;
}