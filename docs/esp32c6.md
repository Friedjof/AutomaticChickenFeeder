# Seeed XIAO ESP32‑C6 Board Reference

This document describes the key specifications, pinout, and usage guidelines for the **Seeed XIAO ESP32‑C6** board used in Automated Chicken 2.0.

![Seeed XIAO ESP32-C6 Pinout](https://www.electroniclinic.com/wp-content/uploads/2024/04/XIAO-ESP32C6-FORNT-1536x864.jpg)

---

## 1. Overview & Key Specs

- **MCU:** ESP32‑C6 (RISC‑V single-core up to 160 MHz + low‑power core at 20 MHz)  
- **Memory:** 512 KiB SRAM, 320 KiB ROM, external flash up to several MB 
- **Wireless:**  
  - Wi‑Fi 6 (802.11ax), Bluetooth 5.3 LE  
  - 802.15.4 for Zigbee / Thread / Matter
- **Security:** Secure boot, flash encryption, TEE support
- **Power:** Deep‑sleep current ~15 µA using ULP core
- **Form Factor:** 21 × 17.5 mm board without pin headers by default (headers to be soldered manually)

---

## 2. Pinout Diagram

See image above for front and back pin layout of the XIAO ESP32‑C6, showing GPIO assignments, power pins, and boot/flash pins.

- 14 castellated pads (7 per side) support digital I/O, ADC, UART, I²C, SPI, power, and boot functions.
- Seven **low-power ULP-accessible pins**: LP_GPIO0..LP_GPIO7, mapped to standard GPIO0–GPIO7.

---

## 3. GPIO & Functionality Summary

| GPIO       | Available Functions                | Notes                                    |
|------------|------------------------------------|------------------------------------------|
| GPIO0–GPIO7| Digital I/O, ADC, interrupts       | Also ULP core accessible                 |
| GPIO18–GPIO23 | SPI / SDIO (used for flash)     | Avoid using for peripherals              |
| GPIO16,17,19,20,21,22,23 | PWM, UART, I²C, etc. | Other digital and analog operations      |
| 5V, 3V3, GND, VBAT | Power Supply pins           | USB-C provides 5V; board regulates to 3.3 V |

- Boot strapping pins such as GPIO8, GPIO9, GPIO15, MTMS, MTDI are pre-configured—avoid modifying assignments.

---

## 4. Recommended Pin Usage

- **Servo control:** Use PWM-capable GPIO pins ≥ GPIO16 (e.g. GPIO18 or 17).
- **Buttons:** Use interrupt-capable pins like GPIO0–GPIO7 or D1 (GPIO1).
- **VL53L0X sensor (I²C):** Use any I²C-capable GPIO pair (e.g. GPIO6/7 for SDA/SCL).
- **Vibration motor:** Use a free digital GPIO with PWM support for control.

---

## 5. Power & Boot Behavior

- **USB-C connector** supports both 5 V power and USB-to-UART interface for flashing/debugging.
- **Boot mode:**  
  - Press and hold BOOT button while connecting USB to enter bootloader mode.
  - Reset button reboots to application firmware.

---

## 6. Pin Mapping Table

| XIAO Label | PAxx Legacy | ESP32‑C6 GPIO | Typical Function           |
|------------|-------------|----------------|----------------------------|
| **D0 / A0**| PA02        | GPIO0          | Digital I/O, ADC1_CH0      |
| **D1 / A1**| PA04        | GPIO1          | Digital I/O, ADC1_CH1      |
| **D2 / A2**| PA10        | GPIO2          | Digital I/O, ADC1_CH2      |
| **D3**     | PA11        | GPIO21         | Digital I/O                |
| **D4 (SDA)**| PA08       | GPIO22         | I²C SDA (default), SDIO_D2 |
| **D5 (SCL)**| PA09       | GPIO23         | I²C SCL (default), SDIO_D3 |
| **D6 (TX0)**| PB08       | GPIO16         | UART0 TX, Digital I/O      |
| **D7 (RX0)**| PB09       | GPIO17         | UART0 RX, Digital I/O      |
| **D8 (SCK)**| PA07       | GPIO19         | SPI CLK (SDIO_CLK), I/O    |
| **D9 (MISO)**| PA05      | GPIO20         | SPI MISO (SDIO_DATA0) I/O  |
| **D10 (MOSI)**| PA06    | GPIO18         | SPI MOSI (SDIO_CMD) I/O    |

---

## 7. Notes on Pin Use

- The board has **14 pads**, but only **11 user GPIO usable pins plus power lines**.
- **Avoid using** strapping or flash-related GPIOs: GPIO4, 5, 8, 9, 12, 13, 14, 15, and GPIO24‑30.
- Strapping pins (GPIO4, 5, 8, 9, 15) determine boot mode—do not rely on them for general I/O.

---

## 8. Recommended Peripheral Allocations

- **Servo signals (PWM):** Use any PWM-capable GPIO like GPIO16–GPIO23.
- **Buttons (interrupt-capable):** GPIO0–GPIO7 are interrupt-capable and accessible by the low‑power ULP core.
- **VL53L0X sensor (I²C):** Best on `D4=SDA(GPIO22)` and `D5=SCL(GPIO23)`.
- **UART console:** Connect to `D6=TX(GPIO16)` and `D7=RX(GPIO17)` for serial interface.

---
