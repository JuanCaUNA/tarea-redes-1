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

const int pinSensor = A0;  // Receptor para LUX-RING
const int umbral = 100;    // ajustar según sensor
const int bitTimeMs = 20;  // tiempo por bit (20 ms)
const uint8_t MAX_PAQUETES = 15;
const uint8_t MAX_PAYLOAD = 32;
const uint8_t BYTE_INICIO = 0xFC;
const uint8_t BYTE_FIN = 0x00;
const uint8_t MAC_LEN = 6;
const uint8_t PAQUETES_POR_LINEA = 4;
const uint16_t BYTES_POR_LINEA = MAX_PAYLOAD * PAQUETES_POR_LINEA;

// Buffer para reconstrucción de mensajes fragmentados
struct PaqueteRecibido {
  bool recibido;
  uint8_t len;
  uint8_t payload[MAX_PAYLOAD];
};

struct FrameRx {
  uint8_t len;
  uint8_t control;
  uint8_t payload[MAX_PAYLOAD];
  uint8_t checksum;
  uint8_t fin;
};

PaqueteRecibido bufferPaquetes[MAX_PAQUETES];  // máximo 15 paquetes (4 bits)
int totalPaquetesEsperados = 0;
int paquetesRecibidos = 0;

void limpiarBuffer() {
  for (uint8_t i = 0; i < MAX_PAQUETES; i++) {
    bufferPaquetes[i].recibido = false;
    bufferPaquetes[i].len = 0;
  }
}

void reiniciarRecepcionMensaje(uint8_t totalPaq) {
  limpiarBuffer();
  totalPaquetesEsperados = totalPaq;
  paquetesRecibidos = 0;
}

void descartarBytes(uint8_t cantidad) {
  for (uint8_t i = 0; i < cantidad; i++) {
    readByteFromSensor();
  }
}

void copiarPayload(uint8_t *destino, const uint8_t *origen, uint8_t len) {
  for (uint8_t i = 0; i < len; i++) {
    destino[i] = origen[i];
  }
}

void reportarErrorRecepcion(const char *detalle) {
  Serial.print("<<:Fallos recepcion de mensaje: ");
  Serial.println(detalle);
}

void setup() {
  Serial.begin(9600);
  pinMode(pinSensor, INPUT);
  Serial.println("<<:RECEPTOR LUX-RING listo");
  reiniciarRecepcionMensaje(0);
}

void loop() {
  if (!hayInicioDeTrama()) {
    return;
  }

  FrameRx frame;
  if (!leerFrame(frame)) {
    return;
  }

  uint8_t paqActual = 0;
  uint8_t totalPaq = 0;
  if (!decodificarControl(frame.control, paqActual, totalPaq)) {
    reportarErrorRecepcion("control invalido");
    return;
  }

  if (!validarChecksumYFin(frame)) {
    return;
  }

  if (!almacenarPaquete(frame, paqActual, totalPaq)) {
    return;
  }

  Serial.print("<<:[paq ");
  Serial.print(paqActual);
  Serial.print("/");
  Serial.print(totalPaq);
  Serial.println("] recibido");

  if (paquetesRecibidos == totalPaquetesEsperados) {
    reconstruirMensaje(totalPaquetesEsperados);
    reiniciarRecepcionMensaje(0);
  }
}

bool hayInicioDeTrama() {
  int lectura = analogRead(pinSensor);
  if (lectura <= umbral) return false;

  uint8_t inicio = readByteFromSensor();
  return (inicio == BYTE_INICIO);
}

bool leerFrame(FrameRx &frame) {
  // Leer y descartar MAC origen/destino para este escenario
  descartarBytes(MAC_LEN);
  descartarBytes(MAC_LEN);

  frame.len = readByteFromSensor();
  frame.control = readByteFromSensor();

  if (!validarLongitud(frame.len)) {
    reportarErrorRecepcion("longitud invalida");
    return false;
  }

  for (int i = 0; i < frame.len; i++) {
    frame.payload[i] = readByteFromSensor();
  }

  frame.checksum = readByteFromSensor();
  frame.fin = readByteFromSensor();
  return true;
}

