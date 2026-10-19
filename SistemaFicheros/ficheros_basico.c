//Hecho por: Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame
#include "ficheros_basico.h"

//Funciones auxiliares 
static int buscar_primer_bloque_libre(struct superbloque *SB);

static int obtener_posicion_bit(unsigned int nbloque,
                                unsigned int *nbloqueabs,
                                unsigned int *posbyte,
                                unsigned int *posbit,
                                unsigned char *bufferMB);

//Función para calcular el tamaño del mapa de bits (en bloques) necesario para albergar un número dado de bloques.
int tamMB(unsigned int nbloques) {

    int tam = (nbloques / 8) / BLOCKSIZE;

    if ((nbloques / 8) % BLOCKSIZE != 0) { //Si el número de bloques no es múltiplo de 8*BLOCKSIZE, se necesita un bloque adicional para el resto de bits.
        tam++;
    }

    if (DN2==1){
        printf(GRAY "[DEBUG tamMB → %d bloques]\n" RESET, tam);
    }

    return tam;
}

//Función para calcular el tamaño del array de inodos (en bloques) necesario para albergar un número dado de inodos.
int tamAI(unsigned int ninodos) {

    int tam = (ninodos * INODOSIZE) / BLOCKSIZE; //El número de bloques necesarios para albergar los inodos, considerando el tamaño de cada inodo y el tamaño de bloque.

    if ((ninodos * INODOSIZE) % BLOCKSIZE != 0) {
        tam++;
    }

    if (DN2==1){
        printf(GRAY "[DEBUG tamAI → %d bloques]\n" RESET, tam);
    }

    return tam;
}

//Función para inicializar el superbloque con la información del número total de bloques e inodos, y las posiciones de los bloques de metadatos y datos.
int initSB(unsigned int nbloques, unsigned int ninodos) {

    struct superbloque SB;

    SB.posPrimerBloqueMB = posSB + tamSB;
    SB.posUltimoBloqueMB = SB.posPrimerBloqueMB + tamMB(nbloques) - 1;

    SB.posPrimerBloqueAI = SB.posUltimoBloqueMB + 1;
    SB.posUltimoBloqueAI = SB.posPrimerBloqueAI + tamAI(ninodos) - 1;

    SB.posPrimerBloqueDatos = SB.posUltimoBloqueAI + 1;
    SB.posUltimoBloqueDatos = nbloques - 1;

    SB.posInodoRaiz = 0;
    SB.posPrimerInodoLibre = 0;

    SB.cantBloquesLibres = nbloques;
    SB.cantInodosLibres = ninodos;

    SB.totBloques = nbloques;
    SB.totInodos = ninodos;

    if (bwrite(posSB, &SB) == FALLO) {
        fprintf(stderr, RED "Error escribiendo el superbloque\n" RESET);
        return FALLO;
    }

    if (DN2==1){
        printf(GRAY "[DEBUG initSB correcto]\n" RESET);
    }

    return EXITO;
}

//Función para inicializar el mapa de bits, marcando como ocupados los bloques de metadatos (superbloque, mapa de bits y array de inodos) y actualizando la cantidad de bloques libres en el superbloque.
int initMB() {

    struct superbloque SB;
    if (bread(posSB, &SB) == FALLO) {
        fprintf(stderr, RED "Error leyendo SB en initMB\n" RESET);
        return FALLO;
    }

    unsigned char bufferMB[BLOCKSIZE]; //Buffer para escribir el mapa de bits
    memset(bufferMB, 0, BLOCKSIZE); //Inicializar el buffer con ceros (todos los bloques libres)

    int bloquesOcupados = SB.posPrimerBloqueDatos;

    int posbyteMB = bloquesOcupados / 8;
    int posbit = bloquesOcupados % 8;

    int bloquesMB = tamMB(SB.totBloques);

    for (int i = 0; i < bloquesMB; i++) { //Recorrer los bloques del mapa de bits

        memset(bufferMB, 0, BLOCKSIZE);

        if (posbyteMB >= BLOCKSIZE) { //Si el número de bloques ocupados es mayor o igual al número de bits que caben en un bloque, marcar el bloque completo como ocupado (255) y actualizar la posición del siguiente bit a marcar
            memset(bufferMB, 255, BLOCKSIZE);
            posbyteMB -= BLOCKSIZE;
        } else {

            for (int j = 0; j < posbyteMB; j++) {
                bufferMB[j] = 255;
            }

            if (posbit > 0) {
                bufferMB[posbyteMB] = (unsigned char)(255 << (8 - posbit));
            }

            posbyteMB = 0;
        }

        if (bwrite(SB.posPrimerBloqueMB + i, bufferMB) == FALLO) {
            fprintf(stderr, RED "Error escribiendo MB\n" RESET);
            return FALLO;
        }
    }

    SB.cantBloquesLibres -= bloquesOcupados; //Actualizar la cantidad de bloques libres en el superbloque

    if (bwrite(posSB, &SB) == FALLO) {
        fprintf(stderr, RED "Error actualizando SB\n" RESET);
        return FALLO;
    }

    if (DN2==1){
        printf(GRAY "[DEBUG initMB correcto]\n" RESET);
    }

    return EXITO;
}

