[README.md](https://github.com/user-attachments/files/35446218/README.md)
# Centinela

Alarma de intrusos con barrido ultrasónico sobre **Arduino Uno**. Proyecto integrador de **Sistemas Programables (SCC1023)**, grupo S7A, Instituto Tecnológico de Hermosillo, agosto–diciembre 2026.

Un sensor ultrasónico montado sobre un servo barre 11 ángulos (15° a 165°), aprende cómo se ve la zona vacía y, si algo aparece al menos 30 cm más cerca de lo normal **y** el sensor PIR detecta movimiento, dispara una sirena de 12 V por un relevador. Se arma con un botón y se desarma con una clave de pulsaciones cortas y largas.

## Equipo

- Silva Alcantar Kenay De Jesús
- Núñez García José Miguel
- Cuesta Durán Adrián
- Dante Olivarria

## Estructura del repositorio

```
Centinela/
├── centinela.ino          Programa del Arduino
└── README.md
```

## Estados

`DESARMADO` → `CALIBRANDO` → `VIGILANDO` → `ALERTA` · `ERROR` si el ultrasónico no devuelve eco al calibrar.

| LED | Estado |
| --- | --- |
| Verde | DESARMADO |
| Azul | Calentando PIR, CALIBRANDO o VIGILANDO |
| Rojo parpadeando | ALERTA o ERROR |

## Materiales

| Componente | Cant. |
| --- | --- |
| Arduino Uno compatible (CH340) y cable USB A-B | 1 |
| Sensor ultrasónico (Steren ARD-350) | 1 |
| Sensor PIR HC-SR501 | 1 |
| Micro servomotor (Steren MOT-100) | 1 |
| Módulo de 4 relevadores (canal 1) | 1 |
| Mini sirena 12 V | 1 |
| LCD 16x2 HD44780 sin I2C + potenciómetro 10 kΩ | 1 |
| LED RGB de ánodo común (Steren LED-5/RGB) | 1 |
| Buzzer activo (Steren BGD08) | 1 |
| Micro switch de 4 terminales | 1 |
| Resistencias de 220 Ω | 4 |
| Regulador 7805 con disipador + capacitor 470 µF / 25 V | 1 |
| Eliminador 12 V / 2 A con adaptador jack | 1 |
| Protoboard de 400 puntos, cables Dupont y cable 22 AWG | — |

Costo aproximado: **$929.41 MXN** ($444.41 comprobados + $485.00 estimados).

## Conexiones

| Pin | Conectado a |
| --- | --- |
| D2 | Servo (señal) |
| D3 / D5 / D6 | LED RGB rojo / verde / azul (220 Ω cada uno) |
| D4 | PIR OUT |
| D7 / D8 | HC-SR04 TRIG / ECHO |
| D9 / D10 | LCD RS / E |
| D11 / D12 / D13 / A3 | LCD D4 / D5 / D6 / D7 |
| A0 | Relevador IN1 (activo en LOW) |
| A1 | Buzzer |
| A2 | Botón (INPUT_PULLUP) |

Alimentación: la Arduino va por USB (riel 5VARD: sensores, LCD y LED). Un 7805 baja los 12 V a 5 V (riel 5V_REG: servo y relevador). Los 12 V solo llegan al 7805 y al COM del relevador. Todas las tierras se unen; los dos rieles de 5 V nunca.

## Compilar y cargar

1. Instala el [Arduino IDE 2](https://www.arduino.cc/en/software) y, si tu placa es clon, el driver CH340.
2. Abre `centinela.ino`. Usa las librerías `LiquidCrystal` y `Servo`, que ya vienen con el IDE.
3. Selecciona **Arduino Uno** y el puerto COM de la placa.
4. Con la fuente de 12 V desconectada, pulsa **Upload**.
5. Abre el Monitor Serie a **9600 baudios**.
6. Conecta la fuente de 12 V: el servo se centra en 90°.

## Uso

- **Armar:** pulsación corta en DESARMADO. Sal de la zona mientras calibra.
- **Desarmar:** clave **corta, corta, larga, corta** (larga = 0.6 s o más; menos de 4 s entre pulsaciones).
- Tres claves incorrectas seguidas disparan la alarma.

## Datos por el puerto serie

| Línea | Significado |
| --- | --- |
| `ESTADO,VIGILANDO` | Cambio de estado |
| `LECTURA,90,142,0` | Ángulo, distancia en cm y PIR (1 = movimiento) |
| `CLAVE_INCORRECTA` | Se tecleó una clave equivocada |


## Parámetros

| Constante | Valor | Qué hace |
| --- | --- | --- |
| `ANODO_COMUN` | `true` | Tipo de LED RGB |
| `RELE_ON` / `RELE_OFF` | `LOW` / `HIGH` | Nivel que activa el relevador |
| `CLAVE` | `{0, 0, 1, 0}` | 0 = corta, 1 = larga |
| `ANG_MIN` / `PASO` / `N_POS` | 15 / 15 / 11 | Barrido de 15° a 165° |
| `DIST_MAX` | 250 cm | Distancia máxima considerada |
| `UMBRAL_CM` | 30 cm | Cuánto más cerca cuenta como intruso |
| `T_SIRENA_MS` | 8000 ms | Duración de la sirena |
| `T_LARGA` / `T_CLAVE` | 600 / 4000 ms | Pulsación larga / tiempo entre pulsaciones |
| `PIR_CALENT_S` | 30 s | Calentamiento del PIR |

## Créditos

Librerías `LiquidCrystal` y `Servo` de Arduino.
