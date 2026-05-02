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
const byte MAX_PAYLOAD = 32;  // * Tamaño uniforme de todos los paquetes
const byte MAX_PAQUETES = 15;
const uint16_t MAX_MENSAJE_TOTAL = MAX_PAYLOAD * MAX_PAQUETES;  // * 480 bytes máximo

// * Identidad del nodo (MAC propia, 6 bytes) 
const byte MI_MAC[6] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00 };

// * Clave AES-128 precompartida (16 bytes) 
const uint8_t AES_KEY[AES_KEY_SIZE] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66 };

static aes128_ctx_t aesCtx;
static uint8_t aesInitFlag = 0;

// * * Buffer para reensamblado de paquetes multiples 
byte mensajeBuffer[MAX_MENSAJE_TOTAL];
// * Bitfield: bit i = 1 si paquete i recibido
uint16_t paquetesRecibidos = 0;  
uint8_t totalPaquetes = 0;
uint8_t contadorRecibidos = 0;
uint16_t mensajeLongitudEsperada = 0;
unsigned long nextBitStartUs = 0;
// * Buffer reutilizable para datos
byte datosGlobal[MAX_PAYLOAD];   
// * Buffer reutilizable para datos cifrados
byte datosCifradosGlobal[MAX_PAYLOAD];  


// * ! Funciones de capa fisica

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

void descartarBytes(uint8_t cantidad) {
  for (uint8_t i = 0; i < cantidad; i++) {
    recibirByteSync();
  }
}

void descartarRestoTrama(uint8_t longitudPayload) {
  descartarBytes(longitudPayload);
  recibirByteSync();  // * checksum
  recibirByteSync();  // * fin
}

// ! Funciones auxiliares

inline uint8_t truncarMacA4Bits(uint8_t valor) {
  // * Conserva solo 4 bits para el ID MAC logico (0-15).
  return (uint8_t)(valor & 0x0F);
}

inline uint8_t obtenerIdMac4Bits(const byte* mac) {
  // * Se ignoran los otros 5 bytes: solo se usa el primer byte truncado.
  return truncarMacA4Bits(mac[0]);
}

inline void imprimirMAC(const byte* mac) {
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 0x10) Serial.print(F("0"));
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(F(":"));
  }
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

// * Imprime el mensaje completo junto con el ID de origen en formato <<:ipOrigen:Mensaje
void imprimirMensajeCompleto(const byte* macOrigen, const uint8_t* texto, uint16_t totalBytes) {
  uint8_t idOrigen4 = obtenerIdMac4Bits(macOrigen);
  Serial.print(F("<<:"));
  Serial.print(idOrigen4);
  Serial.print(F(":"));
  Serial.write(texto, totalBytes);
  Serial.println();
}

void inicializarAES() {
  if (!aesInitFlag) {
    aes128_key_expansion(&aesCtx, AES_KEY);
    aesInitFlag = 1;
  }
}

// ! setup / loop

void setup() {
  Serial.begin(9600);
  resetearBuffer();
  inicializarAES();
  Serial.println(F("Receptor LUX-RING listo. Esperando trama..."));
}