//Función para inicializar el array de inodos, marcando como libres todos los inodos y enlazándolos en una lista enlazada a través del primer puntero directo de cada inodo.
int initAI() {

    struct superbloque SB;

    if (bread(posSB, &SB) == FALLO) {
        fprintf(stderr, RED "Error leyendo SB en initAI\n" RESET);
        return FALLO;
    }

    struct inodo inodos[BLOCKSIZE / INODOSIZE]; //Array de inodos para escribir en cada bloque del array de inodos

    unsigned int contInodos = SB.posPrimerInodoLibre + 1; //Contador para asignar el número de inodo siguiente en la lista enlazada de inodos libres, comenzando desde el primer inodo libre + 1 (el primer inodo libre se asignará al primer bloque del array de inodos)

    for (unsigned int i = SB.posPrimerBloqueAI;
         i <= SB.posUltimoBloqueAI; i++) { //Recorrer los bloques del array de inodos

        if (bread(i, inodos) == FALLO) {
            fprintf(stderr, RED "Error leyendo AI\n" RESET);
            return FALLO;
        }

        for (int j = 0; j < BLOCKSIZE / INODOSIZE; j++) { //Recorrer los inodos dentro del bloque

            unsigned int indice = (i - SB.posPrimerBloqueAI)
                                  * (BLOCKSIZE / INODOSIZE) + j; //Calcular el índice absoluto del inodo en el array de inodos

            if (indice >= SB.totInodos) break;

            inodos[j].tipo = 'l';

            if (contInodos < SB.totInodos) {
                inodos[j].punterosDirectos[0] = contInodos; //Enlazar el inodo actual con el siguiente inodo libre a través del primer puntero directo
                contInodos++;
            } else {
                inodos[j].punterosDirectos[0] = UINT_MAX;
            }
        }

        if (bwrite(i, inodos) == FALLO) {
            fprintf(stderr, RED "Error escribiendo AI\n" RESET);
            return FALLO;
        }
    }

    if (DN2==1){
        printf(GRAY "[DEBUG initAI correcto]\n" RESET);
    }

    return EXITO;
}

//Función para crear el sistema de archivos, inicializando el superbloque, el mapa de bits y el array de inodos.
int escribir_bit(unsigned int nbloque, unsigned int bit) {

    unsigned int nbloqueabs, posbyte, posbit;
    unsigned char bufferMB[BLOCKSIZE];

    if (obtener_posicion_bit(nbloque, &nbloqueabs, &posbyte, &posbit, bufferMB) == FALLO) {
        return FALLO;
    }

    unsigned char mascara = 128;
    mascara >>= posbit;

    if (bit == 1) //Si el bit a escribir es 1, se marca el bloque como ocupado utilizando una máscara de bits para establecer el bit correspondiente en el byte del mapa de bits. Si el bit a escribir es 0, se marca el bloque como libre utilizando una máscara de bits para limpiar el bit correspondiente en el byte del mapa de bits.
        bufferMB[posbyte] |= mascara;
    else
        bufferMB[posbyte] &= ~mascara;

    if (bwrite(nbloqueabs, bufferMB) == FALLO) {
        fprintf(stderr, RED "Error al escribir bloque del MB\n" RESET);
        return FALLO;
    }

    if (DN3==1){
        fprintf(stderr, GRAY "[escribir_bit(%u,%u)]\n" RESET, nbloque, bit);
    }

    return EXITO;
}

