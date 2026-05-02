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
const uint16_t MAX_MENSAJE_TOTAL = MAX_PAYLOAD * MAX_PAQUETES;  // * 480 bytes máximo

// * Direcciones MAC (6 bytes)
const uint8_t MAC_ORIGEN[6] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };

// * Clave AES-128 compartida
const uint8_t AES_KEY[AES_KEY_SIZE] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66 };

static aes128_ctx_t aesCtx;
static uint8_t aesInitFlag = 0;

struct ComandoEnvio {
  bool valido;
  String mensaje;
  uint8_t macDestino[MAC_LEN];
};

inline uint8_t truncarMacA4Bits(uint8_t valor) {
  // * Conserva solo los 4 bits menos significativos (rango 0-15).
  return (uint8_t)(valor & 0x0F);
}

void construirMacDestino(uint8_t macId4Bits, uint8_t macDestino[MAC_LEN]) {
  for (uint8_t i = 0; i < MAC_LEN; i++) {
    macDestino[i] = macId4Bits;
  }
}

ComandoEnvio parsearComandoEnvio(const String &entradaCruda) {
  ComandoEnvio cmd;
  cmd.valido = false;

  String entrada = entradaCruda;
  entrada.trim();

  int separador = entrada.indexOf(':');
  if (separador <= 0) return cmd;

  String macTexto = entrada.substring(0, separador);
  String mensaje = entrada.substring(separador + 1);
  macTexto.trim();
  mensaje.trim();
  if (macTexto.length() == 0 || mensaje.length() == 0) return cmd;

  int macValor = macTexto.toInt();
  if (String(macValor) != macTexto || macValor < 0 || macValor > 15) return cmd;

  cmd.mensaje = mensaje;
  construirMacDestino(truncarMacA4Bits((uint8_t)macValor), cmd.macDestino);
  cmd.valido = true;
  return cmd;
}

uint8_t calcularNumeroPaquetes(uint16_t msgLen) {
  if (msgLen == 0) return 0;
  if (msgLen <= MAX_PAYLOAD) return 1;
  // * Redondear hacia arriba: (msgLen + MAX_PAYLOAD - 1) / MAX_PAYLOAD
  uint8_t numPaquetes = (uint8_t)((msgLen + MAX_PAYLOAD - 1) / MAX_PAYLOAD);
  if (numPaquetes > MAX_PAQUETES) {
    Serial.println(F(">>:log: Aviso mensaje truncado al maximo de 15 paquetes (480 bytes)"));
    return MAX_PAQUETES;
  }
  return numPaquetes;
}

uint16_t obtenerLongitudDatosPaquete(uint16_t msgLen, uint8_t paqNum) {
  if (paqNum == 0 || msgLen == 0) return 0;

  // * Calcular offset de este paquete
  uint16_t offset = (uint16_t)(paqNum - 1) * MAX_PAYLOAD;
  if (offset >= msgLen) return 0;

  // * Bytes restantes desde este offset
  uint16_t restante = msgLen - offset;
  return (restante > MAX_PAYLOAD) ? MAX_PAYLOAD : restante;
}

