const int pinLaser = 13;            // Emisor para LUX-RING 
const int bitTimeMs = 20;           // tiempo por bit según RFC (20 ms)
const uint8_t MAX_PAYLOAD = 32;
const uint8_t MAX_PAQUETES = 15;
const uint8_t BYTE_INICIO = 0xFC;
const uint8_t BYTE_FIN = 0x00;
const uint8_t MAC_LEN = 6;
const uint8_t FRAME_OVERHEAD = 17;  // Inicio + MACs + Longitud + Control + Checksum + Fin
const uint8_t FRAME_MAX_LEN = FRAME_OVERHEAD + MAX_PAYLOAD;
const uint8_t PAQUETES_POR_LINEA = 4;
const uint16_t BYTES_POR_LINEA = MAX_PAYLOAD * PAQUETES_POR_LINEA;

// Direcciones MAC (6 bytes) — ajustar si se desea
const uint8_t MAC_ORIGEN[6] = {0x01,0x02,0x03,0x04,0x05,0x06};
const uint8_t MAC_DESTINO[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}; // broadcast por defecto

uint8_t calcularChecksum(const uint8_t *data, uint8_t len) {
  uint16_t sum = 0;
  for (uint8_t i = 0; i < len; i++) {
    sum += data[i];
  }
  return (uint8_t)(sum & 0xFF);
}

uint8_t calcularNumeroPaquetes(int msgLen) {
  uint8_t numPaquetes = (uint8_t)((msgLen + MAX_PAYLOAD - 1) / MAX_PAYLOAD);
  if (numPaquetes > MAX_PAQUETES) {
    Serial.println(">>:Aviso mensaje truncado al maximo de 15 paquetes (480 bytes)");
    return MAX_PAQUETES;
  }
  return numPaquetes;
}

void copiarMac(uint8_t *frame, int &idx, const uint8_t *mac) {
  for (uint8_t i = 0; i < MAC_LEN; i++) {
    frame[idx++] = mac[i];
  }
}

uint8_t copiarPayloadYChecksum(const String &msg, int offset, int len, uint8_t *frame, int &idx) {
  uint8_t payload[MAX_PAYLOAD];
  for (int i = 0; i < len; i++) {
    uint8_t b = (uint8_t)msg[offset + i];
    payload[i] = b;
    frame[idx++] = b;
  }
  return calcularChecksum(payload, (uint8_t)len);
}

void imprimirMensajePorLineas(const String &msg, uint8_t totalPaq) {
  int inicio = 0;
  int largoTotal = msg.length();

  while (inicio < largoTotal) {
    int fin = inicio + BYTES_POR_LINEA;
    if (fin > largoTotal) fin = largoTotal;

    uint8_t paqInicio = (uint8_t)(inicio / MAX_PAYLOAD) + 1;
    uint8_t paqFin = (uint8_t)((fin + MAX_PAYLOAD - 1) / MAX_PAYLOAD);
    if (paqFin > totalPaq) paqFin = totalPaq;

    Serial.print(">>:[paq ");
    Serial.print(paqInicio);
    Serial.print("-");
    Serial.print(paqFin);
    Serial.print("/");
    Serial.print(totalPaq);
    Serial.print("] ");
    Serial.println(msg.substring(inicio, fin));

    inicio = fin;
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(pinLaser, OUTPUT);
  digitalWrite(pinLaser, LOW);
  Serial.println(">>:EMISOR LUX-RING listo");
  Serial.println(">>:Escribe mensaje y presiona Enter");
}

void loop() {
  if (Serial.available() > 0) {
    String mensaje = Serial.readStringUntil('\n');
    enviarMensajeConFragmentacion(mensaje);
  }
}

void enviarMensajeConFragmentacion(const String &msg) {
  int msgLen = msg.length();
  if (msgLen == 0) return;
  
  uint8_t numPaquetes = calcularNumeroPaquetes(msgLen);
  
  // Enviar cada paquete
  for (uint8_t paqNum = 1; paqNum <= numPaquetes; paqNum++) {
    int offset = (paqNum - 1) * MAX_PAYLOAD;
    int payloadLen = msgLen - offset;
    if (payloadLen > MAX_PAYLOAD) payloadLen = MAX_PAYLOAD;
    
    enviarPaquete(msg, offset, payloadLen, paqNum, numPaquetes);
    Serial.print(">>:[paq ");
    Serial.print(paqNum);
    Serial.print("/");
    Serial.print(numPaquetes);
    Serial.println("] enviado");
    delay(100); // pausa entre paquetes
  }

  imprimirMensajePorLineas(msg, numPaquetes);
}

void enviarPaquete(const String &msg, int offset, int len, uint8_t paqNum, uint8_t totalPaq) {
  uint8_t frame[FRAME_MAX_LEN];
  int idx = 0;
  
  frame[idx++] = BYTE_INICIO;

  copiarMac(frame, idx, MAC_ORIGEN);
  copiarMac(frame, idx, MAC_DESTINO);
  
  frame[idx++] = (uint8_t)len; // Longitud del payload
  frame[idx++] = ((paqNum & 0x0F) << 4) | (totalPaq & 0x0F); // Control: paq actual | total paq

  frame[idx++] = copiarPayloadYChecksum(msg, offset, len, frame, idx);
  frame[idx++] = BYTE_FIN;
  
  transmitFrame(frame, idx);
}

void transmitFrame(uint8_t *buf, int len) {
  for (int i = 0; i < len; i++) {
    transmitByteLSB(buf[i]);
    delay(bitTimeMs); // espacio corto entre bytes
  }
  digitalWrite(pinLaser, LOW);
}

void transmitByteLSB(uint8_t b) {
  for (int bit = 0; bit < 8; bit++) {
    bool estado = (b >> bit) & 0x01;
    digitalWrite(pinLaser, estado ? HIGH : LOW);
    delay(bitTimeMs);
  }
}