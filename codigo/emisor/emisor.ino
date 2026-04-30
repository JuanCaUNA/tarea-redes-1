/*
* RECEPTOR - RFC-UNA-2026 / Protocolo LUX-RING
*/

// * Emisor para LUX-RING
const int pinLaser = 13;

// * Tiempo por cada bit 50 ms por bit
const unsigned long BIT_TIME_US = 50 * 1000UL;

// ! New
// * silencio previo para sincronizar
const unsigned long GUARD_TIME_US = BIT_TIME_US * 2;
// * Premabulo: 10101010, para ayudar a alinear muestreo
const uint8_t PREAMBLE_BYTE = 0xAA;
// * Bytes de preambulo
const uint8_t PREAMBLE_LEN = 1;

// * Reglas Definidas en el RF
const uint8_t MAX_PAYLOAD = 32;
const uint8_t MAX_PAQUETES = 15;
const uint8_t BYTE_INICIO = 0xFC;
const uint8_t BYTE_FIN = 0x00;
const uint8_t MAC_LEN = 6;
const uint8_t PAQUETES_POR_LINEA = 4;
const uint16_t BYTES_POR_LINEA = MAX_PAYLOAD * PAQUETES_POR_LINEA;

// TODO definir un gestion adecuada de mac (debe ser un valor indentificador personalizado de 1-15) Esto por comunicacion con otro protocolo que usa 4bits de MAC
// * Direcciones MAC (6 bytes)
const uint8_t MAC_ORIGEN[6] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
const uint8_t MAC_DESTINO[6] = { 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };
// ! Debe coincidir con MI_MAC del receptor

uint8_t calcularNumeroPaquetes(int msgLen) {
  uint8_t numPaquetes = (uint8_t)((msgLen + MAX_PAYLOAD - 1) / MAX_PAYLOAD);
  if (numPaquetes > MAX_PAQUETES) {
    Serial.println(">>:Aviso mensaje truncado al maximo de 15 paquetes (480 bytes)");
    return MAX_PAQUETES;
  }
  return numPaquetes;
}

void imprimirMensajePorLineas(const String &msg, uint8_t totalPaq) {
  int inicio = 0;
  int largoTotal = msg.length();
  int maxBytes = largoTotal;
  int bytesEnviados = (int)totalPaq * MAX_PAYLOAD;
  if (bytesEnviados < maxBytes) maxBytes = bytesEnviados;
  // * imprimir solo lo enviado

  while (inicio < maxBytes) {
    int fin = inicio + BYTES_POR_LINEA;
    if (fin > maxBytes) fin = maxBytes;

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

    // * CORRECCIÓN CRÍTICA: Elimina caracteres invisibles como \r o espacios extra
    mensaje.trim();
    enviarMensajeConFragmentacion(mensaje);
  }
}

void enviarMensajeConFragmentacion(const String &msg) {
  int msgLen = msg.length();
  if (msgLen == 0) return;

  uint8_t numPaquetes = calcularNumeroPaquetes(msgLen);
  // * Enviar cada paquete
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
    delay(100);  // pausa entre paquetes
  }

  imprimirMensajePorLineas(msg, numPaquetes);
}

void enviarPaquete(const String &msg, int offset, int len, uint8_t paqNum, uint8_t totalPaq) {
  if (len > MAX_PAYLOAD) len = MAX_PAYLOAD;
  if (len <= 0) return;

  // * Silencio previo para que el receptor detecte el inicio sin ruido
  digitalWrite(pinLaser, LOW);
  unsigned long t0 = micros();
  while (micros() - t0 < GUARD_TIME_US)
    ;

  for (uint8_t i = 0; i < PREAMBLE_LEN; i++) {
    transmitByteMSB(PREAMBLE_BYTE);
  }

  transmitByteMSB(BYTE_INICIO);

  for (uint8_t i = 0; i < MAC_LEN; i++) transmitByteMSB(MAC_ORIGEN[i]);
  for (uint8_t i = 0; i < MAC_LEN; i++) transmitByteMSB(MAC_DESTINO[i]);

  // * Longitud del payload
  transmitByteMSB((uint8_t)len);
  transmitByteMSB(((paqNum & 0x0F) << 4) | (totalPaq & 0x0F));
  // * Control: paq actual | total paq

  uint8_t checksum = 0;
  for (int i = 0; i < len; i++) {
    uint8_t b = (uint8_t)msg[offset + i];
    transmitByteMSB(b);

    // * CheckSum = suma de bytes de datos mod 256
    checksum += b;
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

    // * CORRECCIÓN CRÍTICA: Retardo exacto basado en el reloj interno
    while (micros() - startBit < BIT_TIME_US)
      ;
  }
}