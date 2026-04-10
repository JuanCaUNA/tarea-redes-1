#

1. Sin retrasmision
2. Capas
   - Fisica
   - Enlace
   - Presentacion
3. Codificacion: OOK
4. Envio con IP de destino, cabecera incluye el IP de destino
5. Encriptacion: AES, clave precompatida

## 1

Fisica:

- Medio: Aire
- Emisor: LED
- Receptor: Fotodiodo / sensor de luz
- Tipo de señal: Óptica (luz visible)
- Modulacion: (OOK - On-Off Keying)
  - sin luz = 0
  - Con luz = 1
- Codificacion de linea OOK
- Frecuencia de modulación es 100 Hz
- Tiempo de bit: 10ms por bit, velocidad de segura para minimizar errores

> Notas:  
> Li-Fi (Light Fidelity) o Comunicaciones por Luz Visible
> La **modulación OOK** actúa también como forma de **codificación** de línea en este sistema
> PDU: bits
> tiempo por bit: 10ms | velocidad 100bps/100 hz | estabilidad alta

## 2

Enlace de datos

- Trama: `[Header][Datos][Trailer]`
  - Header:
    - Inicio Trama (1B): 11111100
    - Direccionamiento:
      - MAC Origen (6B)
      - MAC Destino (6B)
    - Longitud (1B): indica cuanto mide los datos/playload
    - Control (1B): indica el paquete actual y la cantidad maxima (0000 | 0000)
      - primeros 4bits: paquete actual
      - siguientes 4bits: cantidad de paquetes que se enviaran (total)
  - Datos / Payload: envia de 1 a 32 Bytes
  - Trailer
    - Deteccione errores (1B): CheckSum
    - Fin de trama (1B): 00000000

    resumen `[Inicio: 1B][MACs: 12B][Longitud: 1B][Control: 1B][Datos: Var][CheckSum: 1B][Fin: 1B]`

    Header = 1B + 6B*2 + 1B + 1B = 15 Bytes
    Datos = max 32 B
    Datos = min 1-32 Bytes
    Trailer = 1B + 1B = 2 Bytes
    Total = Header + Datos + Trailer = 49 Bytes

1. Deteccion de errores: CheckSum, se suma los bits de los datos hasta 255 (si se excede de esa cantidad descarta el bit de acarreo y nos quedamos con los 8 bits inferiores)

2. Control: envio continuo
   - Confirmacion: sin confirmacion
3. Direccionamiento: partida y destino
4. Tipo de enlace: simplex

> notas
> El playload se define como maximo 32Bytes para evitar problemas de descincronisacion

## 3

Presentacion:

Formato de texto: UTF-8

Encriptacion para los datos: método simple AES (simétrico), usa una clave para encriptar y desencriptar. Usa una clave precompartida

## especificaciones

- Dispositivo arduino UNO
- lenguaje C++

### Archivos

1 archivo encargado del envio
1 archivo encargado de la recepcion

1 Control de salida/entrada

Envio de mensaje, Recepcion. Se muestra un panel de entrada y salida, por cada mensaje limpia y vuelve a imprirmir. Permite ver mensajes de error de recepcion y los mensajes enviados

```
Enviados        | Recibidos
mensaje enviado |
                | mensaje recibido
mensaje largo   |
Enviados        |
                | mensaje largo 
                | recibido
                | Fallos recepcion de mensaje: [tipo de fallo]
```
