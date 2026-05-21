// * Forward declaration para acceso entre clases
class Emisor;

class Receptor {
private:
  const uint8_t PIN_LASER = 9;
  const uint8_t PIN_REC = 2;
  const uint8_t CLAVE_XOR = 0x3C;
  const uint8_t ID_PROPIO = 2;
  const uint8_t DELAY_UNIT = 100;

  // Referencia al emisor para reenvío
  Emisor* ptrEmisor;

  // Métodos privados de recepción
  int leerPulso() {
    unsigned long dur = pulseIn(PIN_REC, LOW, 2000000UL);
    if (dur == 0) return 0;
    return (int)(dur / 100000); // Convierte a múltiplos de 100ms
  }

  bool esperarInicio() {
    int dur = leerPulso();
    return (dur == 5); // Pulso de inicio: 500ms
  }

  int leerBit() {
    int dur = leerPulso();
    if (dur == 0)  return -2; // Timeout
    if (dur >= 5)  return -1; // Pulso de control inesperado
    return (dur >= 2) ? 1 : 0; // 1: ~300ms, 0: ~100ms
  }

  byte leerBits(int nbits) {
    byte resultado = 0;
    for (int i = nbits - 1; i >= 0; i--) {
      int b = leerBit();
      if (b < 0) return resultado;
      if (b) resultado |= (1 << i);
    }
    return resultado;
  }

  byte leerByte() {
    return leerBits(8);
  }

public:
  Receptor() : ptrEmisor(nullptr) {}

  void inicializar(Emisor* emisor) {
    ptrEmisor = emisor;
    pinMode(PIN_LASER, OUTPUT);
    digitalWrite(PIN_LASER, LOW);
    pinMode(PIN_REC, INPUT);
    Serial.println("=== Receptor listo ===");
    Serial.print("ID de este nodo: "); Serial.println(ID_PROPIO);
    Serial.print("Escuchando en pin: "); Serial.println(PIN_REC);
    Serial.println("Esperando mensajes...");
    Serial.println("=============================");
  }