//Función para leer el estado de un bloque en el mapa de bits, devolviendo 1 si el bloque está ocupado o 0 si está libre.
char leer_bit(unsigned int nbloque) {

    unsigned int nbloqueabs, posbyte, posbit;
    unsigned char bufferMB[BLOCKSIZE];

    if (obtener_posicion_bit(nbloque, &nbloqueabs, &posbyte, &posbit, bufferMB) == FALLO) {
        return FALLO;
    }

    unsigned char mascara = 128; //Se utiliza una máscara de bits para aislar el bit correspondiente al bloque que se desea leer, desplazando la máscara a la posición del bit dentro del byte del mapa de bits y aplicando una operación AND para obtener el valor del bit. Luego, se desplaza el resultado a la posición más baja para devolver 1 si el bloque está ocupado o 0 si está libre.
    mascara >>= posbit;
    mascara &= bufferMB[posbyte];
    mascara >>= (7 - posbit);

    if (DN3==1){
        fprintf(stderr, GRAY "[leer_bit(%u)]\n" RESET, nbloque);
    }

    return mascara;
}

//Función para reservar un bloque, buscando el primer bloque libre en el mapa de bits, marcándolo como ocupado y devolviendo su número. Si no hay bloques libres, devuelve FALLO.
int reservar_bloque() {

    struct superbloque SB;

    if (bread(posSB, &SB) == FALLO) {
        fprintf(stderr, RED "Error al leer superbloque\n" RESET);
        return FALLO;
    }

    if (SB.cantBloquesLibres == 0) {
        fprintf(stderr, RED "No hay bloques libres\n" RESET);
        return FALLO;
    }

    int nbloque = buscar_primer_bloque_libre(&SB);//Buscar el primer bloque libre en el mapa de bits utilizando la función auxiliar buscar_primer_bloque_libre(), que recorre los bloques del mapa de bits y los bytes dentro de cada bloque para encontrar el primer bit que esté a 0 (libre) y devuelve su número. Si no se encuentra ningún bloque libre, devuelve FALLO.
    if (nbloque == FALLO) return FALLO;

    escribir_bit(nbloque, 1);

    SB.cantBloquesLibres--;
    bwrite(posSB, &SB);

    unsigned char bufferCeros[BLOCKSIZE];
    memset(bufferCeros, 0, BLOCKSIZE);
    bwrite(nbloque, bufferCeros);

    if (DN3==1){
        fprintf(stderr, GRAY "[reservar_bloque(%u)→ reservado:%u]\n" RESET, nbloque, nbloque);
    }

    return nbloque;
}

//Función para liberar un bloque, marcando el bloque como libre en el mapa de bits y actualizando la cantidad de bloques libres en el superbloque.
int liberar_bloque(unsigned int nbloque) {

    struct superbloque SB;
    bread(posSB, &SB);

    escribir_bit(nbloque, 0);

    SB.cantBloquesLibres++;
    bwrite(posSB, &SB);

    if (DN3==1){
        fprintf(stderr, GRAY "[liberar_bloque(%u)]\n" RESET, nbloque);
    }

    return nbloque;
}

//Función para escribir un inodo en el array de inodos, dado su número de inodo y una estructura de inodo con la información a escribir. La función calcula la posición del bloque del array de inodos donde se encuentra el inodo a escribir, lee el bloque completo, actualiza la información del inodo correspondiente dentro del bloque y escribe el bloque actualizado de nuevo en el array de inodos.
int escribir_inodo(unsigned int ninodo, struct inodo *inodo) {

    struct superbloque SB;
    bread(posSB, &SB);

    unsigned int nbloqueAI = (ninodo * INODOSIZE) / BLOCKSIZE;
    unsigned int nbloqueabs = SB.posPrimerBloqueAI + nbloqueAI;

    struct inodo inodos[BLOCKSIZE / INODOSIZE];

    bread(nbloqueabs, inodos);

    unsigned int posinodo = ninodo % (BLOCKSIZE / INODOSIZE);

    inodos[posinodo] = *inodo;

    bwrite(nbloqueabs, inodos);

    return EXITO;
}

