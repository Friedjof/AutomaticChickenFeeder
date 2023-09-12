# Hühner-Futterautomat

## Übersicht

Der Hühner-Futterautomat ist ein Projekt, das entwickelt wurde, um die Fütterung von Hühnern in landwirtschaftlichen Betrieben zu automatisieren. Dieses System wurde entwickelt, um die Effizienz in der Hühnerhaltung zu steigern, die Fütterung zu optimieren und den Aufwand für die tägliche Pflege der Tiere zu reduzieren.

![Futterautomat](images/screenshot_desktop.png)

## Funktionen

- Automatisierte Fütterung zu festgelegten Zeiten
- Konfigurierbare Fütterungszeiten und -mengen über eine webbasierte Benutzeroberfläche
- Sicherung der Fütterungsdaten im permanenten Speicher des Mikrocontrollers
- Erstellt ein gesichertes WLAN für die Konfiguration und Überwachung des Systems
- Kann sich in einen Energiesparmodus versetzen, um die Batterielebensdauer zu verlängern

## Hardware

- ESP32-Mikrocontroller
- Motorsteuerungsmodul (z.B., Servomotor)
- Smartphones, Tablets oder Computer mit einem Webbrowser und WLAN
- Real-Time-Clock-Modul DS3231 für die genaue Zeitmessung
- Stromversorgung (Batterie oder Netzteil)

## Installation und Konfiguration

1. Klonen Sie das Repository
2. Installieren Sie die Abhängigkeiten (VSCode-Erweiterung PlatformIO IDE und PlatformIO Core)
3. Konfigurieren Sie die `platformio.ini`-Datei, um die richtige Platine und den richtigen Port auszuwählen oder starte eine Nix-Shell mit `nix-shell`.
4. Mit `make help` können Sie sich die verfügbaren Befehle anzeigen lassen.

## Hilfreiche Links
* [PlatformIO und der ESP32](https://docs.platformio.org/en/latest/platforms/espressif32.html)
* [ESP32 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
* [DS3231 RTC](https://www.analog.com/media/en/technical-documentation/data-sheets/DS3231.pdf)
* [RTC Interrupt](https://github.com/IowaDave/RTC-DS3231-Arduino-Interrupt)
* [Akku Betrieb](https://randomnerdtutorials.com/power-esp32-esp8266-solar-panels-battery-level-monitoring/)
* [ESP32 deep sleep](https://randomnerdtutorials.com/esp32-deep-sleep-arduino-ide-wake-up-sources/)


## Beitrag und Mitwirkung

Ich begrüße Beiträge und Mitwirkungen an diesem Projekt. Wenn Sie Verbesserungen vornehmen möchten, Fehler beheben oder neue Funktionen hinzufügen möchten, erstellen Sie bitte einen Issue oder ein Pull Request.

## Autor

- [Friedjof Noweck](https://github.com/Friedjof)
