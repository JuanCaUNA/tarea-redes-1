/* =============================================================
 * RECEPTOR - RFC-UNA-2026 / Protocolo LUX-RING
 * Autores RFC: Juan Carlos Camacho Solano, Noemi Murillo Godinez
 * =============================================================
 *
 * Capa Fisica:
 *   - OOK: luz ON = '1', luz OFF = '0'
 *   - Duracion de bit: 50 ms
 *   - Sincronizacion: deteccion del byte de inicio 0xFC
 *
 * Formato de trama:
 *   [Inicio 1B][MAC_Orig 6B][MAC_Dest 6B][Longitud 1B][Control 1B]
 *   [Datos 1-32B][CheckSum 1B][Fin 1B]
 *
 * Seguridad: AES-128 con clave precompartida de 16 bytes
 * ============================================================= */

#include "aes_minimal.h"

// * Pines y tiempos 
const int PIN_LDR = A0;
const int UMBRAL = 20;
const unsigned long DUR_BIT_US = 50 * 1000UL;
const unsigned long MUESTREO_US = DUR_BIT_US / 2;

// * Constantes del protocolo 
const byte INICIO_TRAMA = 0xFC;
const byte FIN_TRAMA = 0x00;
const byte MAX_PAYLOAD = 32;
const byte MAX_PAQUETES = 15;
const byte MAX_DATOS_PRIMER_PAQUETE = 30;
const uint16_t MAX_MENSAJE_TOTAL = MAX_DATOS_PRIMER_PAQUETE + ((uint16_t)(MAX_PAQUETES - 1) * MAX_PAYLOAD);

// * Identidad del nodo (MAC propia, 6 bytes) 
const byte MI_MAC[6] = { 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };

// * Clave AES-128 precompartida (16 bytes) 
const uint8_t AES_KEY[AES_KEY_SIZE] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66 };

static aes128_ctx_t aesCtx;
static uint8_t aesInitFlag = 0;

// * Buffer para reensamblado de paquetes multiples 
byte mensajeBuffer[MAX_MENSAJE_TOTAL];
uint16_t paquetesRecibidos = 0;  // Bitfield: bit i = 1 si paquete i recibido
uint8_t totalPaquetes = 0;
uint8_t contadorRecibidos = 0;
uint16_t mensajeLongitudEsperada = 0;
unsigned long nextBitStartUs = 0;
byte datosGlobal[MAX_PAYLOAD];   // Buffer reutilizable para datos
byte datosCifradosGlobal[MAX_PAYLOAD];  // Buffer reutilizable para datos cifrados


// Funciones de capa fisica

bool hayLuz() {
  return analogRead(PIN_LDR) < UMBRAL;
}

bool esperarFlancoSubida(unsigned long timeoutUs) {
  unsigned long t0 = micros();
  bool vioOscuro = false;

  while (micros() - t0 < timeoutUs) {
    if (!hayLuz()) {
      vioOscuro = true;
      continue;
    }

    if (vioOscuro) {
      nextBitStartUs = micros();
      return true;
    }
  }

  return false;
}

void esperarHasta(unsigned long tObjetivo) {
  while ((long)(micros() - tObjetivo) < 0) {
  }
}

bool recibirBitSync() {
  unsigned long sampleTime = nextBitStartUs + MUESTREO_US;
  esperarHasta(sampleTime);
  bool bit = hayLuz();
  nextBitStartUs += DUR_BIT_US;
  esperarHasta(nextBitStartUs);
  return bit;
}

byte recibirByteSync() {
  byte b = 0;
  for (int i = 7; i >= 0; i--) {
    if (recibirBitSync()) bitSet(b, i);
  }
  return b;
}

bool buscarInicioTrama() {
  byte shift = 0;
  for (unsigned int i = 0; i < 200; i++) {
    shift = (byte)((shift << 1) | (recibirBitSync() ? 1 : 0));
    if (shift == INICIO_TRAMA) return true;
  }
  return false;
}

void esperarCanalLibre() {
  while (hayLuz()) {
  }
}

//  Funciones auxiliares

inline bool macIgual(const byte* a, const byte* b) {
  for (int i = 0; i < 6; i++) if (a[i] != b[i]) return false;
  return true;
}

inline void imprimirMAC(const byte* mac) {
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 0x10) Serial.print(F("0"));
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(F(":"));
  }
}