//Función para leer un inodo del array de inodos, dado su número de inodo y un puntero a una estructura de inodo donde se almacenará la información leída. La función calcula la posición del bloque del array de inodos donde se encuentra el inodo a leer, lee el bloque completo, extrae la información del inodo correspondiente dentro del bloque y la almacena en la estructura de inodo proporcionada.
int leer_inodo(unsigned int ninodo, struct inodo *inodo) {

    struct superbloque SB;
    bread(posSB, &SB);

    unsigned int nbloqueAI = (ninodo * INODOSIZE) / BLOCKSIZE;
    unsigned int nbloqueabs = SB.posPrimerBloqueAI + nbloqueAI;

    struct inodo inodos[BLOCKSIZE / INODOSIZE];

    bread(nbloqueabs, inodos);

    unsigned int posinodo = ninodo % (BLOCKSIZE / INODOSIZE);

    *inodo = inodos[posinodo];

    return EXITO;
}

//Función para reservar un inodo, buscando el primer inodo libre en el array de inodos, marcándolo como ocupado, actualizando la cantidad de inodos libres en el superbloque y devolviendo su número. Si no hay inodos libres, devuelve FALLO.
int reservar_inodo(unsigned char tipo, unsigned char permisos) {

    struct superbloque SB;
    bread(posSB, &SB);

    if (SB.cantInodosLibres == 0) {
        fprintf(stderr, RED "No hay inodos libres\n" RESET);
        return FALLO;
    }

    unsigned int posInodoReservado = SB.posPrimerInodoLibre;

    struct inodo inodo;
    memset(&inodo, 0, sizeof(struct inodo));
    leer_inodo(posInodoReservado, &inodo);

    SB.posPrimerInodoLibre = inodo.punterosDirectos[0];
    memset(inodo.punterosDirectos, 0, sizeof(inodo.punterosDirectos));
    memset(inodo.punterosIndirectos, 0, sizeof(inodo.punterosIndirectos));

    inodo.tipo = tipo;
    inodo.permisos = permisos;
    inodo.nlinks = 1;
    inodo.tamEnBytesLog = 0;
    inodo.numBloquesOcupados = 0;

    time_t ahora = time(NULL);
    inodo.atime = ahora;
    inodo.mtime = ahora;
    inodo.ctime = ahora;
    inodo.btime = ahora;

    escribir_inodo(posInodoReservado, &inodo);

    SB.cantInodosLibres--;
    bwrite(posSB, &SB);

    if (DN3==1){
        fprintf(stderr, GRAY "[reservar_inodo()→ reservado:%u]\n" RESET, posInodoReservado);
    }

    return posInodoReservado;
}

//Calcula la localización de un bit del MB dentro del dispositivo Devuelve EXITO si todo va bien
int obtener_posicion_bit(unsigned int nbloque,
                         unsigned int *nbloqueabs,
                         unsigned int *posbyte,
                         unsigned int *posbit,
                         unsigned char *bufferMB) {

    struct superbloque SB;

    if (bread(posSB, &SB) == FALLO) {
        fprintf(stderr, RED "Error al leer el superbloque\n" RESET);
        return FALLO;
    }

    unsigned int posbyteMB = nbloque / 8;
    *posbit = nbloque % 8;

    unsigned int nbloqueMB = posbyteMB / BLOCKSIZE;
    *nbloqueabs = SB.posPrimerBloqueMB + nbloqueMB;

    if (bread(*nbloqueabs, bufferMB) == FALLO) {
        fprintf(stderr, RED "Error al leer bloque del MB\n" RESET);
        return FALLO;
    }

    *posbyte = posbyteMB % BLOCKSIZE;

    return EXITO;
}

//Funcion para buscar el primer bloque libre en el mapa de bits, marcándolo como ocupado y devolviendo su número. Si no hay bloques libres, devuelve FALLO.
static int buscar_primer_bloque_libre(struct superbloque *SB) {

    unsigned char bufferMB[BLOCKSIZE];
    unsigned char bufferAux[BLOCKSIZE];
    memset(bufferAux, 255, BLOCKSIZE);

    unsigned int nbloqueMB = 0;

    while (nbloqueMB <= SB->posUltimoBloqueMB - SB->posPrimerBloqueMB) { //Recorrer los bloques del mapa de bits hasta encontrar un bloque que no esté completamente lleno (es decir, que no sea igual a 255 en todos sus bytes)

        if (bread(SB->posPrimerBloqueMB + nbloqueMB, bufferMB) == FALLO) {
            fprintf(stderr, RED "Error al leer MB\n" RESET);
            return FALLO;
        }

        if (memcmp(bufferMB, bufferAux, BLOCKSIZE) != 0)
            break;

        nbloqueMB++;
    }

    unsigned int posbyte = 0;
    while (posbyte < BLOCKSIZE && bufferMB[posbyte] == 255)
    posbyte++;

    if (posbyte == BLOCKSIZE) {
        fprintf(stderr, RED "Error: bloque MB lleno inesperadamente\n" RESET);
        return FALLO;
    }

    unsigned char mascara = 128;
    unsigned int posbit = 0;

    unsigned char aux = bufferMB[posbyte];

    while (aux & mascara) {
        aux <<= 1;
        posbit++;
    }

    return (nbloqueMB * BLOCKSIZE + posbyte) * 8 + posbit;
}