void loop() {
  if (!esperarFlancoSubida(500000UL)) return;

  if (!buscarInicioTrama()) {
    Serial.println(F("<<:log:error: no se encontro byte de inicio"));
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

  uint8_t idDestino4 = obtenerIdMac4Bits(macDestino);
  uint8_t idPropio4 = obtenerIdMac4Bits(MI_MAC);
  if (idDestino4 != idPropio4) {
    Serial.print(F("<<:log:error: trama para otro destino. dest="));
    Serial.print(idDestino4);
    Serial.print(F(" propio="));
    Serial.println(idPropio4);

    descartarRestoTrama(longitud);
    esperarCanalLibre();
    return;
  }

  if (longitud == 0 || longitud > MAX_PAYLOAD || (longitud % AES_BLOCK_SIZE) != 0) {
    Serial.print(F("<<:log:error: longitud invalida "));
    Serial.print(longitud);
    esperarCanalLibre();
    return;
  }

  if (paqActual == 0 || paqTotal == 0 || paqActual > paqTotal || paqTotal > MAX_PAQUETES) {
    Serial.print(F("<<:log:error: control invalido 0x"));
    Serial.print(control, HEX);
    Serial.println(F("]"));
    esperarCanalLibre();
    return;
  }

  // * VERIFICACIÓN ANTICIPADA: Para paquete 2+, verificar tamaño ANTES de leer datos
  if (paqActual > 1 && totalPaquetes > 0 && mensajeLongitudEsperada > 0) {
    uint16_t textoLenEsperado = obtenerLongitudDatosPaquete(mensajeLongitudEsperada, paqActual);
    uint16_t cifradoEsperadoAntici = obtenerLongitudCifradaPaquete(textoLenEsperado);
    if (cifradoEsperadoAntici != longitud) {
      Serial.print(F("<<:log:error: tamaño incorrecto: paq "));
      Serial.print(paqActual);
      Serial.print(F(" - calc="));
      Serial.print(cifradoEsperadoAntici);
      Serial.print(F(" recv="));
      Serial.print(longitud);
      // * Saltar bytes restantes para mantener sincronización
      descartarRestoTrama(longitud);
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
    Serial.print(F("<<:log:error: [checksum calc=0x"));
    Serial.print(checkCalc, HEX);
    Serial.print(F(" recibido=0x"));
    Serial.print(checkRecibido, HEX);
    Serial.println(F("]"));
    esperarCanalLibre();
    return;
  }

  if (fin != FIN_TRAMA) {
    Serial.print(F("<<:log:error: fin invalido 0x"));
    Serial.print(fin, HEX);
    Serial.println(F("]"));
    esperarCanalLibre();
    return;
  }

  memcpy(datosCifradosGlobal, datosGlobal, longitud);
  descifrarAES(datosGlobal, longitud);

  Serial.print(F("<<:log:[paq "));
  Serial.print(paqActual);
  Serial.print(F("/"));
  Serial.print(paqTotal);
  Serial.print(F("] CIFRADO: "));
  imprimirDatosHex(datosCifradosGlobal, longitud);
  Serial.println();

  if (paqActual == 1) {
    // * Primer paquete: inicializar estado de reensamblado
    if (paqTotal > MAX_PAQUETES) {
      Serial.println(F("<<:log:error: total de paquetes excede maximo"));
      esperarCanalLibre();
      return;
    }
    resetearBuffer();
    totalPaquetes = paqTotal;
    mensajeLongitudEsperada = 0;  // * Se calculará cuando todos los paquetes lleguen
  } else if (totalPaquetes != paqTotal || totalPaquetes == 0) {
    Serial.println(F("<<:log:error: inconsistencia en total de paquetes"));
    esperarCanalLibre();
    return;
  }

  // * Calcular longitud real del plaintext (remover PKCS7 padding en último paquete)
  uint8_t textoLen = longitud;
  if (paqActual == totalPaquetes && longitud > 0) {
    // * Último paquete: remover padding PKCS7
    uint8_t padLen = datosGlobal[longitud - 1];
    if (padLen > 0 && padLen <= AES_BLOCK_SIZE) {
      textoLen = longitud - padLen;
    }
  }

  uint16_t offsetTexto = (uint16_t)(paqActual - 1) * MAX_PAYLOAD;
  uint8_t* fuente = datosGlobal;  // * Todos los paquetes tienen datos desde offset 0
  memcpy(mensajeBuffer + offsetTexto, fuente, textoLen);

  Serial.print(F("<<:log: [paq "));
  Serial.print(paqActual);
  Serial.print(F("/"));
  Serial.print(paqTotal);
  Serial.print(F("] DESCIFRADO: "));
  Serial.write(fuente, textoLen);
  Serial.println();

  if ((paquetesRecibidos & (1 << (paqActual - 1))) != 0) {
    Serial.print(F("<<:log:error: [paq duplicado "));
    Serial.print(paqActual);
    Serial.println(F("]"));
    esperarCanalLibre();
    return;
  }

  paquetesRecibidos |= (1 << (paqActual - 1));
  contadorRecibidos++;

  if (contadorRecibidos == totalPaquetes) {
    // * Calcular longitud total real (último paquete puede ser parcial)
    mensajeLongitudEsperada = (uint16_t)((totalPaquetes - 1) * MAX_PAYLOAD + textoLen);
    
    Serial.print(F("<<:log:[De "));
    imprimirMAC(macOrigen);
    Serial.println();

    // * Imprimir mensaje completo en el formato solicitado (única salida final)
    imprimirMensajeCompleto(macOrigen, mensajeBuffer, mensajeLongitudEsperada);
    resetearBuffer();
  } else {
    Serial.print(F("<<:log:[paq "));
    Serial.print(paqActual);
    Serial.print(F("/"));
    Serial.print(paqTotal);
    Serial.println();
  }
}
