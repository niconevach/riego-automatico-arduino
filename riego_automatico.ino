// =====================================================================
//  SISTEMA DE RIEGO AUTOMÁTICO CON MONITOREO DE TEMPERATURA
//  Segundo Parcial - Integración Tecnológica
//  Autor: Nicolás Nevach
//
//  Funcionamiento:
//  - Lee la humedad del suelo y la temperatura ambiente.
//  - Si la tierra está seca, activa la bomba de agua (motor) por medio
//    de un relé. Si hace calor, riega antes (umbral más alto).
//  - Muestra humedad, temperatura y estado de la bomba en un LCD 16x2.
//  - LED verde = tierra húmeda / LED rojo = tierra seca, regando.
//  - Pulsador = riego manual mientras se mantiene presionado.
// =====================================================================

// ---------------------- LIBRERÍAS ----------------------
#include <LiquidCrystal.h>   // Librería para manejar la pantalla LCD 16x2

// ---------------------- PINES ----------------------
// Si algún cable está en otro pin, solo hay que cambiar el número acá.
const int PIN_HUMEDAD     = A0;  // Señal del sensor de humedad del suelo
const int PIN_TEMPERATURA = A2;  // Salida del sensor de temperatura TMP36
const int PIN_RELE        = 7;   // Base del transistor que activa el relé
const int PIN_LED_ROJO    = 9;   // LED rojo: tierra seca / regando
const int PIN_LED_VERDE   = 8;   // LED verde: tierra húmeda
const int PIN_PULSADOR    = 2;   // Pulsador de riego manual

// Pines del LCD en este orden: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(12, 11, 5, 4, 3, 6);

// ---------------------- CONSTANTES DE CONFIGURACIÓN ----------------------
const int   UMBRAL_NORMAL = 40;   // % de humedad debajo del cual se riega
const int   UMBRAL_CALOR  = 50;   // % de humedad para regar cuando hace calor
const float TEMP_CALOR    = 30.0; // °C a partir de los cuales se considera calor

// ---------------------- VARIABLES GLOBALES ----------------------
int   humedad     = 0;      // Humedad del suelo en porcentaje (0 a 100)
float temperatura = 0.0;    // Temperatura en grados Celsius
bool  regando     = false;  // true si la bomba está encendida

// =====================================================================
//  SETUP: se ejecuta una sola vez al encender el Arduino
// =====================================================================
void setup() {
  // Los pines que controlan algo se configuran como SALIDA
  pinMode(PIN_RELE, OUTPUT);
  pinMode(PIN_LED_ROJO, OUTPUT);
  pinMode(PIN_LED_VERDE, OUTPUT);

  // El pulsador es una ENTRADA. Está conectado entre el pin y GND, así que
  // activamos la resistencia pull-up interna del Arduino: se lee HIGH cuando
  // está suelto y LOW cuando se presiona (no hace falta resistencia externa)
  pinMode(PIN_PULSADOR, INPUT_PULLUP);

  // Arrancamos con la bomba apagada por seguridad
  digitalWrite(PIN_RELE, LOW);

  // Inicializamos el LCD indicando que tiene 16 columnas y 2 filas
  lcd.begin(16, 2);
  lcd.print("Riego automatico");
  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");
  delay(2000);
  lcd.clear();

  // Monitor serie para ver los valores desde la computadora
  Serial.begin(9600);
}

// =====================================================================
//  LOOP: se repite constantemente
// =====================================================================
void loop() {
  // 1) Leer los sensores
  humedad     = leerHumedad(PIN_HUMEDAD);
  temperatura = leerTemperatura(PIN_TEMPERATURA);

  // 2) Elegir el umbral según la temperatura:
  //    si hace calor la tierra se seca más rápido, entonces regamos antes
  int umbral;
  if (temperatura >= TEMP_CALOR) {
    umbral = UMBRAL_CALOR;
  } else {
    umbral = UMBRAL_NORMAL;
  }

  // 3) Decidir si hay que regar
  bool pulsadorApretado = (digitalRead(PIN_PULSADOR) == LOW); // LOW = presionado

  if (pulsadorApretado) {
    regando = true;              // Riego manual: tiene prioridad
  } else if (humedad < umbral) {
    regando = true;              // Tierra seca: riego automático
  } else {
    regando = false;             // Tierra húmeda: no regar
  }

  // 4) Actuar: bomba, LEDs y pantalla
  controlarBomba(regando);
  actualizarLeds(regando);
  mostrarEnLCD(humedad, temperatura, regando, pulsadorApretado);

  // 5) Mostrar datos en el monitor serie
  Serial.print("Humedad: ");
  Serial.print(humedad);
  Serial.print("%  Temp: ");
  Serial.print(temperatura);
  Serial.print("C  Bomba: ");
  Serial.println(regando ? "ON" : "OFF");

  delay(500);  // Pequeña pausa entre lecturas
}

// =====================================================================
//  FUNCIONES PERSONALIZADAS (con parámetros)
// =====================================================================

// Lee el sensor de humedad conectado en 'pin' y devuelve un porcentaje.
// En Tinkercad el sensor entrega de 0 (seco) a 876 (muy húmedo).
int leerHumedad(int pin) {
  int lectura = analogRead(pin);                 // Valor crudo 0-1023
  int porcentaje = map(lectura, 0, 876, 0, 100); // Lo pasamos a 0-100 %
  return constrain(porcentaje, 0, 100);          // Aseguramos el rango
}

// Lee el TMP36 conectado en 'pin' y devuelve la temperatura en °C.
float leerTemperatura(int pin) {
  int lectura = analogRead(pin);            // Valor crudo 0-1023
  float voltaje = lectura * 5.0 / 1024.0;   // Convertimos a voltios
  return (voltaje - 0.5) * 100.0;           // Fórmula del TMP36
}

// Enciende (true) o apaga (false) la bomba a través del relé.
void controlarBomba(bool encender) {
  if (encender) {
    digitalWrite(PIN_RELE, HIGH);  // El transistor activa la bobina del relé
  } else {
    digitalWrite(PIN_RELE, LOW);   // El relé se abre y la bomba se detiene
  }
}

// Prende el LED que corresponde según si se está regando o no.
void actualizarLeds(bool estaRegando) {
  digitalWrite(PIN_LED_ROJO,  estaRegando ? HIGH : LOW);
  digitalWrite(PIN_LED_VERDE, estaRegando ? LOW  : HIGH);
}

// Muestra los datos en el LCD.
// Fila 1: humedad y temperatura. Fila 2: estado de la bomba.
void mostrarEnLCD(int hum, float temp, bool estaRegando, bool manual) {
  lcd.setCursor(0, 0);
  lcd.print("H:");
  lcd.print(hum);
  lcd.print("% T:");
  lcd.print(temp, 1);   // Temperatura con 1 decimal
  lcd.print("C   ");    // Espacios para borrar restos del valor anterior

  lcd.setCursor(0, 1);
  if (estaRegando && manual) {
    lcd.print("Riego MANUAL    ");
  } else if (estaRegando) {
    lcd.print("Regando...      ");
  } else {
    lcd.print("Tierra OK       ");
  }
}
