/*
  CENTINELA - Alarma de intrusos con barrido (Arduino Uno)
  Sensores: ultrasonico (sobre servo) + PIR
  Actuadores: servo, rele con sirena, buzzer
  HMI: LCD 16x2, LED RGB, boton | Protocolo: UART 9600
*/
#include <Servo.h>
#include <LiquidCrystal.h>

// ---------- PINES ----------
#define PIN_SERVO 2
#define TRIG 7
#define ECHO 8
#define PIR 4
#define LED_R 3
#define LED_G 5
#define LED_B 6
#define RELE A0
#define BUZZER A1
#define BOTON A2

LiquidCrystal lcd(9, 10, 11, 12, 13, A3); // RS, E, D4, D5, D6, D7
Servo servo;

// ---------- AJUSTES ----------
const bool ANODO_COMUN = true;    // true si tu LED RGB es de anodo comun (el tuyo lo es)
const int RELE_ON = LOW;          // modulo activo en bajo (lo comun)
const int RELE_OFF = HIGH;
const byte CLAVE[4] = {0, 0, 1, 0};  // 0 = corta, 1 = larga
const int ANG_MIN = 15, PASO = 15, N_POS = 11;  // 15, 30, ... 165 grados
const int DIST_MAX = 250;         // cm; ignora todo lo mas lejano
const int UMBRAL_CM = 30;         // cuanto mas cerca de lo normal = intruso
const unsigned long T_SIRENA_MS = 8000;
const unsigned long T_LARGA = 600;   // ms para contar como pulsacion larga
const unsigned long T_CLAVE = 4000;  // ms maximo entre pulsaciones
const int PIR_CALENT_S = 30;      // segundos de calentamiento del PIR

// ---------- ESTADOS ----------
#define ST_DESARMADO 0
#define ST_CALIBRANDO 1
#define ST_VIGILANDO 2
#define ST_ALERTA 3
#define ST_ERROR 4
const char* const nombres[] = {"DESARMADO", "CALIBRANDO", "VIGILANDO", "ALERTA", "ERROR"};

byte estado = ST_DESARMADO;
int base[N_POS];  // distancia normal a cada angulo
byte intento[4];
byte nIntento = 0;
byte fallos = 0;
unsigned long tUltimaPulsacion = 0, tAlerta = 0;
int anguloAlerta = -1, distAlerta = 0;

void procesarBoton();

// ---------- SALIDAS ----------
void color(bool r, bool g, bool b) {
  digitalWrite(LED_R, ANODO_COMUN ? !r : r);
  digitalWrite(LED_G, ANODO_COMUN ? !g : g);
  digitalWrite(LED_B, ANODO_COMUN ? !b : b);
}

void escribirLinea(byte fila, const char* txt) {
  lcd.setCursor(0, fila);
  byte n = 0;
  while (txt[n] != 0 && n < 16) { lcd.print(txt[n]); n++; }
  while (n < 16) { lcd.print(' '); n++; }
}

void apagarAlarma() {
  digitalWrite(RELE, RELE_OFF);
  digitalWrite(BUZZER, LOW);
}

void pantallaEstado() {
  char buf[17];
  switch (estado) {
    case ST_DESARMADO:
      escribirLinea(0, "DESARMADO");
      escribirLinea(1, "Pulsa: armar");
      break;
    case ST_CALIBRANDO:
      escribirLinea(0, "CALIBRANDO...");
      escribirLinea(1, "No te muevas");
      break;
    case ST_VIGILANDO:
      escribirLinea(0, "VIGILANDO");
      escribirLinea(1, "Barriendo...");
      break;
    case ST_ALERTA:
      escribirLinea(0, "!! ALERTA !!");
      if (anguloAlerta >= 0) snprintf(buf, 17, "Ang:%3d D:%3dcm", anguloAlerta, distAlerta);
      else snprintf(buf, 17, "3 claves malas");
      escribirLinea(1, buf);
      break;
    case ST_ERROR:
      escribirLinea(0, "ERROR SENSOR");
      escribirLinea(1, "Clave: reiniciar");
      break;
  }
}

void cambiarEstado(byte nuevo) {
  estado = nuevo;
  nIntento = 0;
  switch (nuevo) {
    case ST_DESARMADO:
      apagarAlarma(); color(0, 1, 0); servo.write(90); fallos = 0;
      break;
    case ST_CALIBRANDO:
      color(0, 0, 1);
      break;
    case ST_VIGILANDO:
      color(0, 0, 1);
      break;
    case ST_ALERTA:
      color(1, 0, 0);
      digitalWrite(RELE, RELE_ON);
      digitalWrite(BUZZER, HIGH);
      tAlerta = millis();
      break;
    case ST_ERROR:
      apagarAlarma(); color(1, 0, 0);
      break;
  }
  pantallaEstado();
  Serial.print("ESTADO,");
  Serial.println(nombres[nuevo]);
}

void actualizarSalidas() {
  if (estado == ST_ALERTA) {
    if (millis() - tAlerta > T_SIRENA_MS) apagarAlarma();  // la sirena suena un tiempo limitado
    color((millis() / 300) % 2, 0, 0);                     // rojo parpadeando
  } else if (estado == ST_ERROR) {
    color((millis() / 600) % 2, 0, 0);
  }
}

// ---------- BOTON Y CLAVE ----------
byte leerBoton() {  // 0 nada, 1 corta, 2 larga (se detecta al soltar)
  static bool previo = HIGH;
  static unsigned long t0 = 0;
  bool actual = digitalRead(BOTON);
  byte ev = 0;
  if (previo == HIGH && actual == LOW) { t0 = millis(); delay(30); }
  else if (previo == LOW && actual == HIGH) {
    unsigned long dur = millis() - t0;
    delay(30);
    if (dur > 40) ev = (dur >= T_LARGA) ? 2 : 1;
  }
  previo = actual;
  return ev;
}

