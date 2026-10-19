# Sistema-de-ficheros

Observaciones sobre la implementación (de todas las entregas):

Durante el desarrollo hemos tenido problemas con la gestión de la persistencia del inodo. En concreto, en la función mi_write_f se sobrescribía el inodo con una copia desactualizada después de llamar a traducir_bloque_inodo, lo que provocaba que el campo numBloquesOcupados se quedase siempre a 0. Se ha corregido releyendo el inodo antes de actualizarlo.

También se ha mejorado la función liberar_bloques_inodo con los saltos.

Se ha mejorado el mi_dir y mi_ls para que funcione el comando mi_ls -l.

Se ha creado tanto el mi_touch como el mi_rmdir.

Se ha implementado el caché con LRU y FIFO.

El debuging se puede activar o desactivar por niveles en bloques.h.

Excepto en mi_read_f, se ha optado por poner un semaforo al inicio y en cada salida.
