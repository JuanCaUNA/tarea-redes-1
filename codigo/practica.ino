// ======================================================
// COMUNICACIÓN FULL DUPLEX POR LÁSER (Arduino A y B)
// ======================================================

const int pinLaser = 13;
const int pinSensor = A0;

// CONFIGURACIÓN DE PARÁMETROS
const int umbral = 100;     // Ajusta según la potencia de tu láser
const int delayBit = 50;    // Milisegundos por bit (Sincronización)
const int startPulse = 100; // Pulso inicial para despertar al receptor

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
    int lectura = analogRead(pinSensor);

    // Si la luz supera el umbral, detectamos un posible bit de inicio
    if (lectura > umbral)
    {
        recibirMensaje();
    }
}

// Función para convertir texto en pulsos de luz
void transmitirMensaje(String msg)
{
    for (int i = 0; i < msg.length(); i++)
    {
        char c = msg[i];

        // 1. Bit de Inicio (Start Bit)
        digitalWrite(pinLaser, HIGH);
        delay(startPulse);
        digitalWrite(pinLaser, LOW);
        delay(delayBit);

        // 2. Envío del byte (8 bits)
        for (int bit = 0; bit < 8; bit++)
        {
            bool estado = bitRead(c, bit);
            digitalWrite(pinLaser, estado);
            delay(delayBit);
        }

        // 3. Espacio entre caracteres
        digitalWrite(pinLaser, LOW);
        delay(delayBit * 2);
    }
    Serial.print(">>> Enviado: ");
    Serial.println(msg);
}

// Función para leer los pulsos y convertirlos en texto
void recibirMensaje()
{
    char caracterRecibido = 0;

    // Esperamos a que pase el pulso de inicio
    delay(startPulse + (delayBit / 2));

    // Leemos los 8 bits
    for (int bit = 0; bit < 8; bit++)
    {
        int lectura = analogRead(pinSensor);
        if (lectura > umbral)
        {
            bitWrite(caracterRecibido, bit, 1);
        }
        else
        {
            bitWrite(caracterRecibido, bit, 0);
        }
        delay(delayBit);
    }

    Serial.print(caracterRecibido); // Imprime el caracter en el monitor

    // Pequeña pausa para no detectar el mismo pulso dos veces
    delay(delayBit);
}