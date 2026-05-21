#define PIN_SENSOR 2

const uint8_t PULSO_ESTADO = HIGH; // Cambia a LOW si el sensor entrega pulso invertido.
const unsigned long PULSE_TIMEOUT_US = 1200000UL;
const uint16_t CERO_MS = 100;
const unsigned long CERO_US = (unsigned long)CERO_MS * 1000UL;
const unsigned long HEARTBEAT_MS = 1000UL;

unsigned long ultimoReporteMs = 0;
unsigned long totalPulsos = 0;
unsigned long pulsosInvalidos = 0;

const char *nombreSimbolo(int simbolo) {
  switch (simbolo) {
    case 1:
      return "BIT0";
    case 3:
      return "BIT1";
    case 5:
      return "INICIO";
    case 7:
      return "FIN";
    default:
      return "DESCONOCIDO";
  }
}

void setup() {
  pinMode(PIN_SENSOR, INPUT);
  Serial.begin(9600);
  Serial.println("=== Monitor de Pulsos (Receptor) ===");
  Serial.print("Pin sensor: ");
  Serial.println(PIN_SENSOR);
  Serial.print("Pulso estado: ");
  Serial.println(PULSO_ESTADO == HIGH ? "HIGH" : "LOW");
  Serial.print("Cero (ms): ");
  Serial.println(CERO_MS);
  Serial.println("Esperando pulsos...");
  Serial.println("====================================");
}

void loop() {
  unsigned long duracionUs = pulseIn(PIN_SENSOR, PULSO_ESTADO, PULSE_TIMEOUT_US);
  unsigned long ahoraMs = millis();

  if (duracionUs == 0) {
    if (ahoraMs - ultimoReporteMs >= HEARTBEAT_MS) {
      ultimoReporteMs = ahoraMs;
      Serial.print("[INFO] Sin pulso. Total: ");
      Serial.print(totalPulsos);
      Serial.print(" | Invalidos: ");
      Serial.println(pulsosInvalidos);
    }
    return;
  }

  float unidades = (float)duracionUs / (float)CERO_US;
  int simbolo = (int)round(unidades);
  bool conocido = (simbolo == 1 || simbolo == 3 || simbolo == 5 || simbolo == 7);

  totalPulsos++;
  if (!conocido) pulsosInvalidos++;

  Serial.print("Pulso us=");
  Serial.print(duracionUs);
  Serial.print(" | unidades=");
  Serial.print(unidades, 2);
  Serial.print(" | simbolo=");
  Serial.print(simbolo);
  Serial.print(" (");
  Serial.print(nombreSimbolo(simbolo));
  Serial.println(")");
}