void procesarBoton() {
  byte ev = leerBoton();
  if (ev == 0) {
    if (nIntento > 0 && millis() - tUltimaPulsacion > T_CLAVE) { nIntento = 0; pantallaEstado(); }
    return;
  }
  tUltimaPulsacion = millis();
  if (estado == ST_DESARMADO) { cambiarEstado(ST_CALIBRANDO); return; }
  if (estado == ST_CALIBRANDO) return;
  // VIGILANDO, ALERTA o ERROR: se esta tecleando la clave
  intento[nIntento++] = (ev == 2) ? 1 : 0;
  lcd.setCursor(0, 1);
  lcd.print("Clave: ");
  for (byte k = 0; k < nIntento; k++) lcd.print('*');
  for (byte k = nIntento; k < 9; k++) lcd.print(' ');
  if (nIntento < 4) return;
  bool ok = true;
  for (byte k = 0; k < 4; k++) if (intento[k] != CLAVE[k]) ok = false;
  nIntento = 0;
  if (ok) { cambiarEstado(ST_DESARMADO); return; }
  fallos++;
  escribirLinea(1, "Clave incorrecta");
  Serial.println("CLAVE_INCORRECTA");
  delay(1200);
  if (fallos >= 3 && estado != ST_ALERTA) { anguloAlerta = -1; cambiarEstado(ST_ALERTA); }
  else pantallaEstado();
}

void esperar(unsigned long ms) {  // espera sin dejar de leer el boton
  unsigned long t = millis();
  while (millis() - t < ms) { procesarBoton(); actualizarSalidas(); }
}

// ---------- SENSORES Y SERVO ----------
int leerDistancia() {  // mediana de 3 lecturas, en cm
  int l[3];
  for (byte j = 0; j < 3; j++) {
    digitalWrite(TRIG, LOW); delayMicroseconds(2);
    digitalWrite(TRIG, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG, LOW);
    unsigned long t = pulseIn(ECHO, HIGH, 25000UL);
    int d = (t == 0) ? DIST_MAX : (int)(t * 0.0343 / 2);
    l[j] = min(d, DIST_MAX);
    procesarBoton();
    delay(10);
  }
  return max(min(l[0], l[1]), min(max(l[0], l[1]), l[2]));
}

void moverServo(int i) {
  servo.write(ANG_MIN + i * PASO);
  esperar(250);
}

void calibrar() {  // aprende como se ve la zona vacia
  int validas = 0;
  for (int i = 0; i < N_POS; i++) {
    moverServo(i);
    if (estado != ST_CALIBRANDO) return;
    base[i] = leerDistancia();
    if (estado != ST_CALIBRANDO) return;
    if (base[i] < DIST_MAX) validas++;
    char buf[17];
    snprintf(buf, 17, "Ang:%3d D:%3dcm", ANG_MIN + i * PASO, base[i]);
    escribirLinea(1, buf);
  }
  if (validas == 0) cambiarEstado(ST_ERROR);  // ningun eco en todo el barrido: sensor fallando
  else cambiarEstado(ST_VIGILANDO);
}

void vigilar() {  // un barrido completo, ida y vuelta
  for (int k = 0; k < 2 * N_POS - 2; k++) {
    if (estado != ST_VIGILANDO) return;
    int i = (k < N_POS) ? k : (2 * N_POS - 2 - k);
    moverServo(i);
    if (estado != ST_VIGILANDO) return;
    int ang = ANG_MIN + i * PASO;
    int d = leerDistancia();
    if (estado != ST_VIGILANDO) return;
    Serial.print("LECTURA,"); Serial.print(ang); Serial.print(",");
    Serial.print(d); Serial.print(","); Serial.println(digitalRead(PIR));
    if (nIntento == 0) {
      char buf[17];
      snprintf(buf, 17, "Ang:%3d D:%3dcm", ang, d);
      escribirLinea(1, buf);
    }
    if (d < base[i] - UMBRAL_CM) {  // algo mas cerca de lo normal
      int d2 = leerDistancia();     // se confirma con otra medicion
      if (d2 < base[i] - UMBRAL_CM && digitalRead(PIR) == HIGH) {  // y el PIR ve movimiento
        anguloAlerta = ang;
        distAlerta = d2;
        cambiarEstado(ST_ALERTA);
        return;
      }
    }
  }
}

// ---------- PROGRAMA PRINCIPAL ----------
void setup() {
  Serial.begin(9600);
  pinMode(LED_R, OUTPUT); pinMode(LED_G, OUTPUT); pinMode(LED_B, OUTPUT);
  pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT); pinMode(PIR, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BOTON, INPUT_PULLUP);
  digitalWrite(RELE, RELE_OFF);  // apagado antes de configurar el pin
  pinMode(RELE, OUTPUT);
  lcd.begin(16, 2);
  servo.attach(PIN_SERVO);
  servo.write(90);
  color(0, 0, 1);
  for (int s = PIR_CALENT_S; s > 0; s--) {
    char buf[17];
    snprintf(buf, 17, "Espera: %2d s", s);
    escribirLinea(0, "Calentando PIR");
    escribirLinea(1, buf);
    delay(1000);
  }
  cambiarEstado(ST_DESARMADO);
}

void loop() {
  procesarBoton();
  actualizarSalidas();
  if (estado == ST_CALIBRANDO) calibrar();
  else if (estado == ST_VIGILANDO) vigilar();
}
