# Ejemplos y Opciones para Desarrollar un Anillo de Comunicación de Luz

A continuación se presentan varias opciones y enfoques para implementar un anillo de comunicación óptica entre nodos usando Arduino o Ideaboard. Puedes elegir la que mejor se adapte a tus recursos y conocimientos.

---

## Opción 1: Codificación Binaria Simple (Presencia/Ausencia de Luz)

- **Transmisión:**
  - Un LED o láser representa un bit "1" encendiéndose, y un bit "0" apagándose durante un intervalo fijo (ej. 10 ms por bit).
- **Recepción:**
  - Un fotoresistor detecta la luz. Si hay luz, es "1"; si no, es "0".
- **Sincronización:**
  - Se envía una secuencia especial de bits (por ejemplo, "1010") al inicio de cada mensaje.
- **Ventajas:**
  - Fácil de implementar y entender.
- **Desventajas:**
  - Sensible a interferencias de luz ambiente.

---

## Opción 2: Codificación Manchester

- **Transmisión:**
  - Cada bit se representa por dos intervalos: "1" es luz-apagada, "0" es apagada-luz.
- **Recepción:**
  - El receptor mide los cambios de luz en cada intervalo.
- **Sincronización:**
  - Secuencia de bits única al inicio.
- **Ventajas:**
  - Mejor tolerancia a errores y sincronización.
- **Desventajas:**
  - Requiere mayor precisión en el tiempo.

---

## Opción 3: Comunicación con ACK y Retransmisión

- **Transmisión:**
  - Igual que las opciones anteriores, pero después de cada mensaje, el receptor responde con un "ACK" (mensaje corto de confirmación).
- **Recepción:**
  - Si el emisor no recibe el ACK, retransmite el mensaje.
- **Ventajas:**
  - Mayor confiabilidad.
- **Desventajas:**
  - Más complejidad en el código.

---

## Opción 4: Anillo con Reenvío Inteligente

- **Funcionamiento:**
  - Cada nodo recibe el mensaje, verifica si es el destinatario. Si no lo es, lo reenvía al siguiente nodo.
- **Dirección:**
  - Cada mensaje incluye un campo de dirección (ID de nodo destino).
- **Ventajas:**
  - Permite comunicación entre cualquier par de nodos.
- **Desventajas:**
  - Requiere lógica de direccionamiento y reenvío.

---

## Opción 5: Encriptación en la Capa de Presentación

- **Cifrado César:**
  - Antes de enviar el mensaje, se cifra con un desplazamiento fijo (ej. +3).
- **XOR con clave fija:**
  - Cada byte del mensaje se combina con una clave usando XOR.
- **Ventajas:**
  - Cumple el requisito de encriptación.
- **Desventajas:**
  - Se debe compartir la clave o el método de cifrado entre nodos.

---

## Ejemplo de Flujo Básico (Pseudo-código)

```c
// Emisor
inicializar_hardware();
while (hay_mensaje) {
    mensaje_cifrado = cifrar(mensaje);
    enviar_secuencia_inicio();
    for (bit in mensaje_cifrado) {
        enviar_bit(bit);
        esperar_intervalo();
    }
}

// Receptor
inicializar_hardware();
while (true) {
    if (detectar_secuencia_inicio()) {
        mensaje = recibir_bits();
        mensaje_descifrado = descifrar(mensaje);
        mostrar(mensaje_descifrado);
    }
}
```

---

## Consejos Generales

- Usa delays precisos para los intervalos de bits.
- Prueba primero la transmisión entre dos nodos antes de armar el anillo completo.
- Usa el monitor serial para depurar y mostrar mensajes cifrados/descifrados.
- Documenta bien tu protocolo y elige la opción que mejor se adapte a tu grupo.

---

> Puedes combinar varias de estas opciones según tus necesidades y recursos.
