# RFC-UNA-2026-GRUPOX

Protocolo LUX-RING para Comunicación en Anillo de Luz
Grupo: [nombre del grupo]
Autores: [nombres]

## 1. Capa Física

- **Codificación:** OOK (On-Off Keying). Un bit '1' se transmite como luz encendida (LED ON), un bit '0' como luz apagada (LED OFF). La codificación de línea es idéntica a la modulación.
- **Duración de bit:** 20 ms por bit (frecuencia de modulación: 50 Hz).
- **Medio:** Aire, usando luz visible.
- **Emisor:** LASER
- **Receptor:** LDR (sensor de luz)
- **Sincronización:** El inicio de trama se detecta por la secuencia especial de inicio (11111100) en el campo de cabecera.
- **PDU:** Bits

## 2. Capa de Enlace de Datos

- **Formato de trama:**

| Campo       | Tamaño | Descripción                               |
| ----------- | ------ | ----------------------------------------- |
| Inicio      | 1 B    | Secuencia fija: 11111100                  |
| MAC Origen  | 6 B    | Dirección MAC del emisor                  |
| MAC Destino | 6 B    | Dirección MAC del receptor                |
| Longitud    | 1 B    | Tamaño de los datos/payload (en bytes)    |
| Control     | 1 B    | 4 bits actual (1-15), 4 bits total (1-15) |
| Datos       | 1-32 B | Payload cifrado                           |
| CheckSum    | 1 B    | Suma de los bits de datos, mod 256        |
| Fin         | 1 B    | Secuencia fija: 00000000                  |

- **Dirección:** MAC de origen y destino (6 bytes cada una).
- **Detección de errores:** CheckSum de 1 byte (suma de bits de datos, si excede 255 se descarta el acarreo y se usan los 8 bits inferiores).
- **Control:** Permite dividir mensajes largos en varios paquetes (paquete actual / total de paquetes), con numeración explícita en rango 1..15.
- **Tipo de enlace:** Simplex, sin confirmación ni retransmisión.
- **Notas:** El payload máximo es 32 bytes para evitar desincronización.

## 2.1 Formato de salida por consola

- **Prefijo de envío:** `>>:`
- **Prefijo de recepción:** `<<:`
- **Errores de recepción:** `<<:Fallos recepcion de mensaje: [tipo de fallo]`
- **Límite de impresión por línea:** 4 paquetes por línea (128 bytes = 32\*4). Si el mensaje supera ese tamaño, se imprime en líneas consecutivas.
- **Numeración mostrada:** cada línea puede incluir el rango de paquetes mostrado, por ejemplo: `>>:[paq 1-4/10] ...` y `<<:[paq 5-8/10] ...`

## 3. Capa de Presentación / Seguridad

- **Formato de texto:** UTF-8
- **Algoritmo de encriptación:** AES (simétrico, modo simple)
- **Clave utilizada:** Clave precompartida entre emisor y receptor (debe ser acordada previamente y tener la longitud requerida por AES, por ejemplo, 16 bytes para AES-128).
- **Proceso de cifrado:**
  1. El mensaje en texto plano se codifica en UTF-8.
  2. Se cifra usando AES y la clave precompartida.
  3. El resultado cifrado se coloca en el campo Datos/Payload de la trama.
- **Proceso de descifrado:**
  1. El receptor extrae el campo Datos/Payload.
  2. Descifra usando AES y la clave precompartida.
  3. Decodifica el resultado a texto UTF-8.

## 4. Ejemplo de transmisión

Supongamos el mensaje "A" (ASCII 65):

1. Codificación UTF-8: 0x41
2. Cifrado AES (clave: "1234567890abcdef") → Supongamos resultado: 0xB1 0x22 0xC3 0xD4 ...
3. Trama:
   - Inicio: 0xFC
   - MAC Origen: 0x01 0x02 0x03 0x04 0x05 0x06
   - MAC Destino: 0x0A 0x0B 0x0C 0x0D 0x0E 0x0F
   - Longitud: 0x10 (16 bytes de datos cifrados)
   - Control: 0x01 (paquete 1 de 1)
   - Datos: 0xB1 0x22 0xC3 0xD4 ...
   - CheckSum: suma de bytes de datos mod 256
   - Fin: 0x00

El receptor, al recibir la trama, verifica el checksum, extrae los datos, descifra con la clave y obtiene el mensaje original.

---

Este RFC describe completamente el protocolo LUX-RING para comunicación en anillo de luz, permitiendo a otros grupos implementar la recepción y transmisión de mensajes cifrados de forma compatible.
