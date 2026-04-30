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
const int PIN_LDR         = A0;
const int UMBRAL          = 20;    // *** AJUSTAR según calibración ***
const unsigned long DUR_BIT_US = 50000UL;   // 50 ms en microsegundos
const unsigned long MUESTREO_US = 10000UL;  // muestreo a mitad del bit

// ── Constantes del protocolo ─────────────────────────────────
const byte INICIO_TRAMA   = 0xFC;  // 11111100
const byte FIN_TRAMA      = 0x00;  // 00000000
const byte MAX_PAYLOAD    = 32;    // bytes máximo de datos cifrados

// ── Identidad del nodo (MAC propia, 6 bytes) ─────────────────
const byte MI_MAC[6] = { 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };

// ── Clave AES-128 precompartida (16 bytes) ───────────────────
// DEBE coincidir exactamente con la clave del emisor
// AES deshabilitado: clave no usada

// ── Buffer para reensamblado de paquetes múltiples ───────────
// Soporta hasta 15 paquetes × 32 bytes = 480 bytes máx (campo Control: 4 bits)
const int MAX_PAQUETES    = 15;
byte mensajeBuffer[MAX_PAQUETES * MAX_PAYLOAD];
bool paquetesRecibidos[MAX_PAQUETES]; // registro de qué paquetes llegaron
int  totalPaquetes        = 0;
int  contadorRecibidos    = 0;

// ════════════════════════════════════════════════════════════
//  Funciones de capa física
// ════════════════════════════════════════════════════════════

bool hayLuz() {
  return analogRead(PIN_LDR) < UMBRAL;
}

// Lee un byte OOK completo, MSB primero.
// Se llama justo cuando debe comenzar el primer bit.
byte recibirByte() {
  byte b = 0;
  unsigned long tInicioByte = micros();

  for (int i = 7; i >= 0; i--) {
    // Esperar al punto de muestreo del bit (centro del bit)
    unsigned long tMuestra = tInicioByte
                             + (unsigned long)(7 - i) * DUR_BIT_US
                             + MUESTREO_US;
    while (micros() < tMuestra);

    if (hayLuz()) bitSet(b, i);
  }

  // Esperar a que termine el byte completo antes de devolver
  unsigned long tFin = tInicioByte + 8UL * DUR_BIT_US;
  while (micros() < tFin);

  return b;
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
  const int BYTES_POR_LINEA = PAQ_POR_LINEA * MAX_PAYLOAD; // 128

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
    int bytesFin   = min(byteOffset + bytesLinea, totalBytes);
    for (int i = byteOffset; i < bytesFin; i++) {
      if (texto[i] == '\0') break;
      Serial.print(texto[i]);
    }
    Serial.println();

    byteOffset  += bytesLinea;
    paqImpreso  += (paqHasta - paqImpreso);
  }
}

// ════════════════════════════════════════════════════════════
//  Reiniciar buffer de reensamblado
// ════════════════════════════════════════════════════════════
void resetearBuffer() {
  memset(mensajeBuffer, 0, sizeof(mensajeBuffer));
  memset(paquetesRecibidos, false, sizeof(paquetesRecibidos));
  totalPaquetes      = 0;
  contadorRecibidos  = 0;
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

  // ── PASO 1: Esperar luz (inicio de actividad) ────────────
  if (!hayLuz()) return;

  // ── PASO 2: Buscar byte de inicio 0xFC (11111100) ────────
  // Se intenta leer un byte; si no es 0xFC se descarta y se
  // vuelve a esperar (tolerancia ante ruido).
  byte posibleInicio = recibirByte();
  if (posibleInicio != INICIO_TRAMA) {
    // No era trama válida; esperar silencio antes de reintentar
    while (hayLuz());
    return;
  }

  // ── PASO 3: Leer campos de cabecera ──────────────────────
  byte macOrigen[6], macDestino[6];

  for (int i = 0; i < 6; i++) macOrigen[i]  = recibirByte();
  for (int i = 0; i < 6; i++) macDestino[i] = recibirByte();

  byte longitud = recibirByte();  // tamaño del payload cifrado
  byte control  = recibirByte();  // [7:4] paq_actual, [3:0] paq_total

  byte paqActual = (control >> 4) & 0x0F;   // bits altos
  byte paqTotal  = (control)      & 0x0F;   // bits bajos

  // ── PASO 4: Validar destino ───────────────────────────────
  if (!macIgual(macDestino, MI_MAC)) {
    Serial.print("<<:Fallos recepcion de mensaje: [trama para otra MAC ");
    imprimirMAC(macDestino);
    Serial.println("]");
    // Consumir el resto de la trama para limpiar el canal
    for (int i = 0; i < longitud; i++) recibirByte();
    recibirByte(); // checksum
    recibirByte(); // fin
    return;
  }

  // ── PASO 5: Validar longitud ──────────────────────────────
  if (longitud == 0 || longitud > MAX_PAYLOAD) {
    Serial.print("<<:Fallos recepcion de mensaje: [longitud invalida ");
    Serial.print(longitud);
    Serial.println("]");
    return;
  }

  // ── PASO 6: Validar números de paquete ───────────────────
  if (paqActual == 0 || paqTotal == 0 || paqActual > paqTotal) {
    Serial.print("<<:Fallos recepcion de mensaje: [control invalido 0x");
    Serial.print(control, HEX);
    Serial.println("]");
    return;
  }

  // ── PASO 7: Leer payload cifrado ─────────────────────────
  byte datos[MAX_PAYLOAD];
  byte checkCalc = 0;

  for (int i = 0; i < longitud; i++) {
    datos[i]   = recibirByte();
    checkCalc += datos[i];   // CheckSum = suma de bytes de datos mod 256
  }
  // checkCalc ya es mod 256 por desbordamiento natural del byte

  // ── PASO 8: Verificar checksum ───────────────────────────
  byte checkRecibido = recibirByte();

  if (checkCalc != checkRecibido) {
    Serial.print("<<:Fallos recepcion de mensaje: [checksum error calc=0x");
    Serial.print(checkCalc, HEX);
    Serial.print(" recibido=0x");
    Serial.print(checkRecibido, HEX);
    Serial.println("]");
    recibirByte(); // consumir byte de FIN aunque haya error
    return;
  }

  // ── PASO 9: Verificar byte de FIN ────────────────────────
  byte fin = recibirByte();
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