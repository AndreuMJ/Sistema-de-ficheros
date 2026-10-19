//Hecho por Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame
#include "directorios.h"
#include <string.h>
#include <stdio.h>
#if (USARCACHE == 2 || USARCACHE == 3)
static struct UltimaEntrada UltimasEntradas[CACHE_SIZE];
#endif

//Funcion extraer_camino() que se encarga de extraer el primer componente del camino y el resto del camino, así como el tipo (directorio o fichero) del primer componente. Devuelve 0 si el camino es correcto y -1 si el camino es incorrecto.
int extraer_camino(const char *camino, char *inicial, char *final, char *tipo) {
    if (camino[0] != '/') {
        return ERROR_CAMINO_INCORRECTO;
    }

    // Saltamos la primera barra
    const char *resto = camino + 1;
    // Buscamos la siguiente barra
    const char *barra = strchr(resto, '/');

    if (barra != NULL) {
        // Hay una barra, por lo tanto inicial es un directorio
        strncpy(inicial, resto, barra - resto);
        inicial[barra - resto] = '\0';
        strcpy(final, barra);
        *tipo = 'd';
    } else {
        // No hay barra, por lo tanto inicial es un fichero
        strcpy(inicial, resto);
        strcpy(final, "");
        *tipo = 'f';
    }

    return EXITO;
}
//Funcion buscar_entrada() recursiva, que se encarga de buscar una entrada en un directorio dado su camino parcial. Devuelve 0 si la entrada existe o se ha creado correctamente, y -1 si ha ocurrido un error.
int buscar_entrada(const char *camino_parcial, unsigned int *p_inodo_dir, unsigned int *p_inodo, unsigned int *p_entrada, char reservar, unsigned char permisos) {
    struct entrada entrada;
    struct inodo inodo_dir;
    char inicial[sizeof(entrada.nombre)];
    char final[strlen(camino_parcial) + 1];
    char tipo;
    int cant_entradas_inodo, num_entrada_inodo;
    int encontrada = 0; // 0 = no encontrada, 1 = encontrada

    // Si es el directorio raíz
    if (strcmp(camino_parcial, "/") == 0) {
        *p_inodo = 0; // El inodo raíz siempre es el 0
        *p_entrada = 0;
        return EXITO;
    }

    memset(inicial, 0, sizeof(inicial));
    memset(final, 0, sizeof(final));

    int error = extraer_camino(camino_parcial, inicial, final, &tipo);
    if (error < 0) return error;

    if (DN7==1) {
        fprintf(stderr, GRAY "[buscar_entrada()→ inicial: %s, final: %s, reservar: %d]\n" RESET, inicial, final, reservar);
    }

    // Leemos el inodo del directorio padre
    if (leer_inodo(*p_inodo_dir, &inodo_dir) == FALLO) {
        return FALLO;
    }

    // Comprobamos permisos de lectura del inodo directorio (Permiso de lectura: 4)
    if ((inodo_dir.permisos & 4) != 4) {
        return ERROR_PERMISO_LECTURA;
    }

    // Buffer para optimizar la lectura de un bloque entero de entradas
    struct entrada buffer_entradas[BLOCKSIZE / sizeof(struct entrada)];
    memset(buffer_entradas, 0, BLOCKSIZE);

    cant_entradas_inodo = inodo_dir.tamEnBytesLog / sizeof(struct entrada); // Número total de entradas en el directorio
    num_entrada_inodo = 0;

    // Búsqueda lineal de la entrada
    while (num_entrada_inodo < cant_entradas_inodo && encontrada == 0) {
        // Leemos el bloque solo cuando hace falta usando el operador '%'
        if (num_entrada_inodo % (BLOCKSIZE / sizeof(struct entrada)) == 0) {
            mi_read_f(*p_inodo_dir, buffer_entradas, num_entrada_inodo * sizeof(struct entrada), BLOCKSIZE);
        }
        
        // Comparamos el nombre en la posición correspondiente del buffer
        if (strcmp(inicial, buffer_entradas[num_entrada_inodo % (BLOCKSIZE / sizeof(struct entrada))].nombre) == 0) { 
            encontrada = 1;
            *p_entrada = num_entrada_inodo;
            entrada = buffer_entradas[num_entrada_inodo % (BLOCKSIZE / sizeof(struct entrada))];
        } else {
            num_entrada_inodo++;
        }
    }

    // Si la entrada no existe
    if (encontrada == 0 && num_entrada_inodo == cant_entradas_inodo) {
        switch (reservar) {
            case 0: // Modo consulta, como no existe retornamos error
                return ERROR_NO_EXISTE_ENTRADA_CONSULTA;
            case 1: // Modo escritura
                if (inodo_dir.tipo == 'f') {
                    return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO;
                }
                // Comprobamos permisos de escritura del inodo directorio (Permiso de escritura: 2)
                if ((inodo_dir.permisos & 2) != 2) {
                    return ERROR_PERMISO_ESCRITURA;
                }

                strcpy(entrada.nombre, inicial);

                if (tipo == 'd') {
                    if (strcmp(final, "/") == 0) {
                        entrada.ninodo = reservar_inodo('d', permisos);
                        if(DN7==1) fprintf(stderr, GRAY "[buscar_entrada()→ reservado inodo %d tipo d con permisos %d para %s]\n" RESET, entrada.ninodo, permisos, inicial);
                    } else {
                        return ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO;
                    }
                } else { // Es un fichero ('f')
                    entrada.ninodo = reservar_inodo('f', permisos);
                    if(DN7==1) fprintf(stderr, GRAY "[buscar_entrada()→ reservado inodo %d tipo f con permisos %d para %s]\n" RESET, entrada.ninodo, permisos, inicial);
                }

                // Escribir la entrada en el directorio padre por el final
                if (mi_write_f(*p_inodo_dir, &entrada, num_entrada_inodo * sizeof(struct entrada), sizeof(struct entrada)) < 0) {
                    if (entrada.ninodo != -1) {
                        liberar_inodo(entrada.ninodo);
                    }
                    return FALLO;
                }

                if(DN7==1) fprintf(stderr, GRAY "[buscar_entrada()→ creada entrada: %s, %d]\n" RESET, inicial, entrada.ninodo);
                break;
        }
    }

    // Evaluar si hemos llegado al final o debemos seguir bajando por los directorios
    if (strcmp(final, "/") == 0 || strcmp(final, "") == 0) { // Hemos llegado al final del camino
        if (encontrada == 1 && reservar == 1) {
            // Modo escritura y la entrada ya existe
            return ERROR_ENTRADA_YA_EXISTENTE;
        }
        // Cortamos la recursividad devolviendo los punteros
        *p_inodo = entrada.ninodo;
        *p_entrada = num_entrada_inodo;
        return EXITO;
    } else { // Queda camino por explorar
        *p_inodo_dir = entrada.ninodo; // El inodo del directorio que acabamos de encontrar/crear
        return buscar_entrada(final, p_inodo_dir, p_inodo, p_entrada, reservar, permisos);
    }
}

