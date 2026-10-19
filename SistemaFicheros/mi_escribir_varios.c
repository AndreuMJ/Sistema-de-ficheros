#include "directorios.h"
//Hecho por: Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame

int main(int argc, char **argv){


    //Comprobamos sintaxis
    if (argc!=5) {
       fprintf(stderr, "Sintaxis: mi_escribir_varios <nombre_dispositivo> </ruta_fichero> <texto> <offset>\n");
       exit(-1);
    }


    //montamos el dispositivo
    if(bmount(argv[1])<0) return -1;
    //obtenemos el texto y su longitud
    char *buffer_texto = argv[3];
    int longitud=strlen(buffer_texto);


    //obtenemos la ruta y comprobamos que no se refiera a un directorio
    if (argv[2][strlen(argv[2])-1]=='/') {
      fprintf(stderr, "Error: la ruta se corresponde a un directorio");
      exit(-1);
    }
    char *camino = argv[2];
    unsigned int offset=atoi(argv[4]); //obtenemos el offset
    int escritos=0;
    int varios = 10;
    fprintf(stderr, "longitud texto: %d\n", longitud);
    for (int i=0; i<varios; i++) {
      // escribimos varias veces el texto desplazado 1 bloque
      escritos += mi_write(camino,buffer_texto,offset+BLOCKSIZE*i,longitud);
    }
    fprintf(stderr, "Bytes escritos: %d\n", escritos);
    /* Visualización del stat
    struct STAT stat;
    mi_stat_f(ninodo, &stat);
    printf("stat.tamEnBytesLog=%d\n",stat.tamEnBytesLog);
    printf("stat.numBloquesOcupados=%d\n",stat.numBloquesOcupados);
    */


    bumount();
}