bool validarLongitud(uint8_t len) {
  return (len > 0 && len <= MAX_PAYLOAD);
}

bool decodificarControl(uint8_t control, uint8_t &paqActual, uint8_t &totalPaq) {
  paqActual = (control >> 4) & 0x0F;
  totalPaq = control & 0x0F;
  return (paqActual > 0 && paqActual <= MAX_PAQUETES && totalPaq > 0 && totalPaq <= MAX_PAQUETES);
}

uint8_t calcularChecksum(const uint8_t *data, uint8_t len) {
  uint16_t sum = 0;
  for (int i = 0; i < len; i++) {
    sum += data[i];
  }
  return (uint8_t)(sum & 0xFF);
}

bool validarChecksumYFin(const FrameRx &frame) {
  if (frame.fin != BYTE_FIN) {
    reportarErrorRecepcion("fin de trama invalido");
    return false;
  }

  uint8_t esperado = calcularChecksum(frame.payload, frame.len);
  if (esperado != frame.checksum) {
    Serial.print("<<:Fallos recepcion de mensaje: checksum falla [esperado: ");
    Serial.print(esperado);
    Serial.print(", recibido: ");
    Serial.print(frame.checksum);
    Serial.println("]");
    return false;
  }

  return true;
}

bool almacenarPaquete(const FrameRx &frame, uint8_t paqActual, uint8_t totalPaq) {
  // Primer paquete de un nuevo mensaje
  if (paquetesRecibidos == 0) {
    reiniciarRecepcionMensaje(totalPaq);
  }

  if (totalPaq != totalPaquetesEsperados) {
    reportarErrorRecepcion("total de paquetes inconsistente");
    return false;
  }

  uint8_t idx = paqActual - 1;
  if (bufferPaquetes[idx].recibido) {
    reportarErrorRecepcion("paquete duplicado ignorado");
    return false;
  }

  bufferPaquetes[idx].recibido = true;
  bufferPaquetes[idx].len = frame.len;
  copiarPayload(bufferPaquetes[idx].payload, frame.payload, frame.len);

  paquetesRecibidos++;
  return true;
}

void reconstruirMensaje(int totalPaq) {
  char mensajeCompleto[(MAX_PAYLOAD * MAX_PAQUETES) + 1];
  int pos = 0;

  for (int i = 0; i < totalPaq; i++) {
    if (bufferPaquetes[i].recibido) {
      for (int j = 0; j < bufferPaquetes[i].len; j++) {
        mensajeCompleto[pos++] = (char)bufferPaquetes[i].payload[j];
      }
    }
  }
  mensajeCompleto[pos] = '\0';

  int inicio = 0;
  while (inicio < pos) {
    int fin = inicio + BYTES_POR_LINEA;
    if (fin > pos) fin = pos;

    int paqInicio = (inicio / MAX_PAYLOAD) + 1;
    int paqFin = ((fin + MAX_PAYLOAD - 1) / MAX_PAYLOAD);
    if (paqFin > totalPaq) paqFin = totalPaq;

    Serial.print("<<:[paq ");
    Serial.print(paqInicio);
    Serial.print("-");
    Serial.print(paqFin);
    Serial.print("/");
    Serial.print(totalPaq);
    Serial.print("] ");

    for (int i = inicio; i < fin; i++) {
      Serial.write(mensajeCompleto[i]);
    }
    Serial.println();
    inicio = fin;
  }
}

uint8_t readByteFromSensor() {
  uint8_t result = 0;
  // sincronizar leyendo la mitad del bit inicial
  delay(bitTimeMs / 2);
  for (int bit = 0; bit < 8; bit++) {
    int v = analogRead(pinSensor);
    if (v > umbral) bitWrite(result, bit, 1);
    else bitWrite(result, bit, 0);
    delay(bitTimeMs);
  }
  // pequeña pausa antes del siguiente byte
  delay(bitTimeMs / 2);
  return result;
}