// Función auxiliar para mostrar los errores en rojo por consola
void mostrar_error_buscar_entrada(int error) {
    switch (error) {
        case ERROR_CAMINO_INCORRECTO: 
            fprintf(stderr, RED "Error: Camino incorrecto.\n" RESET); 
            break;
        case ERROR_PERMISO_LECTURA: 
            fprintf(stderr, RED "Error: Permiso denegado de lectura.\n" RESET); 
            break;
        case ERROR_NO_EXISTE_ENTRADA_CONSULTA: 
            fprintf(stderr, RED "Error: No existe el archivo o el directorio.\n" RESET); 
            break;
        case ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO: 
            fprintf(stderr, RED "Error: No existe algún directorio intermedio.\n" RESET); 
            break;
        case ERROR_PERMISO_ESCRITURA: 
            fprintf(stderr, RED "Error: Permiso denegado de escritura.\n" RESET); 
            break;
        case ERROR_ENTRADA_YA_EXISTENTE: 
            fprintf(stderr, RED "Error: El archivo ya existe.\n" RESET); 
            break;
        case ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO: 
            fprintf(stderr, RED "Error: No es un directorio.\n" RESET); 
            break;
    }
}

// Crea un fichero o directorio llamando a buscar_entrada con reservar=1
int mi_creat(const char *camino, unsigned char permisos) {
    mi_waitSem();
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;
    if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 1, permisos)) < 0) {
        mi_signalSem();
        return error;
    }
    mi_signalSem();
    return EXITO;
}

