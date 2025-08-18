# Servo Power Control Troubleshooting

## Hardware Debug Checkliste

### 1. GPIO21 Ausgangssignal prüfen
```bash
# Nach dem Build/Flash überprüfen Sie die Log-Ausgabe:
# "Servo power enable: GPIO21 set to 1"
# "Servo power disable: GPIO21 set to 0"
```

**Mit Multimeter messen:**
- GPIO21 Pin am ESP32-C6: 0V (disabled) → 3.3V (enabled)

### 2. 2N2222 Transistor Verbindungen
```
ESP32-C6 GPIO21 → [1kΩ Widerstand] → 2N2222 Basis (mittlerer Pin)
5V Supply → 2N2222 Kollektor (rechter Pin)  
GND → 2N2222 Emitter (linker Pin)
```

**Spannungen messen:**
- **Basis**: ~2.6V wenn GPIO21 HIGH (3.3V - 0.7V)
- **Kollektor**: 0V (OFF) → 5V (ON) 
- **Emitter**: Immer 0V (GND)

### 3. Häufige Fehlerquellen

#### 3.1 2N2222 Pinbelegung falsch
```
Korrekte TO-92 Pinbelegung (flache Seite nach vorne):
E  B  C
|  |  |
1  2  3
```

#### 3.2 Basis-Widerstand fehlt/falsch
- **Ohne Widerstand**: ESP32 GPIO kann beschädigt werden
- **Zu hoch (>10kΩ)**: Transistor schaltet nicht vollständig
- **Empfohlen**: 1kΩ

#### 3.3 Stromversorgung
- **5V Supply**: Muss mindestens 2A liefern können
- **Servo Power**: Gemessen am Servo +5V Pin

#### 3.4 Servo-Verkabelung
```
Servo 1 & 2:
- Rot → 2N2222 Kollektor (geschaltete +5V)
- Braun/Schwarz → GND
- Orange/Gelb → GPIO18/19 (direkt ESP32)
```

### 4. Debug Schritte

#### Schritt 1: GPIO Test
```c
// Manueller GPIO Toggle Test - fügen Sie in main() hinzu:
while(1) {
    gpio_set_level(SERVO_POWER_CONTROL_GPIO, 1);
    ESP_LOGI("TEST", "GPIO21 HIGH");
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    gpio_set_level(SERVO_POWER_CONTROL_GPIO, 0); 
    ESP_LOGI("TEST", "GPIO21 LOW");
    vTaskDelay(pdMS_TO_TICKS(2000));
}
```

#### Schritt 2: Transistor ohne Servo testen
- 5V Supply anschließen
- Multimeter an Kollektor messen
- GPIO21 HIGH → Kollektor sollte ~5V zeigen

#### Schritt 3: Servo direkt an 5V testen
- Servo +5V direkt an Power Supply
- Servo sollte sich beim PWM-Signal bewegen

### 5. Typische Messwerte

| Zustand | GPIO21 | Basis | Kollektor | Servo Status |
|---------|--------|-------|-----------|--------------|
| Power OFF | 0V | 0V | 5V (Float) | Kein Strom |
| Power ON | 3.3V | ~2.6V | ~0.2V | 5V zu Servos |

**Hinweis**: Bei korrektem Schalten sollte der Kollektor LOW sein (Sättigung)!

### 6. Häufige Lösungen

#### Problem: GPIO wird geschaltet, aber Transistor schaltet nicht
- **Lösung**: Basis-Widerstand prüfen, 2N2222 Pinbelegung kontrollieren

#### Problem: Transistor schaltet, aber Servos bewegen sich nicht  
- **Lösung**: Servo-Kabel prüfen, PWM-Signale an GPIO18/19 messen

#### Problem: Servos zucken nur kurz
- **Lösung**: 5V Supply Stromkapazität erhöhen (≥2A)

---