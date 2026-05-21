# RFC-UNA-2026-G

Protocolo CREEPER para Comunicación en Anillo de Luz
Comunicación en Red Encriptada para Enlaces de Pulsos En Redes-ópticas
Grupo [Grupo] — Universidad Nacional, Sede Brunca — I Ciclo 2026
Autores: Emily Castillo Monge, Gabriel Bermúdez Miranda, Óscar Barboza Cedeño

1. Capa Física
   Codificación
   Se utiliza Modulación por Duración de Pulso (PDM). El nodo emisor transmite señales digitales
   mediante el pin 9, conectado directamente al pin 2 del nodo receptor mediante cable, con GND
   compartido entre ambos Arduinos. No se emplea componente óptico. La duración del pulso en estado
   HIGH determina el valor del símbolo, tomando como unidad base cero = 100 ms:

   | Símbolo | HIGH   | LOW    | Total  | Significado                |
   | ------- | ------ | ------ | ------ | -------------------------- |
   | BIT 0   | 100 ms | 100 ms | 200 ms | Cero lógico (1 × cero)     |
   | BIT 1   | 300 ms | 100 ms | 400 ms | Uno lógico (3 × cero)      |
   | INICIO  | 500 ms | 100 ms | 600 ms | Inicio de trama (5 × cero) |
   | FIN     | 700 ms | 100 ms | 800 ms | Fin de trama (7 × cero)    |

   Duración del bit
   200 ms por bit, lo que equivale a una velocidad efectiva de aproximadamente 5 bits por segundo.
   Sincronización
   El receptor ejecuta pulseIn(2, LOW) / 100000 para medir la duración de cada pulso HIGH en
   microsegundos y convertirla a su múltiplo de cero. Los valores resultantes se interpretan de la
   siguiente manera: 5 = INICIO de trama, 7 = FIN de trama, 1 = BIT 0, 3 = BIT 1, 0 = timeout (sin
   señal).

2. Capa de Enlace

   Formato de trama

   | Campo    | Tamaño     | Descripción                                           |
   | -------- | ---------- | ----------------------------------------------------- |
   | INICIO   | 1 pulso    | 500 ms HIGH : marca el inicio de la trama             |
   | DESTINO  | 4 bits     | ID del nodo destino (0001 a 1111)                     |
   | ORIGEN   | 4 bits     | ID del nodo emisor                                    |
   | LONGITUD | 5 bits     | Cantidad de bytes en DATOS (máximo 30)                |
   | DATOS    | ≤ 240 bits | Payload cifrado con XOR (Capa 3)                      |
   | CHECKSUM | 8 bits     | XOR acumulado byte a byte de todos los DATOS cifrados |
   | FIN      | 1 pulso    | 700 ms HIGH : marca el fin de la trama                |

   Direccionamiento
   Universidad Nacional — Comunicación y Redes — I Ciclo 2026
   RFC-UNA-2026-G
   Protocolo CREEPER para Comunicación en Anillo de Luz
   Comunicación en Red Encriptada para Enlaces de Pulsos En Redes-ópticas
   Cada nodo tiene un identificador fijo de 4 bits: Grupo 1 = 0001, Grupo 2 = 0010, Grupo 3 = 0011,
   Broadcast = 1111 (decimal 15). El receptor compara el campo DESTINO con su ID_PROPIO para
   decidir si descifra y muestra el mensaje, o lo reenvía al siguiente nodo.
   Detección de errores
   Se utiliza checksum XOR. El emisor calcula el XOR acumulado byte a byte de todos los bytes de
   DATOS cifrados y lo transmite antes del FIN. El receptor recalcula y compara, si no coincide, la
   trama se descarta.
   Ej: DATOS = [0x7C, 0x21, 0x5D] -> Checksum = 0x7C XOR 0x21 XOR 0x5D = 0x60

3. Capa de Presentación / Seguridad
   Algoritmo de encriptación: Se aplica XOR bit a bit (bitwise XOR) con clave fija en la Capa 3, antes
   de pasar los datos a la Capa 2.
   Clave utilizada: 0x3C (binario: 00111100, decimal: 60). La clave es fija y está definida en este RFC,
   todos los grupos participantes la conocen.
   Proceso de cifrado
   Paso 1. Tomar el byte del mensaje en código ASCII.
   Paso 2. Aplicar: byte_cifrado = byte_original XOR 0x3C.
   Paso 3. Transmitir el byte cifrado. Sin la clave, el valor es ilegible.
   Proceso de descifrado
   Paso 1. Recibir el byte cifrado.
   Paso 2. Aplicar la misma operación: byte_original = byte_cifrado XOR 0x3C.
   Paso 3. El XOR es su propia inversa: aplicarlo dos veces con la misma clave devuelve el valor
   original.

4. Ejemplo de Transmisión
   Escenario: Nodo 1 envía el carácter 'A' al Nodo 2. Comando ingresado por el emisor: 2,A
   Paso 1 — Mensaje original: 'A' = 0x41 = 01000001
   Paso 2 — Cifrado XOR con clave 0x3C: 0x41 XOR 0x3C = 0x7D (ilegible sin la clave)
   Paso 3 — Checksum: 0x7D (único byte de DATOS)
   Paso 4 — Trama: [INICIO
   500ms][DEST=0010][ORIG=0001][LONG=00001][DATOS=01111101][CHK=01111101][FI
   N 700ms]
   Paso 5 — Codificación física de 0x7D (01111101):
   0 -> HIGH 100ms / LOW 100ms 1 -> HIGH 300ms / LOW 100ms
   1 -> HIGH 300ms / LOW 100ms 1 -> HIGH 300ms / LOW 100ms
   1 -> HIGH 300ms / LOW 100ms 1 -> HIGH 300ms / LOW 100ms
   0 -> HIGH 100ms / LOW 100ms 1 -> HIGH 300ms / LOW 100ms
   Paso 6 — Monitor serial receptor (antes de descifrar):
   Datos cifrados (HEX): 0x7D <- ilegible sin la clave
   Paso 7 — Monitor serial receptor (después de descifrar):
   Mensaje descifrado: A <- legible
