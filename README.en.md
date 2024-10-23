### STM32 Smart Door Lock System

#### Project Overview

This project is a smart door lock system developed using the STM32F103 microcontroller, integrating multiple unlocking methods including password, RFID card, Bluetooth, and fingerprint recognition. The system features an OLED display for showing menus and system status. It has an access control mechanism with two modes: administrator mode and normal unlocking mode. The administrator mode requires an admin password and allows advanced operations like password modification, card management, and fingerprint enrollment and deletion.

#### Tech Stack and Hardware

**Microcontroller**: STM32F103

- **Peripherals**:
  - OLED display
  - RC522 RFID reader
  - AS608 fingerprint module
  - HC-05 Bluetooth module
  - 4x4 matrix keypad
- **Communication Protocols**:
  - OLED: I2C
  - Matrix keypad: UART1
  - RFID (RC522): SPI for register read/write
  - Fingerprint: UART2
  - Bluetooth: UART3
- **Development Tools**: Keil uVision

#### System Features

1. Multiple Unlock Methods:
   - **Password Unlock**: Two types of passwords—admin and normal. Admin password unlocks the system and grants access to admin mode, while the normal password only unlocks the door.
   - **RFID Unlock**: Supports up to 10 authorized RFID cards, read via the RC522 module.
   - **Bluetooth Unlock**: Unlock via communication between the Bluetooth module and a smartphone.
   - **Fingerprint Unlock**: Unlock via fingerprint recognition using the AS608 module.
2. Administrator Features:
   - Admins can manage the system through the menu, including adding/removing fingerprints, managing cards, and changing passwords.
3. OLED Menu Display:
   - The OLED screen displays operational menus and system status, covering both user and admin interfaces.
4. Card and Fingerprint Management:
   - The system supports storing and managing multiple RFID cards and fingerprint data, with options to enroll and delete entries.

#### Key Logic Implementation

1. **Main Menu**: After system initialization, the main menu is displayed, allowing the user to select the unlocking method.
2. **Password Unlock Module**: Enter "0" to access password unlock mode, then input the password via the matrix keypad. Admin password unlocks the system and enters admin mode, while the normal password just unlocks the door.
3. **RFID Unlock Module**: The RC522 module reads the card ID and compares it to the authorized list. If the card is authorized, the door unlocks.
4. **Bluetooth Unlock Module**: Receives unlock commands from a smartphone via the Bluetooth module and unlocks if matched.
5. **Fingerprint Unlock Module**: The AS608 module processes fingerprint images and performs matching. If the fingerprint is authorized, the door unlocks.

### Wiring

- **OLED Display**:
  - SCL --> PB12
  - SDA --> PB13
- **AS608 Fingerprint Module**:
  - TX --> PA3
  - RX --> PA2
  - WAK --> PA1
  - Vt --> 3.3V
- **HC-05 Bluetooth Module**:
  - TX --> PB11
  - RX --> PB10
- **RFID-RC522 Module**:
  - SDA --> PA4
  - SCK --> PA5
  - MOSI --> PA7
  - MISO --> PA6
  - IRQ not connected
  - RST --> PA11
- **4x4 Matrix Keypad**:
  - C4 --> PB9
  - C3 --> PB8
  - C2 --> PB7
  - C1 --> PB6
  - R4 --> PB5
  - R3 --> PB4
  - R2 --> PB1
  - R1 --> PB0

Connect the remaining VCC and GND to 3.3V and GND as required.