// Pone el contenido del directorio o fichero en un buffer de memoria dado su camino, el tipo (directorio o fichero) y un flag para mostrar información adicional. Devuelve el número de entradas listadas o -1 si ha ocurrido un error.
int mi_dir(const char *camino, char *buffer, char tipo, char flag) {
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    int error, num_entradas = 0;

    if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 4)) < 0) return error;

    struct inodo inodo;
    if (leer_inodo(p_inodo, &inodo) == FALLO) return FALLO;

    if ((inodo.tipo == 'd' && tipo != 'd') || (inodo.tipo == 'f' && tipo != 'f')) {
        fprintf(stderr, RED "Error: la sintaxis no concuerda con el tipo.\n" RESET);
        return FALLO;
    }

    if (inodo.tipo == 'd') {
        if ((inodo.permisos & 4) != 4) return ERROR_PERMISO_LECTURA;

        struct entrada entradas[BLOCKSIZE / sizeof(struct entrada)];
        int offset = 0;
        int cant_entradas_inodo = inodo.tamEnBytesLog / sizeof(struct entrada);

        while (num_entradas < cant_entradas_inodo) {
            if (mi_read_f(p_inodo, entradas, offset, BLOCKSIZE) < 0) return FALLO;
            
            for (int i = 0; i < (BLOCKSIZE / sizeof(struct entrada)) && num_entradas < cant_entradas_inodo; i++) {
                if (entradas[i].ninodo != -1) {
                    struct inodo inodo_ent;
                    leer_inodo(entradas[i].ninodo, &inodo_ent);
                    
                    char tmp[500]; 
                    char color[50];
                    if (inodo_ent.tipo == 'd') strcpy(color, LBLUE); else strcpy(color, WHITE);

                    if (flag) {
                        char permisos_str[4] = "---";
                        if (inodo_ent.permisos & 4) permisos_str[0] = 'r';
                        if (inodo_ent.permisos & 2) permisos_str[1] = 'w';
                        if (inodo_ent.permisos & 1) permisos_str[2] = 'x';

                        struct tm *tm = localtime(&inodo_ent.mtime);
                        char tiempo[200]; 
                        sprintf(tiempo, "%d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

                        sprintf(tmp, "%c\t%s\t%s\t\t%d\t\t\t%s%s" RESET "\n", inodo_ent.tipo, permisos_str, tiempo, inodo_ent.tamEnBytesLog, color, entradas[i].nombre);
                        strcat(buffer, tmp);
                    } else {
                        sprintf(tmp, "%s%s" RESET "\t", color, entradas[i].nombre);
                        strcat(buffer, tmp);
                    }
                    num_entradas++;
                }
            }
            offset += BLOCKSIZE;
        }
        if (!flag && num_entradas > 0) strcat(buffer, "\n");
        
    } else {
        char tmp[500];
        char *nombre = strrchr(camino, '/');
        if (nombre != NULL) nombre++; else nombre = (char *)camino;

        if (flag) {
            char permisos_str[4] = "---";
            if (inodo.permisos & 4) permisos_str[0] = 'r';
            if (inodo.permisos & 2) permisos_str[1] = 'w';
            if (inodo.permisos & 1) permisos_str[2] = 'x';

            struct tm *tm = localtime(&inodo.mtime);
            char tiempo[200]; 
            sprintf(tiempo, "%d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

            sprintf(tmp, "%c\t%s\t%s\t\t%d\t\t\t%s%s" RESET "\n", inodo.tipo, permisos_str, tiempo, inodo.tamEnBytesLog, WHITE, nombre);
            strcat(buffer, tmp);
        } else {
            sprintf(tmp, "%s%s" RESET "\n", WHITE, nombre);
            strcat(buffer, tmp);
        }
        num_entradas = 1;
    }
    return num_entradas;
}


// Cambia los permisos de un inodo dada su ruta
int mi_chmod(const char *camino, unsigned char permisos) {
    unsigned int p_inodo_dir = 0;
    unsigned int p_inodo = 0;
    unsigned int p_entrada = 0;
    int error;

    if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, permisos)) < 0) {
        return error;
    }
    return mi_chmod_f(p_inodo, permisos);
}

// Lee los metadatos de un inodo dada su ruta
int mi_stat(const char *camino, struct STAT *p_stat) {
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    int error;
    if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0)) < 0) return error;
    if (mi_stat_f(p_inodo, p_stat) == FALLO) return FALLO;
    return p_inodo; 
}

