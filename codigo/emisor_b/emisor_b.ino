#define PIN_LASER 9
#define CLAVE_XOR 0x3C
#define ID_PROPIO 1

uint8_t cero = 100;

void enviarInicio() {
  digitalWrite(PIN_LASER, HIGH);
  delay(5 * cero);
  digitalWrite(PIN_LASER, LOW);
  delay(cero);
}

void enviarFin() {
  digitalWrite(PIN_LASER, HIGH);
  delay(7 * cero);
  digitalWrite(PIN_LASER, LOW);
  delay(cero);
}

void enviarBit(bool bit) {
  if (!bit) {
    digitalWrite(PIN_LASER, HIGH);
    delay(cero);
    digitalWrite(PIN_LASER, LOW);
    delay(cero);
  } else {
    digitalWrite(PIN_LASER, HIGH);
    delay(3 * cero);
    digitalWrite(PIN_LASER, LOW);
    delay(cero);
  }
}

void enviarByte(byte b) {
  for (int i = 7; i >= 0; i--) enviarBit((b >> i) & 1);
}
void enviarBits(byte val, int nbits) {
  for (int i = nbits - 1; i >= 0; i--) enviarBit((val >> i) & 1);
}

void enviarMensaje(byte destino, String mensaje) {
  byte longitud = min((int)mensaje.length(), 30);
  byte datos_cifrados[30];
  byte checksum = 0;

  Serial.println("=============================");
  Serial.print("Mensaje original:     ");
  Serial.println(mensaje);

  Serial.print("Datos cifrados (HEX): ");
  for (int i = 0; i < longitud; i++) {
    datos_cifrados[i] = mensaje[i] ^ CLAVE_XOR;
    checksum ^= datos_cifrados[i];
    Serial.print("0x");
    if (datos_cifrados[i] < 16) Serial.print("0");
    Serial.print(datos_cifrados[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  Serial.print("Checksum (HEX):       0x");
  if (checksum < 16) Serial.print("0");
  Serial.println(checksum, HEX);
  Serial.println("Transmitiendo...");

  enviarInicio();
  enviarBits(destino, 4);
  enviarBits(ID_PROPIO, 4);
  enviarBits(longitud, 5);
  for (int i = 0; i < longitud; i++) enviarByte(datos_cifrados[i]);
  enviarByte(checksum);
  enviarFin();

  Serial.println("Transmision completa.");
  Serial.println("=============================");
}

void procesarComando(String linea) {
  linea.trim();
  int c1 = linea.indexOf(',');

  if (c1 < 0) {
    Serial.println("[ERROR] Formato invalido. Usa: DESTINO,MENSAJE");
    return;
  }

  byte destino = (byte)linea.substring(0, c1).toInt();
  String msg = linea.substring(c1 + 1);

  if (msg.length() == 0) {
    Serial.println("[ERROR] Mensaje vacio.");
    return;
  }

  enviarMensaje(destino, msg);
}

void setup() {
  pinMode(PIN_LASER, OUTPUT);
  digitalWrite(PIN_LASER, LOW);
  Serial.begin(9600);
  Serial.println("=== CREEPER Emisor listo ===");
  Serial.print("ID de este nodo: ");
  Serial.println(ID_PROPIO);
  Serial.print("Clave XOR: 0x");
  Serial.println(CLAVE_XOR, HEX);
  Serial.println("Esperando comando Serial...");
  Serial.println("Formato: DESTINO,MENSAJE");
  Serial.println("Ejemplo:  2,Hola mundo");
  Serial.println("=============================");
}

void loop() {
  if (Serial.available() > 0) {
    String linea = Serial.readString();
    procesarComando(linea);
    Serial.println("Listo. Ingrese otro comando:");
  }
}