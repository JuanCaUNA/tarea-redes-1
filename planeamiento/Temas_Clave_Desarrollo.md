# Temas y Conceptos Clave para el Desarrollo

Esta lista te ayudará a identificar los conocimientos necesarios para diseñar e implementar el protocolo y el sistema completo.

## Temas Técnicos

- **Comunicación óptica (transmisión y recepción de luz):**
 Uso de luz (láser, LED) para enviar información y sensores (fotoresistor, fotodiodo) para recibirla. Ejemplo: prender un LED para enviar un "1", apagarlo para un "0".

- **Codificación de bits (Manchester, pulso-presencia, etc.):**
 Forma de representar los bits usando luz. Ejemplo: en Manchester, un bit "1" es luz-apagada, un bit "0" es apagada-luz en el mismo intervalo.

- **Sincronización de señales:**
 Cómo el receptor sabe cuándo empieza un mensaje. Ejemplo: enviar una secuencia especial de bits (como "1010") al inicio.

- **Protocolos en capas (modelo OSI, TCP/IP, abstracción de capas):**
 Dividir la comunicación en partes independientes (física, enlace, aplicación). Ejemplo: la capa física transmite bits, la de enlace agrupa bits en mensajes, la de aplicación interpreta el mensaje.

- **Formato de tramas y direccionamiento:**
 Estructura de los mensajes y cómo se indica el destinatario. Ejemplo: una trama puede tener campos para origen, destino, datos y verificación.

- **Detección y corrección de errores (paridad, CRC, etc.):**
 Métodos para saber si un mensaje llegó bien. Ejemplo: sumar los bits y agregar un bit extra (paridad) para detectar errores simples.

- **Métodos de encriptación simples (César, XOR, ROT13, sustitución):**
 Técnicas para ocultar el mensaje. Ejemplo: en cifrado César, la letra "A" se convierte en "D" (desplazamiento de 3 posiciones).

- **Interoperabilidad de protocolos:**
 Capacidad de diferentes sistemas para comunicarse aunque usen protocolos distintos. Ejemplo: traducir un mensaje de un formato a otro al reenviarlo.

- **Programación de Arduino/Ideaboard:**
 Escribir código para controlar el hardware. Ejemplo: usar Arduino IDE para encender un LED cuando se recibe un "1".

- **Manejo de hardware: láser/LED, fotoresistor/fotodiodo:**
 Conectar y controlar los componentes físicos. Ejemplo: conectar un LED a un pin digital y un fotoresistor a una entrada analógica.

## Habilidades y Conceptos

- **Redacción técnica (RFC):**
 Escribir documentos claros y precisos para describir protocolos. Ejemplo: un RFC explica cómo funciona tu protocolo para que otros lo implementen.

- **Diagramas de protocolos y tramas:**
 Dibujar esquemas que muestran cómo se estructura y transmite la información. Ejemplo: un diagrama de una trama con campos de dirección, datos y verificación.

- **Pruebas y validación de sistemas:**
 Verificar que todo funcione correctamente. Ejemplo: enviar un mensaje y comprobar que llega igual al receptor.

- **Grabación y documentación en video:**
 Registrar en video el funcionamiento del sistema y explicar los resultados. Ejemplo: grabar el monitor serial mostrando el mensaje cifrado y descifrado.

- **Trabajo en equipo y colaboración:**
 Coordinarse con otros para repartir tareas y resolver problemas juntos. Ejemplo: un integrante diseña el protocolo, otro programa el Arduino, otro graba el video.

## Recursos Sugeridos

- Documentación oficial de Arduino: <https://www.arduino.cc/>
- Ejemplos de codificación óptica: busca "optical communication Arduino" en YouTube o Google.
- Tutoriales de encriptación básica: busca "cifrado César Arduino" o "XOR encryption Arduino".
- Ejemplos de RFC y documentación técnica: revisa RFCs en <https://datatracker.ietf.org/doc/rfc/>
- Actividades previas del curso: repasa tus apuntes y ejercicios anteriores.

> Marca los temas que ya dominas y señala los que necesitas investigar o reforzar.