// Función de directorios.c para escribir contenido en un fichero
int mi_write(const char *camino, const void *buf, unsigned int offset, unsigned int nbytes) {
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    int error, cache_hit = 0;

    // Buscar si el camino está en caché
    for (int i = 0; i < CACHE_SIZE; i++) {
        if (strcmp(camino, UltimasEntradas[i].camino) == 0) {
            p_inodo = UltimasEntradas[i].p_inodo;
            cache_hit = 1;
            #if USARCACHE == 3
            gettimeofday(&UltimasEntradas[i].ultima_consulta, NULL);
            #endif
            if (DN9 == 1) fprintf(stderr, GRAY "[mi_write() → Utilizamos cache[%d]: %s]\n" RESET, i, camino);
            break;
        }
    }

    // Si no está en caché, buscamos la entrada y la guardamos
    if (!cache_hit) {
        error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 4);
        if (error < 0) {
            mostrar_error_buscar_entrada(error);
            return 0; // Devolvemos 0 bytes escritos si hay error
        }

        int pos_reemplazo = 0;

        #if USARCACHE == 2
        // Estrategia FIFO
        static int fifo_idx_w = 0; // Contador estático que persiste entre llamadas
        pos_reemplazo = fifo_idx_w;
        fifo_idx_w = (fifo_idx_w + 1) % CACHE_SIZE; // Movimiento circular
        
        #elif USARCACHE == 3
        // Estrategia LRU
        for (int i = 1; i < CACHE_SIZE; i++) {
            if (UltimasEntradas[i].ultima_consulta.tv_sec < UltimasEntradas[pos_reemplazo].ultima_consulta.tv_sec ||
               (UltimasEntradas[i].ultima_consulta.tv_sec == UltimasEntradas[pos_reemplazo].ultima_consulta.tv_sec &&
                UltimasEntradas[i].ultima_consulta.tv_usec < UltimasEntradas[pos_reemplazo].ultima_consulta.tv_usec)) {
                pos_reemplazo = i;
            }
        }
        #endif

        strcpy(UltimasEntradas[pos_reemplazo].camino, camino);
        UltimasEntradas[pos_reemplazo].p_inodo = p_inodo;
        #if USARCACHE == 3
        gettimeofday(&UltimasEntradas[pos_reemplazo].ultima_consulta, NULL);
        #endif

        if (DN9 == 1) fprintf(stderr, GRAY "[mi_write() → Reemplazamos cache[%d]: %s]\n" RESET, pos_reemplazo, camino);
    }

    return mi_write_f(p_inodo, buf, offset, nbytes);
}

// Función de directorios.c para leer contenido de un fichero
int mi_read(const char *camino, void *buf, unsigned int offset, unsigned int nbytes) {
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    int error, cache_hit = 0;

    for (int i = 0; i < CACHE_SIZE; i++) {
        if (strcmp(camino, UltimasEntradas[i].camino) == 0) {
            p_inodo = UltimasEntradas[i].p_inodo;
            cache_hit = 1;
            #if USARCACHE == 3
            gettimeofday(&UltimasEntradas[i].ultima_consulta, NULL);
            #endif
            if (DN9 == 1) fprintf(stderr, GRAY "[mi_read() → Utilizamos cache[%d]: %s]\n" RESET, i, camino);
            break;
        }
    }

    if (!cache_hit) {
        error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 4);
        if (error < 0) {
            mostrar_error_buscar_entrada(error);
            return 0; // Devolvemos 0 bytes leídos si hay error
        }

        int pos_reemplazo = 0;

        #if USARCACHE == 2
        // Estrategia FIFO
        static int fifo_idx_r = 0;
        pos_reemplazo = fifo_idx_r;
        fifo_idx_r = (fifo_idx_r + 1) % CACHE_SIZE;

        #elif USARCACHE == 3
        // Estrategia LRU
        for (int i = 1; i < CACHE_SIZE; i++) {
            if (UltimasEntradas[i].ultima_consulta.tv_sec < UltimasEntradas[pos_reemplazo].ultima_consulta.tv_sec ||
               (UltimasEntradas[i].ultima_consulta.tv_sec == UltimasEntradas[pos_reemplazo].ultima_consulta.tv_sec &&
                UltimasEntradas[i].ultima_consulta.tv_usec < UltimasEntradas[pos_reemplazo].ultima_consulta.tv_usec)) {
                pos_reemplazo = i;
            }
        }
        #endif

        strcpy(UltimasEntradas[pos_reemplazo].camino, camino);
        UltimasEntradas[pos_reemplazo].p_inodo = p_inodo;
        #if USARCACHE == 3
        gettimeofday(&UltimasEntradas[pos_reemplazo].ultima_consulta, NULL);
        #endif

        if (DN9 == 1) fprintf(stderr, GRAY "[mi_read() → Reemplazamos cache[%d]: %s]\n" RESET, pos_reemplazo, camino);
    }

    return mi_read_f(p_inodo, buf, offset, nbytes);
}

