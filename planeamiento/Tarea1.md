#

Universidad Nacional
Sede Regional Brunca
Campus Perez Zeledon / Campus Coto
Comunicacion y redes de computadores
I Tarea - I ciclo 2026
Prof: M.C. Gabriel Nuñez M.
Valor: 5 %

## Objetivo

Implementar un anillo de comunicacion de luz con al menos 3 nodos (segun cantidad de grupos) donde
cada nodo (Arduino/IdeaBoard):
Define su propio stack de protocolos (mınimo 3 capas).
Documenta su protocolo en formato RFC.
Implementa el protocolo de otro grupo para poder recibir mensajes.
Reenvıa mensajes al siguiente nodo utilizando su propio protocolo.

## Cronograma

```tabla
Semana      Actividad
1-2         Diseño e implementacion del protocolo propio
3           Entrega del RFC. Otros grupos lo reciben
4           Implementacion del protocolo vecino. Grabacion de prueba individual
5           Integracion del anillo completo y demostracion final Especificaciones Tecnicas
```

## Hardware (por grupo)

1 Arduino UNO o Ideaboard
1 laser (o LED infrarrojo) como transmisor
1 fotoresistor (o fotodiodo IR) como receptor
Conexion: A criterio de cada grupo (Actividad 1 como referencia).
1

## Protocolo (diseño propio)

Cada grupo define su propio stack de al menos 3 capas (ej. Fısica + Enlace + Presentacion, o Fısica +
Transporte + Aplicacion, etc.).
El RFC debe incluir:

1. Capa Fısica:
   Codificacion de bits (ej. pulso corto/largo, Manchester, presencia/ausencia)
   Duracion del bit (ej. 10 ms)
   Sincronizacion (como detecta inicio)
2. Capas Superiores (al menos dos):
   Formato de trama (campos, tamaños)
   Direccionamiento (identificacion de nodos)
   Deteccion de errores (CRC, paridad)
   Confiabilidad (ACKs, timeouts) - opcional
   Cifrado/Encriptacion (obligatorio en al menos una capa)

> nota El RFC debe permitir que otro grupo implemente la recepcion sin ayuda adicional

## Requisito de Encriptacion

Obligatorio: El protocolo debe incluir un mecanismo de encriptacion en al menos una de las capas superiores. Puede ser:

- Cifrado por desplazamiento (Cesar)
- XOR con clave fija
- ROT13
- Cifrado por sustitucion simple
- Cualquier otro metodo de su invencion o investigacion

En el RFC debe especificar:

- En que capa se aplica el cifrado
- Algoritmo utilizado
- Como se realiza el descifrado en el receptor
- Si utiliza clave, como se comparte o conoce

En la demostracion: Debe mostrarse que sin la clave/descifrado, el mensaje no es legible (ej. mostrando el mensaje cifrado en el monitor serial antes de descifrar).

## Formato del RFC (Entregable Semana 3)

Estructura mınima (maximo 2 paginas):
RFC-UNA-2026-[GRUPO]
Protocolo [NOMBRE] para Comunicacion en Anillo de luz
Grupo: [nombre]
Autores: [nombres]

1. Capa Fısica
   - Codificacion: [descripcion]
   - Duracion bit: [ms]
   - Sincronizacion: [como detecta inicio]

2. Capa Superior 1 (ej. Enlace, Transporte)
   - Formato de trama:

   ```tabla
   | Campo | Tamano | Descripcion |
   |-------|--------|-------------|
   | ...   | ...    | ...         |
   ```

   - Direccionamiento: [como se identifican nodos]
   - Deteccion de errores: [metodo]

3. Capa Superior 2 (Presentacion / Seguridad)
   - Algoritmo de encriptacion: [descripcion]
   - Clave utilizada: [definicion]
   - Proceso de cifrado: [paso a paso]
   - Proceso de descifrado: [paso a paso]

4. Ejemplo de transmision
   [Mensaje "A" codificado paso a paso, mostrando el cifrado]

## Entregable Semana 4: Prueba Individual (Video)

Cada grupo graba un video de 3 minutos maximo demostrando:

1. Su protocolo funciona: Dos Arduinos/Ideaboards del mismo grupo se comunican (emisor → receptor).
2. Encriptacion: Demuestra que el mensaje transmitido no es legible sin descifrar (ej. mostrando el
   monitor serial antes y despues del descifrado).