//Función para obtener el número de bloque físico correspondiente a un bloque lógico de un inodo, dado el número de inodo, el número de bloque lógico y un puntero a una variable donde se almacenará el número de bloque físico. La función determina el rango de bloques (directos, indirectos simples, indirectos dobles o indirectos triples) al que pertenece el bloque lógico, obtiene el puntero correspondiente del inodo y devuelve el número de bloque físico. Si el bloque lógico no existe o no está asignado, devuelve FALLO.
int obtener_nRangoBL(struct inodo inodo, unsigned int nblogico, unsigned int *ptr){

    if (nblogico < DIRECTOS){
        *ptr = inodo.punterosDirectos[nblogico];
        return 0;
    }
    else if (nblogico < INDIRECTOS0){
        *ptr = inodo.punterosIndirectos[0];
        return 1;
    }
    else if (nblogico < INDIRECTOS1){
        *ptr = inodo.punterosIndirectos[1];
        return 2;
    }
    else if (nblogico < INDIRECTOS2){
        *ptr = inodo.punterosIndirectos[2];
        return 3;
    }
    else{
        *ptr = 0;
        return FALLO;
    }
}

//Función para obtener el índice dentro de un bloque de punteros correspondiente a un bloque lógico de un inodo, dado el número de bloque lógico y el nivel de punteros (1 para indirectos simples, 2 para indirectos dobles y 3 para indirectos triples). La función calcula el índice utilizando divisiones y módulos según el nivel de punteros y devuelve el índice correspondiente. Si el bloque lógico no pertenece a ningún rango válido, devuelve FALLO.
int obtener_indice(unsigned int nblogico, int nivel_punteros){

    if (nblogico < DIRECTOS){
        return nblogico;
    }

    else if (nblogico < INDIRECTOS0){
        return nblogico - DIRECTOS;
    }

    else if (nblogico < INDIRECTOS1){

        if (nivel_punteros == 2){
            return (nblogico - INDIRECTOS0) / NPUNTEROS;
        }

        else if (nivel_punteros == 1){
            return (nblogico - INDIRECTOS0) % NPUNTEROS;
        }
    }

    else if (nblogico < INDIRECTOS2){

        if (nivel_punteros == 3){
            return (nblogico - INDIRECTOS1) / (NPUNTEROS * NPUNTEROS);
        }

        else if (nivel_punteros == 2){
            return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) / NPUNTEROS;
        }

        else if (nivel_punteros == 1){
            return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) % NPUNTEROS;
        }
    }

    return FALLO;
}

