# Implementacion AES-128 (sin librerias)

Este documento describe el archivo de codigo [codigo/aes_minimal.h](codigo/aes_minimal.h), que implementa AES-128 para bloques de 16 bytes sin depender de librerias externas. La idea es poder incluirlo directamente en un archivo .ino.

## Alcance

- Cifra y descifra un bloque de 16 bytes (AES-128, 10 rondas).
- Incluye el key schedule (expansion de llave) de 16 bytes a 176 bytes.
- No incluye modos de operacion (CBC, CTR, etc.) ni padding.

## API disponible

- `aes128_key_expansion(aes128_ctx_t *ctx, const uint8_t key[16])`
- `aes128_encrypt_block(const aes128_ctx_t *ctx, uint8_t block[16])`
- `aes128_decrypt_block(const aes128_ctx_t *ctx, uint8_t block[16])`

### Estructura

- `aes128_ctx_t` guarda `round_keys[176]`.
- El bloque de datos se modifica en el mismo arreglo de 16 bytes.

## Flujo de cifrado (AES-128)

Un bloque AES se representa como una matriz de $4 \times 4$ bytes en orden columna.

1. **AddRoundKey (ronda 0)**
   - Se hace XOR del bloque con la primera subllave (16 bytes).
2. **Rondas 1 a 9**
   - `SubBytes`: sustitucion byte a byte usando la S-Box.
   - `ShiftRows`: se rotan las filas 1, 2 y 3 en 1, 2 y 3 posiciones.
   - `MixColumns`: mezcla lineal por columna en $GF(2^8)$.
   - `AddRoundKey`: XOR con la subllave de la ronda.
3. **Ronda 10** (final)
   - `SubBytes` + `ShiftRows` + `AddRoundKey`.
   - No se aplica `MixColumns`.

## Flujo de descifrado

Es el inverso del cifrado:

1. `AddRoundKey` con la ultima subllave.
2. Para rondas 9 a 1:
   - `InvShiftRows` -> `InvSubBytes` -> `AddRoundKey` -> `InvMixColumns`.
3. Ronda final:
   - `InvShiftRows` -> `InvSubBytes` -> `AddRoundKey`.

## Expansión de llave (Key Schedule)

- AES-128 usa una llave de 16 bytes y genera 11 subllaves (16 bytes cada una).
- Cada 16 bytes se aplica:
  - `RotWord` (rota 4 bytes).
  - `SubWord` (S-Box por byte).
  - XOR con `Rcon` (constante de ronda).
- El resto se obtiene con XOR con el bloque anterior.

## Uso en .ino (ejemplo minimo)

```c
#include "aes_minimal.h"

void setup() {
    aes128_ctx_t ctx;
    uint8_t key[16] = { 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f };
    uint8_t block[16] = { 0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff };

    aes128_key_expansion(&ctx, key);
    aes128_encrypt_block(&ctx, block);

    // block ahora contiene el texto cifrado
    aes128_decrypt_block(&ctx, block);
    // block vuelve al texto plano original
}

void loop() {
}
```

## Notas y limites

- AES solo cifra bloques de 16 bytes. Para mensajes mas largos hay que dividir y/o usar un modo de operacion.
- No hay padding: debes asegurar que los datos esten alineados a 16 bytes.
- Para seguridad real en redes, se recomienda usar un modo como CTR o CBC.