El video debe mostrar:

- El montaje fısico.
- El monitor serial del emisor mostrando el mensaje original.
- El monitor serial del receptor mostrando el mensaje cifrado (antes de descifrar).
- El monitor serial del receptor mostrando el mensaje descifrado.
- Explicacion breve de cada capa, destacando donde se aplica el cifrado.

## Entregable Semana 5: Integracion Final (Video + Reporte)

Video (5 minutos maximo)
Demostracion del anillo completo con al menos 3 nodos:

```tabla
Escenario   Mostrar
A → B       Mensaje de A a B (vecino directo)
A → C       Mensaje viaja A→B→C
B → A       Mensaje viaja B→C→A
```

Cada nodo debe mostrar en su monitor serial:

- El mensaje cifrado recibido
- El mensaje descifrado (si es para el)
- Indicacion de reenvıo (si es para otro)

## Reporte (2 paginas maximo)

Cada grupo entrega un unico reporte consolidado que describe su experiencia en el anillo:

1. Resumen de implementacion:
   ¿Que capas implemento su grupo?
   ¿Que protocolo recibio e implemento del grupo vecino?
   ¿Como resolvieron la traduccion entre ambos protocolos?
   ¿Como implementaron la encriptacion?
2. Problemas encontrados: Al menos 2 problemas durante la integracion y como se resolvieron (ej.
   sincronizacion, tiempos, formato de trama, descifrado, etc.).
3. Reflexion: Responder:
   En su anillo, cada nodo usa un protocolo diferente, incluyendo diferentes metodos de encriptacion. ¿Como afecta esto a la interoperabilidad? ¿Que mecanismos permiten que un mensaje
   cifrado con un algoritmo viaje a traves de nodos que usan otros algoritmos? Relacione con
   como funciona la seguridad en Internet (HTTPS, TLS, etc.).

## Evaluacion por Pares

Cada grupo evaluara la claridad y completitud del RFC del grupo que diseño el protocolo que ellos
implementaron.
Instrumento de Evaluacion por Pares

```tabla
Criterio                        Puntaje     Descripcion
Claridad de la especificacion   5   El RFC fue facil de entender sin explicacion adicional
Completitud de la capa Fısica   5   Codificacion, duracion y sincronizacion claramente definidas
Completitud de la capa superior 5   Formato de trama y campos bien
especificados
Especificacion de encriptacion  8   Algoritmo, clave y proceso claramente definidos
Ejemplo util                    2   El ejemplo ayudo a entender el
protocolo
Facilidad de implementacion     5   1=muy difıcil, 5=muy facil de implementar la recepcion
Total                           30
```

Este puntaje (maximo 30 puntos) se suma a la calificacion del grupo evaluado.

### Mecanismo

1. En la Semana 5, cada grupo entrega una evaluacion por pares del RFC que implemento.
2. La evaluacion es confidencial (solo el profesor la recibe).
3. El puntaje promedio de las evaluaciones recibidas por un grupo se suma a su calificacion final.

### Criterios de Evaluacion

```
Criterio                                                    Puntos
RFC claro y completo (evaluado por pares)                   30
Video prueba individual (protocolo propio + encriptacion)   25
Video integracion final (anillo funcionando)                25
Reporte (problemas + reflexion)                             20
Total                                                       100
```

## Notas Importantes

- RFC como contrato: Sea preciso. Otros grupos dependeran de su documento y lo evaluaran.
- Encriptacion obligatoria: Debe ser funcional y demostrable en el video.
- Evaluacion por pares: La claridad de su RFC afecta directamente su calificacion.
- IA permitida: Pueden usar IA para generar codigo, pero deben entenderlo y poder explicarlo.
- Resiliencia: Si el anillo no funciona completamente, demuestren al menos comunicacion entre dos nodos con diferentes protocolos.
- Fechas: RFC en Semana 3. Videos en Semana 4 (individual) y Semana 5 (final).

## Recursos de Referencia

- Actividad 1: Codificacion y comunicacion con Arduino
- Actividad 2: Direccionamiento MAC y deteccion de errores
- Actividad 3: Enrutamiento y direccionamiento IP
- Actividad 4: Protocolos confiables (ACKs, timeouts)
- Actividad 5: DNS y comunicacion peer-to-peer
- Cifrado Cesar, XOR, ROT13 (investigacion propia)

1 2 4 8 = "15\*32=480"
