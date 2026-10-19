//Hecho por: Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame
#include "directorios.h"

void mostrar_buscar_entrada(char *camino, char reservar){
  unsigned int p_inodo_dir = 0;
  unsigned int p_inodo = 0;
  unsigned int p_entrada = 0;
  int error;
  printf("\ncamino: %s, reservar: %d\n", camino, reservar);
  if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, reservar, 6)) < 0) {
    mostrar_error_buscar_entrada(error);
  }
  printf("**********************************************************************\n");
  return;
}


//Función para leer el superbloque del dispositivo virtual y mostrar su información. La función monta el dispositivo utilizando la función bmount(), lee el superbloque utilizando la función bread() y luego muestra la información del superbloque en la consola. Finalmente, desmonta el dispositivo utilizando la función bumount() y devuelve EXITO si la operación se realizó correctamente o FALLO si ocurre un error.
int main(int argc, char **argv){

    if (argc != 2){
        fprintf(stderr, RED "Sintaxis: ./leer_sf <disco>\n" RESET);
        return FALLO;
    }

    if (bmount(argv[1]) == FALLO){
        fprintf(stderr, RED "Error al montar el dispositivo\n" RESET);
        return FALLO;
    }

    struct superbloque SB;

    if (bread(posSB, &SB) == FALLO){
        fprintf(stderr, RED "Error al leer el superbloque\n" RESET);
        return FALLO;
    }

    printf("\nDATOS DEL SUPERBLOQUE\n");

    printf("posPrimerBloqueMB = %u\n", SB.posPrimerBloqueMB);
    printf("posUltimoBloqueMB = %u\n", SB.posUltimoBloqueMB);
    printf("posPrimerBloqueAI = %u\n", SB.posPrimerBloqueAI);
    printf("posUltimoBloqueAI = %u\n", SB.posUltimoBloqueAI);
    printf("posPrimerBloqueDatos = %u\n", SB.posPrimerBloqueDatos);
    printf("posUltimoBloqueDatos = %u\n", SB.posUltimoBloqueDatos);
    printf("posInodoRaiz = %u\n", SB.posInodoRaiz);
    printf("posPrimerInodoLibre = %u\n", SB.posPrimerInodoLibre);
    printf("cantBloquesLibres = %u\n", SB.cantBloquesLibres);
    printf("cantInodosLibres = %u\n", SB.cantInodosLibres);
    printf("totBloques = %u\n", SB.totBloques);
    printf("totInodos = %u\n", SB.totInodos);

    if (bumount() == FALLO){
        fprintf(stderr, RED "Error al desmontar el dispositivo\n" RESET);
        return FALLO;
    }

    return EXITO;
}