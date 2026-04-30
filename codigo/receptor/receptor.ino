/* =============================================================
 * RECEPTOR - RFC-UNA-2026 / Protocolo LUX-RING
 * Autores RFC: Juan Carlos Camacho Solano, Noemi Murillo Godínez
 * =============================================================
 *
 * Capa Física:
 *   - OOK: luz ON = '1', luz OFF = '0'
 *   - Duración de bit: 50 ms ( Hz)
 *   - Sincronización: detección del byte de inicio 0xFC (11111100)
 *
 * Formato de trama:
 *   [Inicio 1B][MAC_Orig 6B][MAC_Dest 6B][Longitud 1B][Control 1B]
 *   [Datos 1-32B][CheckSum 1B][Fin 1B]
 *
 * Seguridad: AES-128 con clave precompartida de 16 bytes
 * ============================================================= */

// #include <AESLib.h>   // AES deshabilitado

// ── Pines y tiempos ──────────────────────────────────────────
const int PIN_LDR = A0;

// *** AJUSTAR según calibración ***
const int UMBRAL = 20;
// * 50 ms en microsegundos (DEBE COINCIDIR CON EMISOR)
const unsigned long DUR_BIT_US = 50 * 1000UL;
// * Muestreo a mitad del bit
const unsigned long MUESTREO_US = DUR_BIT_US / 2;

// !New
// * Tiempo minimo de silencio antes de sincronizar
const unsigned long SILENCIO_US = DUR_BIT_US * 2;
// * ventana de busqueda del byte de inicio
const unsigned int MAX_SYNC_BITS = 200;

// ── Constantes del protocolo ─────────────────────────────────
const byte INICIO_TRAMA = 0xFC;  // 11111100
const byte FIN_TRAMA = 0x00;     // 00000000
const byte MAX_PAYLOAD = 32;     // bytes máximo de datos cifrados

// ── Identidad del nodo (MAC propia, 6 bytes) ─────────────────
const byte MI_MAC[6] = { 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };

// ── Clave AES-128 precompartida (16 bytes) ───────────────────
// DEBE coincidir exactamente con la clave del emisor
// AES deshabilitado: clave no usada

// ── Buffer para reensamblado de paquetes múltiples ───────────
// Soporta hasta 15 paquetes × 32 bytes = 480 bytes máx (campo Control: 4 bits)
const int MAX_PAQUETES = 15;
byte mensajeBuffer[MAX_PAQUETES * MAX_PAYLOAD];
bool paquetesRecibidos[MAX_PAQUETES];  // registro de qué paquetes llegaron
int totalPaquetes = 0;
int contadorRecibidos = 0;
unsigned long nextBitStartUs = 0;

// ════════════════════════════════════════════════════════════
//  Funciones de capa física
// ════════════════════════════════════════════════════════════

bool hayLuz() {
  return analogRead(PIN_LDR) < UMBRAL;
}

// Espera un flanco de subida (oscuridad -> luz) despues de un silencio minimo.
// Devuelve false si no aparece en el tiempo dado.
bool esperarFlancoSubida(unsigned long timeoutUs) {
  unsigned long t0 = micros();
  unsigned long silencioInicio = 0;
  bool enSilencio = false;

  while (micros() - t0 < timeoutUs) {
    if (hayLuz()) {
      enSilencio = false;
      continue;
    }

    if (!enSilencio) {
      enSilencio = true;
      silencioInicio = micros();
    }

    if (micros() - silencioInicio >= SILENCIO_US) {
      while (!hayLuz()) {
        if (micros() - t0 > timeoutUs) return false;
      }
      nextBitStartUs = micros();
      return true;
    }
  }
  return false;
}

// Espera a un tiempo absoluto (maneja overflow de micros).
void esperarHasta(unsigned long tObjetivo) {
  while ((long)(micros() - tObjetivo) < 0) {
    // espera activa
  }
}

// Lee un bit alineado a la base de tiempo interna.
bool recibirBitSync() {
  unsigned long sampleTime = nextBitStartUs + MUESTREO_US;
  esperarHasta(sampleTime);
  bool bit = hayLuz();
  nextBitStartUs += DUR_BIT_US;
  esperarHasta(nextBitStartUs);
  return bit;
}

// Lee un byte OOK completo, MSB primero, usando sincronizacion por bits.
byte recibirByteSync() {
  byte b = 0;
  for (int i = 7; i >= 0; i--) {
    if (recibirBitSync()) bitSet(b, i);
  }
  return b;
}