uint8_t calcularNumeroPaquetes(uint16_t msgLen) {
  if (msgLen == 0) return 0;
  if (msgLen <= MAX_DATOS_PRIMER_PAQUETE) return 1;

  uint16_t restante = msgLen - MAX_DATOS_PRIMER_PAQUETE;
  uint8_t numPaquetes = (uint8_t)(1 + ((restante + MAX_PAYLOAD - 1) / MAX_PAYLOAD));
  return (numPaquetes > MAX_PAQUETES) ? MAX_PAQUETES : numPaquetes;
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

void descifrarAES(byte* datos, byte len) {
  for (uint8_t offset = 0; offset < len; offset += AES_BLOCK_SIZE) {
    aes128_decrypt_block(&aesCtx, datos + offset);
  }
}

void resetearBuffer() {
  memset(mensajeBuffer, 0, sizeof(mensajeBuffer));
  paquetesRecibidos = 0;
  totalPaquetes = 0;
  contadorRecibidos = 0;
  mensajeLongitudEsperada = 0;
}

inline void imprimirDatosHex(const uint8_t* datos, uint16_t len) {
  for (uint16_t i = 0; i < len; i++) {
    if (datos[i] < 0x10) Serial.print(F("0"));
    Serial.print(datos[i], HEX);
  }
}

void imprimirMensaje(const uint8_t* texto, uint16_t totalBytes, uint8_t totalPaq) {
  const uint8_t PAQ_POR_LINEA = 4;
  uint16_t byteOffset = 0;
  uint8_t paqImpreso = 0;

  while (paqImpreso < totalPaq && byteOffset < totalBytes) {
    uint8_t paqDesde = paqImpreso + 1;
    uint8_t paqHasta = paqImpreso + PAQ_POR_LINEA;
    if (paqHasta > totalPaq) paqHasta = totalPaq;

    uint16_t bytesLinea = 0;
    for (uint8_t paq = paqDesde; paq <= paqHasta; paq++) {
      bytesLinea += obtenerLongitudDatosPaquete(totalBytes, paq);
    }

    if (byteOffset + bytesLinea > totalBytes) {
      bytesLinea = totalBytes - byteOffset;
    }

    Serial.print("<<:[paq ");
    Serial.print(paqDesde);
    Serial.print("-");
    Serial.print(paqHasta);
    Serial.print("/");
    Serial.print(totalPaq);
    Serial.print("] ");
    Serial.write(texto + byteOffset, bytesLinea);
    Serial.println();

    byteOffset += bytesLinea;
    paqImpreso = paqHasta;
  }
}

void inicializarAES() {
  if (!aesInitFlag) {
    aes128_key_expansion(&aesCtx, AES_KEY);
    aesInitFlag = 1;
  }
}

//  setup / loop

void setup() {
  Serial.begin(9600);
  resetearBuffer();
  inicializarAES();
  Serial.println(F("Receptor LUX-RING listo. Esperando trama..."));
}

void loop() {
  if (!esperarFlancoSubida(500000UL)) return;

  if (!buscarInicioTrama()) {
    Serial.println(F("<<:Fallos recepcion de mensaje: [no se encontro byte de inicio]"));
    esperarCanalLibre();
    return;
  }

  byte macOrigen[6], macDestino[6];
  for (int i = 0; i < 6; i++) macOrigen[i] = recibirByteSync();
  for (int i = 0; i < 6; i++) macDestino[i] = recibirByteSync();

  byte longitud = recibirByteSync();
  byte control = recibirByteSync();
  byte paqActual = (control >> 4) & 0x0F;
  byte paqTotal = control & 0x0F;

  if (!macIgual(macDestino, MI_MAC)) {
    Serial.print(F("<<:Fallos recepcion de mensaje: [trama para otra MAC "));
    imprimirMAC(macDestino);
    Serial.println(F("]"));
    for (int i = 0; i < longitud; i++) recibirByteSync();
    recibirByteSync();
    recibirByteSync();
    esperarCanalLibre();
    return;
  }

  if (longitud == 0 || longitud > MAX_PAYLOAD || (longitud % AES_BLOCK_SIZE) != 0) {
    Serial.print(F("<<:Fallos recepcion de mensaje: [longitud invalida "));
    Serial.print(longitud);
    Serial.println(F("]"));
    esperarCanalLibre();
    return;
  }

  if (paqActual == 0 || paqTotal == 0 || paqActual > paqTotal || paqTotal > MAX_PAQUETES) {
    Serial.print(F("<<:Fallos recepcion de mensaje: [control invalido 0x"));
    Serial.print(control, HEX);
    Serial.println(F("]"));
    esperarCanalLibre();
    return;
  }

  // VERIFICACIÓN ANTICIPADA: Para paquete 2+, verificar tamaño ANTES de leer datos
  if (paqActual > 1 && totalPaquetes > 0 && mensajeLongitudEsperada > 0) {
    uint16_t textoLenEsperado = obtenerLongitudDatosPaquete(mensajeLongitudEsperada, paqActual);
    uint16_t cifradoEsperadoAntici = obtenerLongitudCifradaPaquete(textoLenEsperado);
    if (cifradoEsperadoAntici != longitud) {
      Serial.print(F("<<:Fallos recepcion de mensaje: [tamaño incorrecto paq "));
      Serial.print(paqActual);
      Serial.print(F(" - calc="));
      Serial.print(cifradoEsperadoAntici);
      Serial.print(F(" recv="));
      Serial.print(longitud);
      Serial.println(F("]"));
      // Saltar bytes restantes para mantener sincronización
      for (int i = 0; i < longitud; i++) recibirByteSync();
      recibirByteSync();  // checksum
      recibirByteSync();  // fin
      esperarCanalLibre();
      return;
    }
  }

  byte checkCalc = 0;
  for (int i = 0; i < longitud; i++) {
    datosGlobal[i] = recibirByteSync();
    checkCalc += datosGlobal[i];
  }

  byte checkRecibido = recibirByteSync();
  byte fin = recibirByteSync();

  if (checkCalc != checkRecibido) {
    Serial.print(F("<<:Fallos recepcion de mensaje: [checksum calc=0x"));
    Serial.print(checkCalc, HEX);
    Serial.print(F(" recibido=0x"));
    Serial.print(checkRecibido, HEX);
    Serial.println(F("]"));
    esperarCanalLibre();
    return;
  }

  if (fin != FIN_TRAMA) {
    Serial.print(F("<<:Fallos recepcion de mensaje: [fin invalido 0x"));
    Serial.print(fin, HEX);
    Serial.println(F("]"));
    esperarCanalLibre();
    return;
  }

  memcpy(datosCifradosGlobal, datosGlobal, longitud);
  descifrarAES(datosGlobal, longitud);

  Serial.print(F("<<:[paq "));
  Serial.print(paqActual);
  Serial.print(F("/"));
  Serial.print(paqTotal);
  Serial.print(F("] CIFRADO: "));
  imprimirDatosHex(datosCifradosGlobal, longitud);
  Serial.println();

  if (paqActual == 1) {
    uint16_t longitudMensaje = (uint16_t)((datosGlobal[0] << 8) | datosGlobal[1]);
    uint8_t totalEsperado = calcularNumeroPaquetes(longitudMensaje);

    if (longitudMensaje == 0 || longitudMensaje > MAX_MENSAJE_TOTAL) {
      Serial.println(F("<<:Fallos recepcion de mensaje: [longitud de mensaje invalida]"));
      esperarCanalLibre();
      return;
    }
    if (paqTotal != totalEsperado) {
      Serial.println(F("<<:Fallos recepcion de mensaje: [total de paquetes no coincide]"));
      esperarCanalLibre();
      return;
    }

    resetearBuffer();
    totalPaquetes = paqTotal;
    mensajeLongitudEsperada = longitudMensaje;
  } else if (totalPaquetes != paqTotal || mensajeLongitudEsperada == 0) {
    Serial.println(F("<<:Fallos recepcion de mensaje: [inconsistencia en total de paquetes]"));
    return;
  }

  uint16_t textoLen = obtenerLongitudDatosPaquete(mensajeLongitudEsperada, paqActual);
  uint16_t cifradoEsperado = obtenerLongitudCifradaPaquete((paqActual == 1) ? (uint16_t)(2 + textoLen) : textoLen);
  if (cifradoEsperado != longitud) {
    Serial.println(F("<<:Fallos recepcion de mensaje: [longitud cifrada inesperada]"));
    esperarCanalLibre();
    return;
  }

  uint16_t offsetTexto = (paqActual == 1) ? 0 : (MAX_DATOS_PRIMER_PAQUETE + (uint16_t)(paqActual - 2) * MAX_PAYLOAD);
  uint8_t* fuente = (paqActual == 1) ? (datosGlobal + 2) : datosGlobal;
  memcpy(mensajeBuffer + offsetTexto, fuente, textoLen);

  Serial.print(F("<<:[paq "));
  Serial.print(paqActual);
  Serial.print(F("/"));
  Serial.print(paqTotal);
  Serial.print(F("] DESCIFRADO: "));
  Serial.write(fuente, textoLen);
  Serial.println();

  if ((paquetesRecibidos & (1 << (paqActual - 1))) != 0) {
    Serial.print(F("<<:Fallos recepcion de mensaje: [paq duplicado "));
    Serial.print(paqActual);
    Serial.println(F("]"));
    esperarCanalLibre();
    return;
  }

  paquetesRecibidos |= (1 << (paqActual - 1));
  contadorRecibidos++;

  if (contadorRecibidos == totalPaquetes) {
    Serial.print(F("<<:[De "));
    imprimirMAC(macOrigen);
    
    imprimirMensaje(mensajeBuffer, mensajeLongitudEsperada, totalPaquetes);
    resetearBuffer();
  } else {
    Serial.print(F("<<:[paq "));
    Serial.print(paqActual);
    Serial.print(F("/"));
    Serial.print(paqTotal);
    Serial.println(F(" esperando resto..."));
  }
}
