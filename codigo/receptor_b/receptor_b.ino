class Receptor {
private:
  const uint8_t PIN_LASER = 9;
  const uint8_t PIN_REC = 2;
  const uint8_t CLAVE_XOR = 0x3C;
  const uint8_t ID_PROPIO = 2;
  const uint8_t DELAY_UNIT = 100;

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
  Receptor() {}

  void inicializar() {
    pinMode(PIN_LASER, OUTPUT);
    digitalWrite(PIN_LASER, LOW);
    pinMode(PIN_REC, INPUT);
    Serial.begin(9600);
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
      Serial.println("  (No es para este nodo, descartado.)");
    }

    Serial.println("=============================");
  }
};

Receptor receptor;

void setup() {
  receptor.inicializar();
}

void loop() {
  receptor.recibirMensaje();
}