int traducir_bloque_inodo(unsigned int ninodo, unsigned int nblogico, unsigned char reservar){

struct inodo inodo;
unsigned int ptr = 0;
unsigned int ptr_ant = 0;
unsigned int indice = 0;

int nRangoBL;
int nivel_punteros;

unsigned int buffer[NPUNTEROS];

if (leer_inodo(ninodo, &inodo) == FALLO){
    fprintf(stderr, RED "Error leyendo inodo\n" RESET);
    return FALLO;
}

nRangoBL = obtener_nRangoBL(inodo, nblogico, &ptr);
nivel_punteros = nRangoBL;

if (nRangoBL == 0){

    if (ptr == 0){
        if (reservar == 0){
            return FALLO;
        }

        ptr = reservar_bloque();

        inodo.numBloquesOcupados++;
        inodo.ctime = time(NULL);

        inodo.punterosDirectos[nblogico] = ptr;

        if (DN4==1){
            fprintf(stderr, GRAY "[traducir_bloque_inodo()→ inodo.punterosDirectos[%u] = %u (reservado BF %u para BL %u)]\n" RESET, nblogico, ptr, ptr, nblogico);
        }
    }

} else {

while (nivel_punteros > 0){

        if (ptr == 0){

            if (reservar == 0){
                return FALLO;
            }

            ptr = reservar_bloque();

            inodo.numBloquesOcupados++;
            inodo.ctime = time(NULL);

            if (nivel_punteros == nRangoBL){

                inodo.punterosIndirectos[nRangoBL - 1] = ptr;

                if (DN4==1){
                    fprintf(stderr, GRAY "[traducir_bloque_inodo()→ inodo.punterosIndirectos[%d] = %u (reservado BF %u para punteros_nivel%d)]\n" RESET, nRangoBL - 1, ptr, ptr, nivel_punteros);
                }

            } else {

                if (bread(ptr_ant, buffer) == FALLO){
                    fprintf(stderr, RED "Error leyendo bloque de punteros\n" RESET);
                    return FALLO;
                }

                buffer[indice] = ptr;

                if (bwrite(ptr_ant, buffer) == FALLO){
                    fprintf(stderr, RED "Error escribiendo bloque de punteros\n" RESET);
                    return FALLO;
                }

                if (DN4==1){
                    fprintf(stderr, GRAY "[traducir_bloque_inodo()→ punteros_nivel%d [%u] = %u (reservado BF %u)]\n" RESET, nivel_punteros + 1, indice, ptr, ptr);
                }
            }
            
            // ¡CLAVE AQUI! 
            // Inicializar a ceros el nuevo bloque en disco y dejar el buffer a 0.
            memset(buffer, 0, BLOCKSIZE);
            if (bwrite(ptr, buffer) == FALLO) {
                fprintf(stderr, RED "Error inicializando a 0s el bloque de punteros\n" RESET);
                return FALLO;
            }

        } else {

            if (bread(ptr, buffer) == FALLO){
                fprintf(stderr, RED "Error leyendo bloque de punteros\n" RESET);
                return FALLO;
            }
        }

        indice = obtener_indice(nblogico, nivel_punteros);
        ptr_ant = ptr;
        
        ptr = buffer[indice]; 

        nivel_punteros--;
    }

    if (ptr == 0){

        if (reservar == 0){
            return FALLO;
        }

        ptr = reservar_bloque();

        inodo.numBloquesOcupados++;
        inodo.ctime = time(NULL);

        if (bread(ptr_ant, buffer) == FALLO){
            fprintf(stderr, RED "Error leyendo bloque de punteros\n" RESET);
            return FALLO;
        }

        buffer[indice] = ptr;

        if (bwrite(ptr_ant, buffer) == FALLO){
            fprintf(stderr, RED "Error escribiendo bloque de punteros\n" RESET);
            return FALLO;
        }

        if (DN4==1){
            fprintf(stderr, GRAY "[traducir_bloque_inodo()→ punteros_nivel1 [%u] = %u (reservado BF %u para BL %u)]\n" RESET, indice, ptr, ptr, nblogico);
        }
    }
}

if (escribir_inodo(ninodo, &inodo) == FALLO){
    fprintf(stderr, RED "Error escribiendo inodo\n" RESET);
    return FALLO;
}

return ptr;

}

