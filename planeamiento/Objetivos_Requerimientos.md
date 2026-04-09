
# Objetivos y Requerimientos — Tarea Anillo de Comunicación de Luz

## Fase 1: Diseño y Definición del Protocolo

- [ ] Definir stack de protocolos propio (mínimo 3 capas)
- [ ] Especificar codificación de bits, duración, sincronización
- [ ] Definir formato de trama, direccionamiento y detección de errores
- [ ] Seleccionar e integrar un método de encriptación en una capa superior

## Fase 2: Documentación (RFC)

- [ ] Redactar RFC (máx. 2 páginas) siguiendo la plantilla oficial
- [ ] RFC debe ser claro y suficiente para que otro grupo implemente la recepción
- [ ] Incluir ejemplo de transmisión cifrada

## Fase 3: Implementación y Pruebas

- [ ] Montar hardware: Arduino/Ideaboard, transmisor (láser/LED), receptor (fotoresistor/fotodiodo)
- [ ] Programar emisor y receptor según el protocolo definido
- [ ] Probar transmisión y recepción de mensajes cifrados

## Fase 4: Prueba Individual (Video)

- [ ] Grabar video mostrando:
  - [ ] Montaje físico
  - [ ] Mensaje original (emisor)
  - [ ] Mensaje cifrado (receptor, antes de descifrar)
  - [ ] Mensaje descifrado (receptor)
  - [ ] Explicación de cada capa y cifrado

## Fase 5: Integración del Anillo

- [ ] Implementar recepción del protocolo de otro grupo
- [ ] Integrar al menos 3 nodos en anillo
- [ ] Demostrar escenarios: A→B, A→B→C, B→C→A
- [ ] Mostrar en cada nodo: mensaje cifrado recibido, mensaje descifrado (si es para él), reenvío (si es para otro)

## Fase 6: Reporte Final y Evaluación

- [ ] Redactar reporte final (máx. 2 páginas) con:
  - [ ] Capas implementadas
  - [ ] Protocolo vecino implementado
  - [ ] Traducción entre protocolos
  - [ ] Implementación de encriptación
  - [ ] Problemas encontrados y soluciones (mín. 2)
  - [ ] Reflexión sobre interoperabilidad y seguridad
- [ ] Evaluar RFC de otro grupo (evaluación por pares)
- [ ] Cumplir cronograma de entregas (RFC, videos, reporte)
