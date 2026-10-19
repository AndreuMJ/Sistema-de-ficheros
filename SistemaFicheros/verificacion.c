//Hecho por: Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame
#include "verificacion.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, RED "Sintaxis: ./verificacion <disco> <directorio_simulacion>\n" RESET);
        return FALLO;
    }

    bmount(argv[1]);
    char *dir_simul = argv[2];

    //Verificar que el directorio existe
    struct STAT stat;
    int ninodo_dir = mi_stat(dir_simul, &stat); 
    if (ninodo_dir < 0) {
        fprintf(stderr, RED "Error: No se encuentra el directorio de simulación\n" RESET);
        bumount();
        return FALLO;
    }

    // Calcular el nº de entradas 
    int num_entradas = stat.tamEnBytesLog / sizeof(struct entrada);
    
    // Comprobar si están los 100 procesos
    if (num_entradas != NUMPROCESOS) {
        fprintf(stderr, RED "Error: El número de entradas (%d) no coincide con NUMPROCESOS (%d)\n" RESET, num_entradas, NUMPROCESOS);
        bumount();
        return FALLO;
    }

    // creamos el fichero informe.txt dentro del directorio de simulación
    char camino_informe[200];
    sprintf(camino_informe, "%sinforme.txt", dir_simul);
    if (mi_creat(camino_informe, 6) < 0) {
        fprintf(stderr, RED "Error al crear informe.txt\n" RESET);
        bumount();
        return FALLO;
    }

    int offset_informe = 0;
    int offset_dir = 0;
    struct entrada entrada_dir;

    printf("dir_sim: %s\n", dir_simul);
    printf("Nº de inodo: %d\n", ninodo_dir);
    printf("numentradas: %d NUMPROCESOS: %d\n", num_entradas, NUMPROCESOS);

    int num_proceso_impreso = 1; //Contador para el índice 

    // Recorrer todas las entradas del directorio de simulación
    for (int i = 0; i < num_entradas; i++) {
        // Leer la entrada actual del directorio
        mi_read(dir_simul, &entrada_dir, offset_dir, sizeof(struct entrada));
        offset_dir += sizeof(struct entrada);

        //Si la entrada no es un directorio de proceso, la saltamos 
        if (strchr(entrada_dir.nombre, '_') == NULL) continue;

        //Extraer el PID del nombre del directorio
        int pid = atoi(strchr(entrada_dir.nombre, '_') + 1);

        //construir la ruta al fichero prueba.dat
        char camino_fichero[250];
        sprintf(camino_fichero, "%s%s/prueba.dat", dir_simul, entrada_dir.nombre);

        //Inicializar la estructura de información
        struct INFORMACION info;
        info.pid = pid;
        info.nEscrituras = 0;

        //Búfer para leer los registros en bloques optimizados
        int cant_registros_buffer = 256;
        struct REGISTRO buffer_registros[cant_registros_buffer];
        int offset_fichero = 0;
        int leidos = 0;

        //Leer secuencialmente todo el archivo prueba.dat
        while ((leidos = mi_read(camino_fichero, buffer_registros, offset_fichero, sizeof(buffer_registros))) > 0) {
            int num_registros_leidos = leidos / sizeof(struct REGISTRO);

            for (int r = 0; r < num_registros_leidos; r++) {
                //Validar que la escritura no es basura y pertenece al proceso que estamos analizando
                if (buffer_registros[r].pid == info.pid) {
                    if (info.nEscrituras == 0) {
                        info.PrimeraEscritura = buffer_registros[r];
                        info.UltimaEscritura = buffer_registros[r];
                        info.MenorPosicion = buffer_registros[r];
                        info.MayorPosicion = buffer_registros[r];
                    } else {
                        if (buffer_registros[r].nEscritura < info.PrimeraEscritura.nEscritura) {
                            info.PrimeraEscritura = buffer_registros[r];
                        }
                        if (buffer_registros[r].nEscritura > info.UltimaEscritura.nEscritura) {
                            info.UltimaEscritura = buffer_registros[r];
                        }
                        if (buffer_registros[r].nRegistro < info.MenorPosicion.nRegistro) {
                            info.MenorPosicion = buffer_registros[r];
                        }
                        if (buffer_registros[r].nRegistro > info.MayorPosicion.nRegistro) {
                            info.MayorPosicion = buffer_registros[r];
                        }
                    }
                    info.nEscrituras++;
                }
            }
            offset_fichero += leidos;
        }

        //Si se han validado escrituras, preparamos el texto y lo escribimos en informe.txt
        if (info.nEscrituras > 0) {
            char buffer_string[2048];
            char t1[50], t2[50], t3[50], t4[50];
            struct tm *tm1 = localtime(&info.PrimeraEscritura.fecha);
            strftime(t1, sizeof(t1), "%Y-%m-%d %H:%M:%S", tm1);
            struct tm *tm2 = localtime(&info.UltimaEscritura.fecha);
            strftime(t2, sizeof(t2), "%Y-%m-%d %H:%M:%S", tm2);
            struct tm *tm3 = localtime(&info.MenorPosicion.fecha);
            strftime(t3, sizeof(t3), "%Y-%m-%d %H:%M:%S", tm3);
            struct tm *tm4 = localtime(&info.MayorPosicion.fecha);
            strftime(t4, sizeof(t4), "%Y-%m-%d %H:%M:%S", tm4);

            sprintf(buffer_string,
                    "PID: %d\n"
                    "Numero de escrituras: %d\n"
                    "Primera escritura\t%d\t%d\t%s\n"
                    "Ultima escritura\t%d\t%d\t%s\n"
                    "Menor Posicion\t\t%d\t%d\t%s\n"
                    "Mayor Posicion\t\t%d\t%d\t%s\n\n",
                    info.pid, info.nEscrituras,
                    info.PrimeraEscritura.nEscritura, info.PrimeraEscritura.nRegistro, t1,
                    info.UltimaEscritura.nEscritura, info.UltimaEscritura.nRegistro, t2,
                    info.MenorPosicion.nEscritura, info.MenorPosicion.nRegistro, t3,
                    info.MayorPosicion.nEscritura, info.MayorPosicion.nRegistro, t4);

            mi_write(camino_informe, buffer_string, offset_informe, strlen(buffer_string));
            offset_informe += strlen(buffer_string);
            
            printf("[%d) %d escrituras validadas en %s]\n", num_proceso_impreso++, info.nEscrituras, camino_fichero);
        }
    }

    bumount();
    return EXITO;
}