//Función para liberar los bloques ocupados por un inodo, dado el número de bloque lógico del primer bloque a liberar y un puntero a la estructura de inodo. La función recorre los bloques lógicos del inodo desde el primer bloque lógico especificado hasta el último bloque lógico ocupado, utilizando las funciones auxiliares obtener_nRangoBL() y obtener_indice() para determinar el rango de bloques y el índice dentro del bloque de punteros. Para cada bloque lógico ocupado, se libera el bloque de datos correspondiente y se actualizan los punteros en el inodo y en los bloques de punteros según sea necesario. La función devuelve la cantidad total de bloques liberados o FALLO si ocurre un error.
int liberar_bloques_inodo(unsigned int primerBL, struct inodo *inodo) {
    if (inodo->tamEnBytesLog == 0) return 0;

    unsigned int ultimoBL;
    if (inodo->tamEnBytesLog == 0)
        ultimoBL = 0;
    else
        ultimoBL = (inodo->tamEnBytesLog - 1) / BLOCKSIZE;

    if (DN6==1){
        fprintf(stderr, GRAY "[liberar_bloques_inodo()→ primer BL: %d, último BL: %d]\n" RESET, primerBL, ultimoBL);
    }

    int liberados = 0;
    int total_breads = 0;
    int total_bwrites = 0;

    unsigned int ptr;
    int nRangoBL, nivel_punteros, indice;

    unsigned int bloques_punteros[3][NPUNTEROS];
    unsigned int ptr_nivel[3];
    int indices[3];
    int modificados[3] = {0, 0, 0};

    unsigned char bufferCeros[BLOCKSIZE];
    memset(bufferCeros, 0, BLOCKSIZE);

    unsigned int nBL = primerBL;

    while (nBL <= ultimoBL) {
        nRangoBL = obtener_nRangoBL(*inodo, nBL, &ptr);
        if (nRangoBL < 0) return FALLO;

        // Puntero de más alto nivel a 0
        if (ptr == 0) {
            unsigned int salto = 0;
            if (nBL < DIRECTOS) {
                nBL++;
                continue;
            }
            else if (nBL < INDIRECTOS0) salto = INDIRECTOS0;
            else if (nBL < INDIRECTOS1) salto = INDIRECTOS1;
            else if (nBL < INDIRECTOS2) salto = INDIRECTOS2;
            else salto = ultimoBL + 1;

            if (salto > ultimoBL + 1) salto = ultimoBL + 1;

            if (DN6==1){
                fprintf(stderr, GRAY "[liberar_bloques_inodo()→ Saltamos del BL %d al BL %d\n" RESET, nBL, salto - 1);
            }

            nBL = salto;
            continue;
        }

        nivel_punteros = nRangoBL;

        // Bajamos por los niveles de indirección
        while (nivel_punteros > 0) {
            indice = obtener_indice(nBL, nivel_punteros);

            if (indice == 0 || nBL == primerBL) {
                bread(ptr, bloques_punteros[nivel_punteros - 1]);
                total_breads++;
            }

            ptr_nivel[nivel_punteros - 1] = ptr;
            indices[nivel_punteros - 1] = indice;

            ptr = bloques_punteros[nivel_punteros - 1][indice];

            // Si un puntero intermedio es 0
            if (ptr == 0) {
                // ANTES de saltar, volcamos los bloques modificados porque saldremos de su rango
                for (int n = 1; n <= nRangoBL; n++) {
                    if (modificados[n - 1]) {
                        unsigned int ptr_actual = ptr_nivel[n - 1];
                        if (memcmp(bloques_punteros[n - 1], bufferCeros, BLOCKSIZE) == 0) {
                            liberar_bloque(ptr_actual);
                            liberados++;
                            if (DN6==1) fprintf(stderr, GRAY "[liberar_bloques_inodo()→ liberado BF %d de punteros_nivel%d correspondiente al BL %d]\n" RESET, ptr_actual, n, nBL - 1);
                            
                            if (n == nRangoBL) {
                                inodo->punterosIndirectos[nRangoBL - 1] = 0;
                            } else {
                                bloques_punteros[n][indices[n]] = 0;
                                modificados[n] = 1;
                            }
                        } else {
                            bwrite(ptr_actual, bloques_punteros[n - 1]);
                            total_bwrites++;
                            if (DN6==1) fprintf(stderr, GRAY "[liberar_bloques_inodo()→ salvado BF %d de punteros_nivel%d correspondiente al BL %d]\n" RESET, ptr_actual, n, nBL - 1);
                        }
                        modificados[n - 1] = 0;
                    }
                }

                // Contar ceros inteligentemente y calcular el salto exacto considerando la desalineación
                int zeros = 0;
                while (indice + zeros < NPUNTEROS && bloques_punteros[nivel_punteros - 1][indice + zeros] == 0) {
                    zeros++;
                }

                int blocks_per_pointer = 1;
                if (nivel_punteros == 2) blocks_per_pointer = NPUNTEROS;
                else if (nivel_punteros == 3) blocks_per_pointer = NPUNTEROS * NPUNTEROS;

                int base_offset = 0;
                if (nRangoBL == 1) base_offset = nBL - DIRECTOS;
                else if (nRangoBL == 2) base_offset = nBL - INDIRECTOS0;
                else if (nRangoBL == 3) base_offset = nBL - INDIRECTOS1;

                unsigned int salto = nBL - (base_offset % blocks_per_pointer) + zeros * blocks_per_pointer;

                if (salto > ultimoBL + 1) salto = ultimoBL + 1;

                if (DN6==1){
                    fprintf(stderr, GRAY "[liberar_bloques_inodo()→ Saltamos del BL %d al BL %d\n" RESET, nBL, salto - 1);
                }

                nBL = salto;
                goto saltar_incremento;
            }

            nivel_punteros--;
        }

        // Si existe bloque de datos, lo liberamos
        if (ptr > 0) {
            liberar_bloque(ptr);
            liberados++;

            if (DN6==1){
                fprintf(stderr, GRAY "[liberar_bloques_inodo()→ liberado BF %d de datos para BL %d]\n" RESET, ptr, nBL);
            }

            if (nRangoBL == 0) {
                inodo->punterosDirectos[nBL] = 0;
            } else {
                bloques_punteros[0][indices[0]] = 0;
                modificados[0] = 1;

                // Evaluamos de abajo a arriba si toca consolidar por fin de bloque o fin de fichero
                for (int n = 1; n <= nRangoBL; n++) {
                    if (modificados[n - 1] && (indices[n - 1] == NPUNTEROS - 1 || nBL == ultimoBL)) {
                        unsigned int ptr_actual = ptr_nivel[n - 1];
                        
                        if (memcmp(bloques_punteros[n - 1], bufferCeros, BLOCKSIZE) == 0) {
                            liberar_bloque(ptr_actual);
                            liberados++;
                            if (DN6==1) fprintf(stderr, GRAY "[liberar_bloques_inodo()→ liberado BF %d de punteros_nivel%d correspondiente al BL %d]\n" RESET, ptr_actual, n, nBL);
                            
                            if (n == nRangoBL) {
                                inodo->punterosIndirectos[nRangoBL - 1] = 0;
                            } else {
                                bloques_punteros[n][indices[n]] = 0;
                                modificados[n] = 1;
                            }
                        } else {
                            bwrite(ptr_actual, bloques_punteros[n - 1]);
                            total_bwrites++;
                            if (DN6==1) fprintf(stderr, GRAY "[liberar_bloques_inodo()→ salvado BF %d de punteros_nivel%d correspondiente al BL %d]\n" RESET, ptr_actual, n, nBL);
                        }
                        modificados[n - 1] = 0;
                    } else {
                        // Si el nivel actual no se vacía, los superiores tampoco
                        break;
                    }
                }
            }
        }

        nBL++;
    saltar_incremento:
        ;
    }

    if (DN6==1){
        fprintf(stderr, GRAY "[liberar_bloques_inodo()→ total bloques liberados: %d, total_breads: %d, total_bwrites: %d]\n" RESET, liberados, total_breads, total_bwrites);
    }

    return liberados;
}

