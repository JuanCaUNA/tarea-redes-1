
# Guía Paso a Paso — Tarea Anillo de Comunicación de Luz

## 1. Preparación y Definición

1. Lee cuidadosamente los requerimientos y objetivos del proyecto.
2. Investiga conceptos clave: codificación óptica, protocolos en capas, encriptación básica, direccionamiento y detección de errores.
3. Elige el hardware a utilizar (Arduino/Ideaboard, transmisor y receptor óptico).

## 2. Diseño del Protocolo Propio

1. Define el stack de 3 capas (ejemplo: Física, Enlace, Presentación).
2. Especifica para cada capa:
   - Codificación de bits (cómo se representa 0 y 1)
   - Duración de bit
   - Sincronización (inicio de mensaje)
   - Formato de trama (campos, tamaños, dirección)
   - Detección de errores
   - Algoritmo de encriptación y clave
3. Haz un diagrama o tabla de tu protocolo.

## 3. Redacción del RFC

1. Usa la plantilla oficial (ver archivo aparte).
2. Completa solo los apartados que ya tienes definidos; deja indicaciones en los que faltan por decidir.
3. Incluye un ejemplo de transmisión cifrada (puede ser simulado).

## 4. Implementación y Pruebas

1. Monta el circuito según tu diseño.
2. Programa el emisor y receptor.
3. Realiza pruebas de transmisión y recepción, mostrando el mensaje cifrado y descifrado.

## 5. Prueba Individual (Semana 4)

1. Graba un video mostrando:
   - Montaje físico
   - Mensaje original (emisor)
   - Mensaje cifrado (receptor, antes de descifrar)
   - Mensaje descifrado (receptor)
   - Explicación de cada capa y cifrado

## 6. Implementación del Protocolo Vecino

1. Recibe el RFC de otro grupo.
2. Implementa la recepción de mensajes usando ese protocolo.
3. Prueba la interoperabilidad.

## 7. Integración del Anillo (Semana 5)

1. Conecta al menos 3 nodos en anillo.
2. Prueba los escenarios: A→B, A→B→C, B→C→A.
3. Cada nodo debe mostrar: mensaje cifrado recibido, mensaje descifrado (si es para él), reenvío (si es para otro).

## 8. Reporte Final y Evaluación

1. Redacta el reporte final siguiendo la estructura solicitada.
2. Evalúa el RFC del grupo cuyo protocolo implementaste.
3. Entrega todos los productos en las fechas indicadas.
