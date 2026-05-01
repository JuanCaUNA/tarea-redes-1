/*
 * EMISOR - RFC-UNA-2026 / Protocolo LUX-RING
 */

#include "aes_minimal.h"

// * Emisor para LUX-RING
const int pinLaser = 13;

// * Tiempo por cada bit 50 ms por bit
const unsigned long BIT_TIME_US = 50 * 1000UL;

// * silencio previo para sincronizar
const unsigned long GUARD_TIME_US = BIT_TIME_US * 2;

// * Reglas definidas en el RFC
const uint8_t MAX_PAYLOAD = 32;
const uint8_t MAX_PAQUETES = 15;
const uint8_t BYTE_INICIO = 0xFC;
const uint8_t BYTE_FIN = 0x00;
const uint8_t MAC_LEN = 6;
const uint8_t PAQUETES_POR_LINEA = 4;
const uint8_t MAX_DATOS_PRIMER_PAQUETE = 30;
const uint16_t MAX_MENSAJE_TOTAL = MAX_DATOS_PRIMER_PAQUETE + ((MAX_PAQUETES - 1) * MAX_PAYLOAD);

// * Direcciones MAC (6 bytes)
const uint8_t MAC_ORIGEN[6] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
const uint8_t MAC_DESTINO[6] = { 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };

// * Clave AES-128 compartida
const uint8_t AES_KEY[AES_KEY_SIZE] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66 };

static aes128_ctx_t aesCtx;
static uint8_t aesInitFlag = 0;

uint8_t calcularNumeroPaquetes(uint16_t msgLen) {
  if (msgLen == 0) return 0;
  if (msgLen <= MAX_DATOS_PRIMER_PAQUETE) return 1;
  uint16_t restante = msgLen - MAX_DATOS_PRIMER_PAQUETE;
  uint8_t numPaquetes = (uint8_t)(1 + ((restante + MAX_PAYLOAD - 1) / MAX_PAYLOAD));
  if (numPaquetes > MAX_PAQUETES) {
    Serial.println(F(">>:Aviso mensaje truncado al maximo de 15 paquetes (478 bytes)"));
    return MAX_PAQUETES;
  }
  return numPaquetes;
}

uint16_t obtenerLongitudDatosPaquete(uint16_t msgLen, uint8_t paqNum) {
  if (paqNum == 0 || msgLen == 0) return 0;

  if (paqNum == 1) {
    return (msgLen < MAX_DATOS_PRIMER_PAQUETE) ? msgLen : MAX_DATOS_PRIMER_PAQUETE;
  }

  uint16_t offset = MAX_DATOS_PRIMER_PAQUETE + (uint16_t)(paqNum - 2) * MAX_PAYLOAD;
  if (offset >= msgLen) return 0;

  uint16_t restante = msgLen - offset;
  return (restante > MAX_PAYLOAD) ? MAX_PAYLOAD : restante;
}

