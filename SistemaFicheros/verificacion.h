//Hecho por: Andreu Morey Juan, Miquel Matas Soler y Francisco Hermosa Adame
#include "simulacion.h"

struct INFORMACION {
    int pid;
    unsigned int nEscrituras;//validadas
    struct REGISTRO PrimeraEscritura;
    struct REGISTRO UltimaEscritura;
    struct REGISTRO MenorPosicion;
    struct REGISTRO MayorPosicion;
};