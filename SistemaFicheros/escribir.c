//Hecho por: Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame
#include "ficheros.h"
#include <string.h> 

int main(int argc, char **argv){

    if (argc != 4){
        fprintf(stderr,
        "Sintaxis: escribir <nombre_dispositivo> <\"$(cat fichero)\"> <diferentes_inodos>\n");
        return FALLO;
    }

    unsigned int offsets[5]={9000,209000,30725000,409605000,480000000};

    bmount(argv[1]);

    char *texto=argv[2];
    int diferentes=atoi(argv[3]);

    int longitud=strlen(texto);

    printf("longitud texto: %d\n\n",longitud);

    int ninodo;

    if (!diferentes){
        ninodo = reservar_inodo('f',6);
    }

    for(int i=0;i<5;i++){

        if(diferentes){
            ninodo=reservar_inodo('f',6);
        }

        printf("Nº inodo reservado: %d\n",ninodo);
        printf("offset: %d\n",offsets[i]);

        int escritos=mi_write_f(ninodo,texto,offsets[i],longitud);

        printf("Bytes escritos: %d\n",escritos);

        struct STAT stat;

        mi_stat_f(ninodo,&stat);

        printf("stat.tamEnBytesLog=%d\n",stat.tamEnBytesLog);
        printf("stat.numBloquesOcupados=%d\n\n",stat.numBloquesOcupados);
    }

    bumount();

    return EXITO;
}