int mi_link(const char *camino1, const char *camino2) {
    mi_waitSem();
    unsigned int p_inodo_dir1 = 0, p_inodo1 = 0, p_entrada1 = 0;
    unsigned int p_inodo_dir2 = 0, p_inodo2 = 0, p_entrada2 = 0;
    int error;
    struct inodo inodo1;

    //Comprobar que camino1 existe y tiene permisos de lectura
    error = buscar_entrada(camino1, &p_inodo_dir1, &p_inodo1, &p_entrada1, 0, 4);
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        mi_signalSem();
        return error;
    }

    //Leemos el inodo1 para comprobar que es un fichero
    if (leer_inodo(p_inodo1, &inodo1) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    if (inodo1.tipo != 'f') {
        fprintf(stderr, RED "Error: camino1 no es un fichero (no se enlazan directorios)\n" RESET);
        mi_signalSem();
        return FALLO;
    }

    //Comprobar que camino2 no existe y crearlo
    error = buscar_entrada(camino2, &p_inodo_dir2, &p_inodo2, &p_entrada2, 1, 6);
    if (error < 0) { //Si buscar_entrada devuelve error, es que el archivo ya existía o hubo otro fallo
        mostrar_error_buscar_entrada(error);
        mi_signalSem();
        return error;
    }

    //Crear el enlace
    struct entrada entrada2;
    //Leemos la entrada creada en camino2
    if (mi_read_f(p_inodo_dir2, &entrada2, p_entrada2 * sizeof(struct entrada), sizeof(struct entrada)) < 0) {
        mi_signalSem();
        return FALLO;
    }

    //Asociamos a esta entrada el inodo del camino1
    entrada2.ninodo = p_inodo1;

    //Escribimos la entrada modificada
    if (mi_write_f(p_inodo_dir2, &entrada2, p_entrada2 * sizeof(struct entrada), sizeof(struct entrada)) < 0) {
        mi_signalSem();
        return FALLO;
    }

    //Liberar el inodo artificialmente creado para camino2
    if (liberar_inodo(p_inodo2) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    //Actualizar los nlinks y ctime del inodo1
    inodo1.nlinks++;
    inodo1.ctime = time(NULL);
    if (escribir_inodo(p_inodo1, &inodo1) == FALLO) {
        mi_signalSem();
        return FALLO;
    }
    mi_signalSem();
    return EXITO;
}

int mi_unlink(const char *camino) {
    mi_waitSem();
    unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
    int error;
    struct inodo inodo, inodo_dir;

    //Comprobar que camino existe
    error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 4);
    if (error < 0) {
        mostrar_error_buscar_entrada(error);
        mi_signalSem();
        return error;
    }

    //Leemos el inodo a eliminar
    if (leer_inodo(p_inodo, &inodo) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    //Si es un directorio, comprobar que esté vacío
    if (inodo.tipo == 'd' && inodo.tamEnBytesLog > 0) {
        fprintf(stderr, RED "Error: El directorio %s no está vacío\n" RESET, camino);
        mi_signalSem();
        return FALLO;
    }

    //Leemos el inodo del directorio padre
    if (leer_inodo(p_inodo_dir, &inodo_dir) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    //Obtener el nº de entradas del directorio
    int num_entradas = inodo_dir.tamEnBytesLog / sizeof(struct entrada);

    //Compactar el directorio (si la entrada a eliminar no es la última)
    if (p_entrada != num_entradas - 1) {
        struct entrada ultima_entrada;
        //Leemos la última entrada
        if (mi_read_f(p_inodo_dir, &ultima_entrada, (num_entradas - 1) * sizeof(struct entrada), sizeof(struct entrada)) < 0) {
            mi_signalSem();
            return FALLO;
        }
        //Escribimos la última entrada en la posición de la que queremos borrar
        if (mi_write_f(p_inodo_dir, &ultima_entrada, p_entrada * sizeof(struct entrada), sizeof(struct entrada)) < 0) {
            mi_signalSem();
            return FALLO;
        }
    }

    //Truncar el inodo del directorio padre (eliminamos el tamaño de 1 entrada)
    if (mi_truncar_f(p_inodo_dir, inodo_dir.tamEnBytesLog - sizeof(struct entrada)) == FALLO) {
        mi_signalSem();
        return FALLO;
    }

    //Decrementar nlinks y comprobar si hay que liberar
    inodo.nlinks--;
    
    if (inodo.nlinks == 0) {
        if (liberar_inodo(p_inodo) == FALLO) {
            mi_signalSem();
            return FALLO;
        }
    } else {
        inodo.ctime = time(NULL);
        if (escribir_inodo(p_inodo, &inodo) == FALLO) {
            mi_signalSem();
            return FALLO;
        }
    }

    mi_signalSem();
    return EXITO;
}