uint16_t obtenerLongitudCifradaPaquete(uint16_t datosPlainLen) {
  return (uint16_t)(((datosPlainLen + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE);
}

uint8_t obtenerBytePlanoPaquete(const String &msg, uint16_t msgLen, uint8_t paqNum, uint16_t indiceEnPayload, uint16_t datosPlainLen) {
  if (paqNum == 1) {
    if (indiceEnPayload == 0) return (uint8_t)((msgLen >> 8) & 0xFF);
    if (indiceEnPayload == 1) return (uint8_t)(msgLen & 0xFF);

    uint16_t indiceTexto = (uint16_t)(indiceEnPayload - 2);
    if (indiceTexto < datosPlainLen) {
      return (uint8_t)msg[indiceTexto];
    }
    return 0;
  }

  uint16_t offsetTexto = MAX_DATOS_PRIMER_PAQUETE + (uint16_t)(paqNum - 2) * MAX_PAYLOAD;
  if (indiceEnPayload >= datosPlainLen) return 0;

  uint16_t indiceTexto = offsetTexto + indiceEnPayload;
  if (indiceTexto >= msgLen) return 0;
  return (uint8_t)msg[indiceTexto];
}

void transmitirBloqueCifrado(uint8_t bloque[AES_BLOCK_SIZE], uint8_t &checksum) {
  aes128_encrypt_block(&aesCtx, bloque);
  for (uint8_t i = 0; i < AES_BLOCK_SIZE; i++) {
    transmitByteMSB(bloque[i]);
    checksum += bloque[i];
  }
}

void imprimirMensajePorLineas(const String &msg, uint16_t msgLen, uint8_t totalPaq) {
  uint16_t inicio = 0;
  while (inicio < msgLen) {
    uint8_t paqInicio = (uint8_t)(inicio == 0 ? 1 : ((inicio <= MAX_DATOS_PRIMER_PAQUETE) ? 1 : 2 + ((inicio - MAX_DATOS_PRIMER_PAQUETE) / MAX_PAYLOAD)));
    uint8_t paqFin = paqInicio;
    uint16_t bytesLinea = 0;
    for (uint8_t paq = paqInicio; paq < paqInicio + PAQUETES_POR_LINEA && paq <= totalPaq; paq++) {
      bytesLinea += obtenerLongitudDatosPaquete(msgLen, paq);
      paqFin = paq;
    }
    uint16_t fin = inicio + bytesLinea;
    if (fin > msgLen) fin = msgLen;
    Serial.print(F(">>:[paq "));
    Serial.print(paqInicio);
    Serial.print(F("-"));
    Serial.print(paqFin);
    Serial.print(F("/"));
    Serial.print(totalPaq);
    Serial.print(F("] "));
    Serial.println(msg.substring(inicio, fin));
    inicio = fin;
  }
}

void inicializarAES() {
  if (!aesInitFlag) {
    aes128_key_expansion(&aesCtx, AES_KEY);
    aesInitFlag = 1;
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(pinLaser, OUTPUT);
  digitalWrite(pinLaser, LOW);
  inicializarAES();
  Serial.println(F(">>:EMISOR LUX-RING listo"));
  Serial.println(F(">>:Escribe mensaje y presiona Enter"));
}

void loop() {
  if (Serial.available() > 0) {
    String mensaje = Serial.readStringUntil('\n');
    mensaje.trim();
    enviarMensajeConFragmentacion(mensaje);
  }
}

void enviarMensajeConFragmentacion(const String &msg) {
  uint16_t msgLen = (uint16_t)msg.length();
  if (msgLen == 0) return;
  if (msgLen > MAX_MENSAJE_TOTAL) {
    Serial.println(F(">>:Aviso mensaje truncado al maximo permitido por el protocolo"));
    msgLen = MAX_MENSAJE_TOTAL;
  }
  uint8_t numPaquetes = calcularNumeroPaquetes(msgLen);
  for (uint8_t paqNum = 1; paqNum <= numPaquetes; paqNum++) {
    uint16_t datosPlainLen = obtenerLongitudDatosPaquete(msgLen, paqNum);
    uint16_t payloadPlainLen = (paqNum == 1) ? (uint16_t)(2 + datosPlainLen) : datosPlainLen;
    uint16_t payloadCifradoLen = obtenerLongitudCifradaPaquete(payloadPlainLen);
    enviarPaquete(msg, msgLen, paqNum, numPaquetes, datosPlainLen, payloadCifradoLen);
    Serial.print(F(">>:[paq "));
    Serial.print(paqNum);
    Serial.print(F("/"));
    Serial.print(numPaquetes);
    Serial.println(F("] enviado"));
    delay(100);
  }
  imprimirMensajePorLineas(msg, msgLen, numPaquetes);
}

void enviarPaquete(const String &msg, uint16_t msgLen, uint8_t paqNum, uint8_t totalPaq, uint16_t datosPlainLen, uint16_t payloadCifradoLen) {
  if (datosPlainLen == 0) return;

  digitalWrite(pinLaser, LOW);
  unsigned long t0 = micros();
  while (micros() - t0 < GUARD_TIME_US) {
  }

  transmitByteMSB(BYTE_INICIO);
  for (uint8_t i = 0; i < MAC_LEN; i++) transmitByteMSB(MAC_ORIGEN[i]);
  for (uint8_t i = 0; i < MAC_LEN; i++) transmitByteMSB(MAC_DESTINO[i]);

  transmitByteMSB((uint8_t)payloadCifradoLen);
  transmitByteMSB(((paqNum & 0x0F) << 4) | (totalPaq & 0x0F));

  uint8_t checksum = 0;
  uint16_t payloadPlainLen = (paqNum == 1) ? (uint16_t)(2 + datosPlainLen) : datosPlainLen;
  uint8_t bloque[AES_BLOCK_SIZE];

  for (uint16_t offset = 0; offset < payloadPlainLen; offset += AES_BLOCK_SIZE) {
    for (uint8_t i = 0; i < AES_BLOCK_SIZE; i++) {
      bloque[i] = 0;
    }

    uint8_t bytesBloque = (uint8_t)((payloadPlainLen - offset > AES_BLOCK_SIZE) ? AES_BLOCK_SIZE : (payloadPlainLen - offset));
    for (uint8_t i = 0; i < bytesBloque; i++) {
      bloque[i] = obtenerBytePlanoPaquete(msg, msgLen, paqNum, (uint16_t)(offset + i), datosPlainLen);
    }

    transmitirBloqueCifrado(bloque, checksum);
  }

  transmitByteMSB(checksum);
  transmitByteMSB(BYTE_FIN);
  digitalWrite(pinLaser, LOW);
}

void transmitByteMSB(uint8_t b) {
  for (int bit = 7; bit >= 0; bit--) {
    bool estado = (b >> bit) & 0x01;
    unsigned long startBit = micros();
    digitalWrite(pinLaser, estado ? HIGH : LOW);

    while (micros() - startBit < BIT_TIME_US) {
    }
  }
}
