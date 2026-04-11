# Implementación LUX-RING: Emisor y Receptor

## Resumen de Mejoras

Se implementaron `emisor.ino` y `receptor.ino` basados en el RFC-UNA-2026-LUX-RING, con soporte para fragmentación y reconstrucción de mensajes.

---

## 1. Capa Física (practica.ino como referencia)

- **Codificación:** OOK (On-Off Keying)
- **Tiempo por bit:** 10 ms (100 Hz)
- **Sincronización:** Detección de inicio de trama (0xFC)
- **GPIO:**
  - Pin 13 (LED/Láser) — EMISOR
  - A0 (Sensor) — RECEPTOR
  - Umbral: ~100 (analógico)

---

## 2. Capa de Enlace: Formato de Trama

```
[0xFC] [MAC_origen:6B] [MAC_dest:6B] [Longitud:1B] [Control:1B] [Payload:1-32B] [Checksum:1B] [0x00]
```

| Campo       | Bytes | Descripción                             |
| ----------- | ----- | --------------------------------------- |
| Inicio      | 1     | 0xFC                                    |
| MAC Origen  | 6     | Dirección emisor                        |
| MAC Destino | 6     | Dirección receptor/broadcast            |
| Longitud    | 1     | Bytes de payload (1-32)                 |
| Control     | 1     | [4 bits: paq actual][4 bits: total paq] |
| Payload     | 1-32  | Datos del mensaje                       |
| Checksum    | 1     | Suma de payload mod 256                 |
| Fin         | 1     | 0x00                                    |

---

## 3. Fragmentación y Reconstrucción

### Emisor: `enviarMensajeConFragmentacion()`

- **Algoritmo:**
  1. Calcula número de paquetes: `ceil(longitud / 32)`
  2. Para cada paquete:
     - Extrae 32 bytes máximo
     - Codifica Control: `(paqNum << 4) | totalPaq`
     - Calcula checksum del payload
     - Transmite bit a bit (LSB primero, OOK)
  3. Pausa de 100 ms entre paquetes

- **Ejemplo:** Mensaje de 80 bytes ➜ 3 paquetes
  ```
  Paquete 1: Control=0x13 (1 de 3), Payload=32B
  Paquete 2: Control=0x23 (2 de 3), Payload=32B
  Paquete 3: Control=0x33 (3 de 3), Payload=16B
  ```

### Receptor: Buffer de Reconstrucción

- **Estructura:** Array de 15 paquetes (límite de 4 bits)
- **Lógica:**
  1. Detecta primer paquete ➜ inicializa buffer
  2. Almacena cada paquete según Control byte
  3. Cuenta paquetes recibidos vs. esperados
  4. Cuando `paquetesRecibidos == totalEsperados` ➜ reconstruye mensaje
  5. Concatena payloads en orden y muestra completo

---

## 4. Instalación y Prueba

### Requisitos

- Arduino IDE 1.8+
- 2× Arduino UNO (o similares)
- LED de bajo consumo o láser de ~5 mW (pin 13)
- Fotodiodo o sensor de luz LDR (A0)
- Resistencias y conexión a GND/5V

### Carga

1. **Emisor:** Subir `emisor.ino`
2. **Receptor:** Subir `receptor.ino`
3. Monitor Serial (9600 baud) en ambos

### Ejemplo de Uso

**Emisor (enviar "Hola"):**

```
--- EMISOR LUX-RING listo ---
Escribe mensaje y presiona Enter:
Hola
>>> Enviado en 1 paquete(s)
```

**Receptor:**

```
--- RECEPTOR LUX-RING listo ---
<<< Paquete recibido: 1 de 1
<<< MENSAJE COMPLETO: Hola
```

**Emisor (enviar 80 caracteres):**

```
Mensaje largo aquí con muchos caracteres para probar fragmentación...x
>>> Enviado en 3 paquete(s)
```

**Receptor:**

```
<<< Paquete recibido: 1 de 3
<<< Paquete recibido: 2 de 3
<<< Paquete recibido: 3 de 3
<<< MENSAJE COMPLETO: Mensaje largo aquí con muchos caracteres para probar fragmentación...x
```

---

## 5. Validaciones Implementadas

✓ Longitud de payload (1-32 bytes)  
✓ Control byte válido (paquete actual y total ≤ 15)  
✓ Checksum de payload (suma mod 256)  
✓ Byte de inicio (0xFC) y fin (0x00)  
✓ Sincronización de bits (10 ms por bit)  
✓ Reconstrucción ordenada de fragmentos

---

## 6. Próximas Mejoras

- [ ] Encriptación AES del payload (según RFC capa 3)
- [ ] Ajuste automático de `umbral` según sensor
- [ ] Retransmisión en caso de checksum fallido
- [ ] EEPROM storage de MAC origen/destino personalizadas
- [ ] Interfaz web o app móvil para visualizar mensajes

---

## 7. Notas Técnicas

- **Frecuencia de modulación:** 100 Hz (Periodo = 10 ms/bit)
- **Velocidad:** 100 bps (bits por segundo)
- **Distancia efectiva:** 30-50 cm (según luz ambiente y potencia LED)
- **Ruido:** Sensible a luz ambiente → usar LED IR o láser para mayor confiabilidad
- **Sincronización:** LSB primero en cada byte, pausa entre bytes para sincronización