uint16_t obtenerLongitudCifradaPaquete(uint16_t datosPlainLen) {
  return (uint16_t)(((datosPlainLen + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE);
}

uint8_t obtenerBytePlanoPaquete(const String &msg, uint16_t msgLen, uint8_t paqNum, uint16_t indiceEnPayload, uint16_t datosPlainLen) {
  // * Todos los paquetes usan la misma lógica: offset del paquete + índice dentro del payload
  uint16_t offsetPaquete = (uint16_t)(paqNum - 1) * MAX_PAYLOAD;
  uint16_t indiceTexto = offsetPaquete + indiceEnPayload;
  
  if (indiceTexto >= msgLen) return 0;
  if (indiceEnPayload >= datosPlainLen) return 0;
  
  return (uint8_t)msg[indiceTexto];
}

void aplicarPaddingPKCS7(uint8_t bloque[AES_BLOCK_SIZE], uint8_t datosLen) {
  // * datosLen = cantidad de bytes reales en bloque[0:datosLen-1]
  // * Rellenar resto con padding PKCS7
  uint8_t padLen = AES_BLOCK_SIZE - datosLen;
  for (uint8_t i = datosLen; i < AES_BLOCK_SIZE; i++) {
    bloque[i] = padLen;
  }
}

void transmitirBloqueCifrado(uint8_t bloque[AES_BLOCK_SIZE], uint8_t &checksum) {
  aes128_encrypt_block(&aesCtx, bloque);
  for (uint8_t i = 0; i < AES_BLOCK_SIZE; i++) {
    transmitByteMSB(bloque[i]);
    checksum += bloque[i];
  }
}

void imprimirMensajePorLineas(const String &msg, uint16_t msgLen, uint8_t totalPaq) {
  for (uint8_t paq = 1; paq <= totalPaq; paq++) {
    uint16_t inicio = (uint16_t)(paq - 1) * MAX_PAYLOAD;
    uint16_t bytesLinea = obtenerLongitudDatosPaquete(msgLen, paq);
    uint16_t fin = inicio + bytesLinea;
    if (fin > msgLen) fin = msgLen;

    Serial.print(F(">>:log: Eviado [paq "));
    Serial.print(paq);
    Serial.print(F("/"));
    Serial.print(totalPaq);
    Serial.print(F("] "));
    Serial.println(msg.substring(inicio, fin));
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
  Serial.println(F("EMISOR LUX-RING listo"));
  Serial.println(F("Usa formato N:mensaje (N entre 0 y 15)"));
}

void loop() {
  if (Serial.available() > 0) {
    ComandoEnvio cmd = parsearComandoEnvio(Serial.readStringUntil('\n'));
    if (!cmd.valido) {
      Serial.println(F(">>:log: Formato invalido. Usa N:mensaje con N entre 0 y 15"));
      return;
    }

    enviarMensajeConFragmentacion(cmd.mensaje, cmd.macDestino);
  }
}

void enviarMensajeConFragmentacion(const String &msg, const uint8_t macDestino[MAC_LEN]) {
  uint16_t msgLen = (uint16_t)msg.length();
  if (msgLen == 0) return;
  if (msgLen > MAX_MENSAJE_TOTAL) {
    Serial.println(F(">>:log: Aviso mensaje truncado al maximo permitido por el protocolo"));
    msgLen = MAX_MENSAJE_TOTAL;
  }
  uint8_t numPaquetes = calcularNumeroPaquetes(msgLen);
  for (uint8_t paqNum = 1; paqNum <= numPaquetes; paqNum++) {
    uint16_t datosPlainLen = obtenerLongitudDatosPaquete(msgLen, paqNum);
    uint16_t payloadCifradoLen = obtenerLongitudCifradaPaquete(datosPlainLen);
    enviarPaquete(msg, msgLen, paqNum, numPaquetes, datosPlainLen, payloadCifradoLen, macDestino);

    delay(100);
  }
  imprimirMensajePorLineas(msg, msgLen, numPaquetes);
}

void enviarPaquete(const String &msg, uint16_t msgLen, uint8_t paqNum, uint8_t totalPaq, uint16_t datosPlainLen, uint16_t payloadCifradoLen, const uint8_t macDestino[MAC_LEN]) {
  if (datosPlainLen == 0) return;

  digitalWrite(pinLaser, LOW);
  unsigned long t0 = micros();
  while (micros() - t0 < GUARD_TIME_US) {
  }

  transmitByteMSB(BYTE_INICIO);
  for (uint8_t i = 0; i < MAC_LEN; i++) transmitByteMSB(MAC_ORIGEN[i]);
  for (uint8_t i = 0; i < MAC_LEN; i++) transmitByteMSB(macDestino[i]);

  transmitByteMSB((uint8_t)payloadCifradoLen);
  transmitByteMSB(((paqNum & 0x0F) << 4) | (totalPaq & 0x0F));

  uint8_t checksum = 0;
  uint8_t bloque[AES_BLOCK_SIZE];

  // * Cifrar datos del paquete por bloques
  for (uint16_t offset = 0; offset < datosPlainLen; offset += AES_BLOCK_SIZE) {
    for (uint8_t i = 0; i < AES_BLOCK_SIZE; i++) {
      bloque[i] = 0;
    }

    uint8_t bytesBloque = (uint8_t)((datosPlainLen - offset > AES_BLOCK_SIZE) ? AES_BLOCK_SIZE : (datosPlainLen - offset));
    for (uint8_t i = 0; i < bytesBloque; i++) {
      bloque[i] = obtenerBytePlanoPaquete(msg, msgLen, paqNum, (uint16_t)(offset + i), datosPlainLen);
    }
    if (bytesBloque < AES_BLOCK_SIZE) {
      aplicarPaddingPKCS7(bloque, bytesBloque);
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