//Función para liberar un inodo, dado su número de inodo. La función lee el inodo correspondiente, libera los bloques ocupados por el inodo utilizando la función liberar_bloques_inodo(), actualiza la información del inodo para marcarlo como libre, inserta el inodo liberado en la lista de inodos libres del superbloque y actualiza el superbloque en el dispositivo. Finalmente, devuelve el número de inodo liberado o FALLO si ocurre un error.
int liberar_inodo(unsigned int ninodo) {

    struct inodo inodo;
    struct superbloque SB;

    if (leer_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error al leer inodo\n" RESET);
        return FALLO;
    }

    int liberados = liberar_bloques_inodo(0, &inodo);

    inodo.numBloquesOcupados -= liberados;
    inodo.tamEnBytesLog = 0;
    inodo.tipo = 'l';

    if (bread(posSB, &SB) == FALLO) {
        fprintf(stderr, RED "Error al leer SB\n" RESET);
        return FALLO;
    }

    //insertar en lista de libres
    inodo.punterosDirectos[0] = SB.posPrimerInodoLibre;
    SB.posPrimerInodoLibre = ninodo;
    SB.cantInodosLibres++;

    inodo.ctime = time(NULL);

    if (bwrite(posSB, &SB) == FALLO) {
        fprintf(stderr, RED "Error al escribir SB\n" RESET);
        return FALLO;
    }

    if (escribir_inodo(ninodo, &inodo) == FALLO) {
        fprintf(stderr, RED "Error al escribir inodo\n" RESET);
        return FALLO;
    }

    if (DN6==1){
        fprintf(stderr, GRAY "[liberar_inodo()→ inodo %d liberado]\n" RESET, ninodo);
    }

    struct inodo inodo_leido;

    if (leer_inodo(ninodo, &inodo_leido) == FALLO) {
        fprintf(stderr, RED "Error leyendo inodo tras liberar\n" RESET);
        return FALLO;
    }

    return ninodo;
}