  void recibirMensaje() {
    if (!esperarInicio()) return;

    Serial.println("=============================");
    Serial.println("[RX] Inicio detectado. Recibiendo trama...");

    byte destino  = leerBits(4);
    byte origen   = leerBits(4);
    byte longitud = leerBits(5);

    if (longitud == 0 || longitud > 30) {
      Serial.println("[ERROR] Longitud de trama invalida.");
      return;
    }

    byte datos[30];
    byte checksum_calc = 0;
    for (int i = 0; i < longitud; i++) {
      datos[i] = leerByte();
      checksum_calc ^= datos[i];
    }

    byte checksum_rx = leerByte();

    int fin = leerPulso();
    if (fin != 7) {
      Serial.println("[WARN] Pulso FIN inesperado.");
    }

    if (checksum_rx != checksum_calc) {
      Serial.println("[ERROR] Checksum incorrecto. Trama descartada.");
      return;
    }

    Serial.println("=============================");
    Serial.print("Trama recibida de Nodo "); Serial.println(origen);
    Serial.print("Destino: Nodo ");          Serial.println(destino);
    Serial.print(">> Datos cifrados (HEX): ");
    for (int i = 0; i < longitud; i++) {
      Serial.print("0x");
      if (datos[i] < 16) Serial.print("0");
      Serial.print(datos[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    Serial.println("   (sin clave: ilegible)");

    if (destino == ID_PROPIO) {
      Serial.print(">> Mensaje descifrado: ");
      for (int i = 0; i < longitud; i++) Serial.print((char)(datos[i] ^ CLAVE_XOR));
      Serial.println();
    } else {
      Serial.print("[REENVIO] Reenviando a Nodo "); Serial.println(destino);
      if (ptrEmisor != nullptr) {
        reenviarMensaje(destino, origen, datos, longitud);
      }
    }

    Serial.println("=============================");
  }

  void reenviarMensaje(uint8_t destino, uint8_t origen, byte* datos, uint8_t longitud) {
    // Convierte los datos recibidos en string para el emisor
    String mensaje;
    for (uint8_t i = 0; i < longitud; i++) {
      mensaje += (char)datos[i];
    }
    
    // Construye MAC destino con los 4 bits
    uint8_t macDestino[6];
    for (uint8_t i = 0; i < 6; i++) {
      macDestino[i] = destino;
    }
    
    Serial.print("[REENVIO] Enviando ...");
    ptrEmisor->enviarMensajeConFragmentacion(mensaje, macDestino);
  }
};

Receptor receptor;

#include "aes_minimal.h"

struct ComandoEnvio {
  bool valido;
  String mensaje;
  uint8_t macDestino[6];
};

class Emisor {
private:
  // * Constantes
  static const int PIN_LASER = 13;
  static const unsigned long BIT_TIME_US = 50 * 1000UL;
  static const unsigned long GUARD_TIME_US = BIT_TIME_US * 2;
  static const uint8_t MAX_PAYLOAD = 32;
  static const uint8_t MAX_PAQUETES = 15;
  static const uint8_t BYTE_INICIO = 0xFC;
  static const uint8_t BYTE_FIN = 0x00;
  static const uint8_t MAC_LEN = 6;
  static const uint16_t MAX_MENSAJE_TOTAL = MAX_PAYLOAD * MAX_PAQUETES;

  // * Direcciones MAC (6 bytes)
  static const uint8_t MAC_ORIGEN[6];

  // * Clave AES-128 compartida
  static const uint8_t AES_KEY[AES_KEY_SIZE];

  // * Estado
  aes128_ctx_t aesCtx;
  uint8_t aesInitFlag;

  // * Métodos privados
  uint8_t truncarMacA4Bits(uint8_t valor) {
    return (uint8_t)(valor & 0x0F);
  }

  void construirMacDestino(uint8_t macId4Bits, uint8_t macDestino[MAC_LEN]) {
    for (uint8_t i = 0; i < MAC_LEN; i++) {
      macDestino[i] = macId4Bits;
    }
  }

  uint8_t calcularNumeroPaquetes(uint16_t msgLen) {
    if (msgLen == 0) return 0;
    if (msgLen <= MAX_PAYLOAD) return 1;
    uint8_t numPaquetes = (uint8_t)((msgLen + MAX_PAYLOAD - 1) / MAX_PAYLOAD);
    if (numPaquetes > MAX_PAQUETES) {
      Serial.println(F(">>:log: Aviso mensaje truncado al maximo de 15 paquetes (480 bytes)"));
      return MAX_PAQUETES;
    }
    return numPaquetes;
  }

  uint16_t obtenerLongitudDatosPaquete(uint16_t msgLen, uint8_t paqNum) {
    if (paqNum == 0 || msgLen == 0) return 0;
    uint16_t offset = (uint16_t)(paqNum - 1) * MAX_PAYLOAD;
    if (offset >= msgLen) return 0;
    uint16_t restante = msgLen - offset;
    return (restante > MAX_PAYLOAD) ? MAX_PAYLOAD : restante;
  }

  uint16_t obtenerLongitudCifradaPaquete(uint16_t datosPlainLen) {
    return (uint16_t)(((datosPlainLen + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE);
  }

  uint8_t obtenerBytePlanoPaquete(const String &msg, uint16_t msgLen, uint8_t paqNum, uint16_t indiceEnPayload, uint16_t datosPlainLen) {
    uint16_t offsetPaquete = (uint16_t)(paqNum - 1) * MAX_PAYLOAD;
    uint16_t indiceTexto = offsetPaquete + indiceEnPayload;
    if (indiceTexto >= msgLen) return 0;
    if (indiceEnPayload >= datosPlainLen) return 0;
    return (uint8_t)msg[indiceTexto];
  }

  void aplicarPaddingPKCS7(uint8_t bloque[AES_BLOCK_SIZE], uint8_t datosLen) {
    uint8_t padLen = AES_BLOCK_SIZE - datosLen;
    for (uint8_t i = datosLen; i < AES_BLOCK_SIZE; i++) {
      bloque[i] = padLen;
    }
  }

  void transmitByteMSB(uint8_t b) {
    for (int bit = 7; bit >= 0; bit--) {
      bool estado = (b >> bit) & 0x01;
      unsigned long startBit = micros();
      digitalWrite(PIN_LASER, estado ? HIGH : LOW);
      while (micros() - startBit < BIT_TIME_US) {
      }
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

  void enviarPaquete(const String &msg, uint16_t msgLen, uint8_t paqNum, uint8_t totalPaq, uint16_t datosPlainLen, uint16_t payloadCifradoLen, const uint8_t macDestino[MAC_LEN]) {
    if (datosPlainLen == 0) return;

    digitalWrite(PIN_LASER, LOW);
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
    digitalWrite(PIN_LASER, LOW);
  }

public:
  Emisor() : aesInitFlag(0) {
    memset(&aesCtx, 0, sizeof(aesCtx));
  }

  void inicializar() {
    pinMode(PIN_LASER, OUTPUT);
    digitalWrite(PIN_LASER, LOW);
    inicializarAES();
    Serial.println(F("EMISOR LUX-RING listo"));
    Serial.println(F("Usa formato N:mensaje (N entre 0 y 15)"));
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

  void procesarEntrada(const String &entrada) {
    ComandoEnvio cmd = parsearComandoEnvio(entrada);
    if (!cmd.valido) {
      Serial.println(F(">>:log: Formato invalido. Usa N:mensaje con N entre 0 y 15"));
      return;
    }
    enviarMensajeConFragmentacion(cmd.mensaje, cmd.macDestino);
  }
};

// * Inicializar constantes estáticas
const uint8_t Emisor::MAC_ORIGEN[6] = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };
const uint8_t Emisor::AES_KEY[AES_KEY_SIZE] = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66 };

// * Instancia global
Emisor emisor;


void setup() {
  Serial.begin(9600);
  delay(500);
  
  // Inicializar receptor con referencia al emisor
  receptor.inicializar(&emisor);
  
  delay(500);
  
  // Inicializar emisor
  emisor.inicializar();
  
  Serial.println();
  Serial.println("=== SISTEMA COMPLETO INICIALIZADO ===");
  Serial.println("Modo: RECEPTOR + EMISOR (con reenvío automático)");
  Serial.println("Enviar: N:mensaje (N entre 0 y 15)");
  Serial.println("=====================================");
}

void loop() {
  // Escuchar mensajes entrantes
  receptor.recibirMensaje();

  // Permitir envío manual por serial
  if (Serial.available() > 0) {
    emisor.procesarEntrada(Serial.readStringUntil('\n'));
  }
}
