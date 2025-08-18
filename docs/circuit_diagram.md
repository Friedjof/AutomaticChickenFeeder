# Circuit Diagram - Automated Chicken Feeder

Hardware schematic for the ESP32-C6 based chicken feeding system with servo control and manual button input.

---

## 1. Complete System Overview

```mermaid
graph TD
    subgraph ESP32C6["Seeed XIAO ESP32-C6"]
        GPIO1["GPIO1 (D1)<br/>(Pull-up internal)"]
        GPIO16["GPIO16 (D10)"]
        GPIO17["GPIO17 (D8)"]
        GPIO20["GPIO20 (D3)"]
        VCC3V3["3.3V"]
        GND_ESP["GND"]
    end
    
    subgraph Power["Power Supply"]
        VCC5V["5V Supply"]
        GND_PWR["GND"]
    end
    
    subgraph Button["Manual Feed Button"]
        BTN["Taster (NO)"]
    end
    
    subgraph ServoControl["Servo Power Control"]
        R1["1kΩ"]
        Q1["S8550 PNP NPN"]
    end
    
    subgraph Servos["Servo Motors"]
        SERVO1["Servo 1 (Rotation)"]
        SERVO2["Servo 2 (Tilt)"]
    end
    
    %% Power flow through transistor control
    VCC5V --> Q1
    Q1 --> SERVO1
    Q1 --> SERVO2
    GND_PWR --> Q1
    GND_PWR --> SERVO1
    GND_PWR --> SERVO2
    GND_PWR --> GND_ESP
    
    %% Button circuit (simplified - pull-up is internal)
    GPIO1 --> BTN
    BTN --> GND_ESP
    
    %% Servo power control
    GPIO20 --> R1
    R1 --> Q1
    
    %% Servo signal connections
    GPIO16 --> SERVO1
    GPIO17 --> SERVO2
    
    classDef vcc fill:#cc4444
    classDef gnd fill:#444444
    classDef signal fill:#005555
    classDef control fill:#332200
    
    class VCC5V,VCC3V3 vcc
    class GND_ESP,GND_PWR gnd
    class GPIO1,GPIO16,GPIO17,GPIO20 signal
    class Q1,R1,BTN control
```

---

## 2. Manual Feed Button Circuit

```mermaid
graph LR
    subgraph ButtonCircuit["Button Input Circuit"]
        VCC3V3["3.3V"] 
        PULLUP["10kΩ Pull-up<br/>(ESP32 internal)"]
        GPIO1["GPIO1<br/>(D1)"]
        BUTTON["Manual Feed<br/>Button"]
        GND["GND"]
    end
    
    VCC3V3 -.-> PULLUP
    PULLUP --> GPIO1
    GPIO1 --> BUTTON
    BUTTON --> GND
    
    classDef vcc fill:#cc4444
    classDef gnd fill:#444444
    classDef signal fill:#005555
    classDef input fill:#774400
    
    class VCC3V3 vcc
    class GND gnd
    class GPIO1 signal
    class BUTTON input
```

**Button Logic:**
- **Not Pressed**: GPIO1 = HIGH (3.3V via pull-up)
- **Pressed**: GPIO1 = LOW (0V via GND connection)
- **Interrupt**: Falling edge detection (HIGH → LOW)

---

## 3. Servo Power Control Circuit

```mermaid
graph TB
    subgraph PowerControl["Servo Power Switching"]
        GPIO20["GPIO20 (D3)"]
        R1["1kΩ Base<br/>Resistor"]
        Q1["S8550 PNP NPN<br/>Transistor"]
        VCC5V["5V Supply"]
        GND["GND"]
    end
    
    subgraph ServoLoad["Servo Motors Load"]
        SERVO1_PWR["Servo 1<br/>Power"]
        SERVO2_PWR["Servo 2<br/>Power"]
    end
    
    GPIO20 --> R1
    R1 --> Q1
    VCC5V --> Q1
    Q1 --> SERVO1_PWR
    Q1 --> SERVO2_PWR
    Q1 --> GND
    SERVO1_PWR --> GND
    SERVO2_PWR --> GND
    
    classDef control fill:#4d6b3d
    classDef vcc fill:#cc4444
    classDef gnd fill:#444444
    classDef load fill:#aa6633
    
    class GPIO20,R1,Q1 control
    class VCC5V vcc
    class GND gnd
    class SERVO1_PWR,SERVO2_PWR load
```

**Power Control Logic:**
- **GPIO20 HIGH**: Transistor OFF → Servos unpowered (PNP logic)
- **GPIO20 LOW**: Transistor ON → Servos powered (PNP logic)

---

## 4. Pin Assignment Table

| Function | ESP32-C6 GPIO | XIAO Label | Component | Notes |
|----------|---------------|------------|-----------|-------|
| Manual Button | GPIO1 | D1 | Taster → GND | Pull-up enabled, interrupt-capable |
| Servo 1 Signal | GPIO16 | D6 (TX0) | PWM Signal | Scoop rotation control |
| Servo 2 Signal | GPIO17 | D7 (RX0) | PWM Signal | Scoop tilt control |
| Servo Power | GPIO20 | D9 (MISO) | S8550 Base | Power switching via PNP transistor |

---

## 5. Component List

### Active Components
- **1x** Seeed XIAO ESP32-C6 Development Board
- **1x** S8550 PNP NPN Transistor (TO-92 package)
- **2x** MG90S Servo Motors (or compatible 180° servos)

### Passive Components  
- **1x** 1kΩ Resistor (Base resistor for S8550 PNP)
- **1x** Push Button (Normally Open)

### Power Supply
- **1x** 5V Power Supply (≥2A for servo operation)

---

## 6. Physical Connections

### S8550 PNP Transistor Pinout (TO-92)
```
Looking at flat side:
    E  B  C
    |  |  |
    1  2  3

E = Emitter → +5V Supply
B = Base → GPIO20 via 1kΩ 
C = Collector → Servo +5V Power
```

### Servo Connections
```
Servo 1 (Rotation):
- Red → S8550 Collector (switched +5V)
- Brown/Black → GND  
- Orange/Yellow → GPIO16

Servo 2 (Tilt):
- Red → S8550 Collector (switched +5V)
- Brown/Black → GND
- Orange/Yellow → GPIO17
```

---

## 7. Safety Notes

- **Power Supply**: Use regulated 5V supply with adequate current capacity (≥2A)
- **Transistor Heat**: S8550 PNP may require small heatsink under continuous operation
- **Servo Stall**: Implement timeout protection in software to prevent servo damage
- **ESD Protection**: Handle ESP32-C6 with appropriate ESD precautions

---

## 8. Testing Points

- **Button Test**: Monitor GPIO1 state changes via serial output
- **Power Control**: Measure 5V at servo inputs when GPIO20 is LOW (PNP)
- **Servo Signals**: Verify PWM signals on GPIO16/17 using oscilloscope
- **Current Draw**: Monitor total system current consumption

---