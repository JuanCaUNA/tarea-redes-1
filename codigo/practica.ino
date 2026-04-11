// ======================================================
// COMUNICACIÓN FULL DUPLEX POR LÁSER (Arduino A y B)
// ======================================================

const int pinLaser = 13;
const int pinSensor = A0;

// CONFIGURACIÓN DE PARÁMETROS
const int umbral = 100;         // Misma calibración base del protocolo LUX-RING
const int bitTimeMs = 20;       // Duración de bit según RFC actualizado (50 Hz)
const int startPulseMs = 40;    // Pulso de inicio: 2 bits para sincronizar
const int gapEntreBytesMs = 40; // Espacio entre bytes para reducir falsas detecciones

bool leerBitSensor()
{
    return analogRead(pinSensor) > umbral;
}

void escribirBitLaser(bool bit)
{
    digitalWrite(pinLaser, bit ? HIGH : LOW);
    delay(bitTimeMs);
}

void enviarPulsoInicio()
{
    digitalWrite(pinLaser, HIGH);
    delay(startPulseMs);
    digitalWrite(pinLaser, LOW);
    delay(bitTimeMs);
}

void transmitirByte(uint8_t b)
{
    enviarPulsoInicio();

    for (uint8_t bit = 0; bit < 8; bit++)
    {
        escribirBitLaser(bitRead(b, bit));
    }

    digitalWrite(pinLaser, LOW);
    delay(gapEntreBytesMs);
}

bool hayInicioDeByte()
{
    if (!leerBitSensor())
    {
        return false;
    }

    delay(startPulseMs);
    return !leerBitSensor();
}

uint8_t recibirByte()
{
    uint8_t valor = 0;

    // Muestreo en el centro del primer bit.
    delay(bitTimeMs / 2);
    for (uint8_t bit = 0; bit < 8; bit++)
    {
        bitWrite(valor, bit, leerBitSensor() ? 1 : 0);
        delay(bitTimeMs);
    }

    return valor;
}

void setup()
{
    Serial.begin(9600);
    pinMode(pinLaser, OUTPUT);
    digitalWrite(pinLaser, LOW);
    Serial.println("--- SISTEMA FULL DUPLEX ACTIVO ---");
    Serial.println("Escribe tu mensaje y presiona Enter:");
}

void loop()
{
    // --- PARTE 1: TRANSMISIÓN (TX) ---
    if (Serial.available() > 0)
    {
        String mensaje = Serial.readStringUntil('\n');
        transmitirMensaje(mensaje);
    }

    // --- PARTE 2: RECEPCIÓN (RX) ---
    if (hayInicioDeByte())
    {
        recibirByteYMostrar();
    }
}

// Función para convertir texto en pulsos de luz
void transmitirMensaje(const String &msg)
{
    for (int i = 0; i < msg.length(); i++)
    {
        transmitirByte((uint8_t)msg[i]);
    }

    Serial.print(">>> Enviado: ");
    Serial.println(msg);
}

// Función para leer los pulsos y convertirlos en texto
void recibirByteYMostrar()
{
    char caracterRecibido = (char)recibirByte();
    Serial.println(caracterRecibido);
    delay(bitTimeMs);
}