// Busca el byte de inicio 0xFC desplazando bit a bit.
bool buscarInicioTrama() {
  byte shift = 0;
  for (unsigned int i = 0; i < MAX_SYNC_BITS; i++) {
    shift = (byte)((shift << 1) | (recibirBitSync() ? 1 : 0));
    if (shift == INICIO_TRAMA) return true;
  }
  return false;
}

// ════════════════════════════════════════════════════════════
//  Funciones auxiliares
// ════════════════════════════════════════════════════════════

// Compara dos MACs de 6 bytes. Devuelve true si son iguales.
bool macIgual(const byte* a, const byte* b) {
  for (int i = 0; i < 6; i++) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

// Imprime una MAC como HH:HH:HH:HH:HH:HH
void imprimirMAC(const byte* mac) {
  for (int i = 0; i < 6; i++) {
    if (mac[i] < 0x10) Serial.print("0");
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(":");
  }
}

// Descifra un bloque de 'len' bytes con AES-128 en modo ECB.
// Los datos descifrados quedan en el mismo buffer.
// AESLib trabaja en bloques de 16 bytes; se rellena con 0 si len < 16.
void descifrarAES(byte* datos, byte len) {
  // AES deshabilitado: no se modifica el payload
  (void)datos;
  (void)len;
}

// ════════════════════════════════════════════════════════════
//  Impresión formateada según RFC (prefijo <<:, 4 paq/línea)
// ════════════════════════════════════════════════════════════

// Imprime el mensaje final ensamblado respetando el límite de
// 4 paquetes (128 bytes) por línea de consola.
void imprimirMensaje(const char* texto, int totalBytes, int totalPaq) {
  // Cuántos paquetes entran por línea
  const int PAQ_POR_LINEA = 4;
  const int BYTES_POR_LINEA = PAQ_POR_LINEA * MAX_PAYLOAD;  // 128

  int paqImpreso = 0;
  int byteOffset = 0;

  while (paqImpreso < totalPaq) {
    int paqDesde = paqImpreso + 1;
    int paqHasta = min(paqImpreso + PAQ_POR_LINEA, totalPaq);

    Serial.print("<<:[paq ");
    Serial.print(paqDesde);
    Serial.print("-");
    Serial.print(paqHasta);
    Serial.print("/");
    Serial.print(totalPaq);
    Serial.print("] ");

    // Imprimir los bytes correspondientes a esos paquetes
    int bytesLinea = (paqHasta - paqImpreso) * MAX_PAYLOAD;
    int bytesFin = min(byteOffset + bytesLinea, totalBytes);
    for (int i = byteOffset; i < bytesFin; i++) {
      if (texto[i] == '\0') break;
      Serial.print(texto[i]);
    }
    Serial.println();

    byteOffset += bytesLinea;
    paqImpreso += (paqHasta - paqImpreso);
  }
}

// ════════════════════════════════════════════════════════════
//  Reiniciar buffer de reensamblado
// ════════════════════════════════════════════════════════════
void resetearBuffer() {
  memset(mensajeBuffer, 0, sizeof(mensajeBuffer));
  memset(paquetesRecibidos, false, sizeof(paquetesRecibidos));
  totalPaquetes = 0;
  contadorRecibidos = 0;
}

// ════════════════════════════════════════════════════════════
//  setup / loop
// ════════════════════════════════════════════════════════════

void setup() {
  Serial.begin(9600);
  resetearBuffer();
  Serial.println("Receptor LUX-RING listo. Esperando trama...");
}

void loop() {

  // ── PASO 1: Esperar flanco de subida para alinear bits ───
  if (!esperarFlancoSubida(500000UL)) return;

  // ── PASO 2: Buscar byte de inicio 0xFC bit a bit ─────────
  if (!buscarInicioTrama()) {
    Serial.println("<<:Fallos recepcion de mensaje: [no se encontro byte de inicio]");
    while (hayLuz())
      ;
    return;
  }

  // ── PASO 3: Leer campos de cabecera ──────────────────────
  byte macOrigen[6], macDestino[6];

  for (int i = 0; i < 6; i++) macOrigen[i] = recibirByteSync();
  for (int i = 0; i < 6; i++) macDestino[i] = recibirByteSync();

  byte longitud = recibirByteSync();  // tamaño del payload cifrado
  byte control = recibirByteSync();   // [7:4] paq_actual, [3:0] paq_total

  byte paqActual = (control >> 4) & 0x0F;  // bits altos
  byte paqTotal = (control)&0x0F;          // bits bajos

  // ── PASO 4: Validar destino ───────────────────────────────
  if (!macIgual(macDestino, MI_MAC)) {
    Serial.print("<<:Fallos recepcion de mensaje: [trama para otra MAC ");
    imprimirMAC(macDestino);
    Serial.println("]");
    // Consumir el resto de la trama para limpiar el canal
    for (int i = 0; i < longitud; i++) recibirByteSync();
    recibirByteSync();  // checksum
    recibirByteSync();  // fin
    return;
  }

  // ── PASO 5: Validar longitud ──────────────────────────────
  if (longitud == 0 || longitud > MAX_PAYLOAD) {
    Serial.print("<<:Fallos recepcion de mensaje: [longitud invalida ");
    Serial.print(longitud);
    Serial.println("]");
    while (hayLuz())
      ;
    return;
  }

  // ── PASO 6: Validar números de paquete ───────────────────
  if (paqActual == 0 || paqTotal == 0 || paqActual > paqTotal || paqTotal > MAX_PAQUETES) {
    Serial.print("<<:Fallos recepcion de mensaje: [control invalido 0x");
    Serial.print(control, HEX);
    Serial.println("]");
    while (hayLuz())
      ;
    return;
  }

  // ── PASO 7: Leer payload cifrado ─────────────────────────
  byte datos[MAX_PAYLOAD];
  byte checkCalc = 0;

  for (int i = 0; i < longitud; i++) {
    datos[i] = recibirByteSync();
    checkCalc += datos[i];  // CheckSum = suma de bytes de datos mod 256
  }
  // checkCalc ya es mod 256 por desbordamiento natural del byte

  // ── PASO 8: Verificar checksum ───────────────────────────
  byte checkRecibido = recibirByteSync();

  // ── PASO 9: Verificar byte de FIN ────────────────────────
  byte fin = recibirByteSync();

  if (checkCalc != checkRecibido) {
    Serial.print("<<:Fallos recepcion de mensaje: [checksum calc=0x");
    Serial.print(checkCalc, HEX);
    Serial.print(" recibido=0x");
    Serial.print(checkRecibido, HEX);
    Serial.println("]");
    return;
  }

  if (fin != FIN_TRAMA) {
    Serial.print("<<:Fallos recepcion de mensaje: [fin invalido 0x");
    Serial.print(fin, HEX);
    Serial.println("]");
    return;
  }

  // ── PASO 10: Descifrar payload AES-128 ───────────────────
  descifrarAES(datos, longitud);

  // ── PASO 11: Almacenar en buffer de reensamblado ─────────
  // Si es el primer paquete del mensaje, inicializar buffer
  if (paqActual == 1) {
    resetearBuffer();
    totalPaquetes = paqTotal;
  } else if (totalPaquetes != paqTotal) {
    // Inconsistencia: diferente total que el paquete 1
    Serial.println("<<:Fallos recepcion de mensaje: [inconsistencia en total de paquetes]");
    return;
  }

  // Guardar datos descifrados en la posición correcta del buffer
  int offset = (paqActual - 1) * MAX_PAYLOAD;
  if (paquetesRecibidos[paqActual - 1]) {
    Serial.print("<<:Fallos recepcion de mensaje: [paq duplicado ");
    Serial.print(paqActual);
    Serial.println("]");
    return;
  }

  memcpy(mensajeBuffer + offset, datos, longitud);
  paquetesRecibidos[paqActual - 1] = true;
  contadorRecibidos++;

  // ── PASO 12: Si se completó el mensaje, imprimir ─────────
  if (contadorRecibidos == totalPaquetes) {

    // Calcular longitud real del texto (puede haber padding AES)
    int totalBytes = totalPaquetes * MAX_PAYLOAD;
    // Buscar fin real de string (null terminator del padding AES)
    int longitudReal = 0;
    for (int i = 0; i < totalBytes; i++) {
      if (mensajeBuffer[i] == '\0') break;
      longitudReal++;
    }

    // Mostrar información del remitente
    Serial.print("<<:[De ");
    imprimirMAC(macOrigen);
    Serial.println("]");

    // Imprimir mensaje respetando formato RFC (4 paq/línea)
    imprimirMensaje((const char*)mensajeBuffer, longitudReal, totalPaquetes);

    resetearBuffer();
  } else {
    // Paquete recibido pero mensaje incompleto aún
    Serial.print("<<:[paq ");
    Serial.print(paqActual);
    Serial.print("/");
    Serial.print(paqTotal);
    Serial.println(" recibido, esperando resto...]");
  }
}