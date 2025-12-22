# Embedded Systems Project - Smart Lock System

A comprehensive embedded systems project implementing a smart door locking mechanism using dual microcontroller architecture (TM4C123GH6PM). This system demonstrates real-time embedded systems concepts including inter-processor communication, password authentication, EEPROM storage, and hardware interface control.

## 📋 Project Overview

This project consists of two main subsystems:

1. **Control Unit (Control/)** - Manages door lock mechanism
2. **Human-Machine Interface Unit (HMI/)** - User interaction and authentication

Both units communicate via UART to implement a complete smart lock system with password protection, automatic timeout features, and secure door control.

---

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Smart Lock System                        │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌────────────────────┐       UART       ┌────────────────┐ │
│  │   HMI Unit         │◄────────────────►│ Control Unit   │ │
│  │  (User Interface)  │                  │  (Lock Logic)  │ │
│  │                    │                  │                │ │
│  │ - LCD Display      │                  │ - Servo Motor  │ │
│  │ - Keypad Input     │                  │ - Buzzer       │ │
│  │ - ADC (sensors)    │                  │ - EEPROM       │ │
│  │                    │                  │ - System Timer │ │
│  └────────────────────┘                  └────────────────┘ │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## 📁 Project Structure

```
adeem/
├── Control/
│   ├── main.c                 # Control unit main program
│   ├── buzzer.c/h            # Buzzer driver
│   ├── Servo.c/h             # Servo motor control
│   ├── uart.c/h              # UART communication driver
│   ├── dio.c/h               # Digital I/O control
│   ├── eeprom.c/h            # EEPROM storage management
│   ├── systick.c/h           # System tick timer
│   ├── startup_ewarm.c       # ARM startup code
│   ├── tm4c123gh6pm.h        # Microcontroller definitions
│   └── Debug/                # Compilation output
│
├── HMI/
│   ├── main.c                 # HMI unit main program
│   ├── lcd.c/h               # 16x2 LCD display driver
│   ├── keypad.c/h            # 4x4 Keypad input driver
│   ├── uart.c/h              # UART communication driver
│   ├── adc.c/h               # Analog-to-Digital converter
│   ├── dio.c/h               # Digital I/O control
│   ├── systick.c/h           # System tick timer
│   ├── startup_ewarm.c       # ARM startup code
│   ├── tm4c123gh6pm.h        # Microcontroller definitions
│   └── Debug/                # Compilation output
│
├── Yarab.eww                  # Workspace file (IAR Embedded Workbench)
└── README.md                  # This file
```

---

## 🔧 Hardware Components

### Control Unit (Door Lock Controller)
- **Microcontroller:** TM4C123GH6PM (ARM Cortex-M4)
- **Servo Motor:** Controls door lock mechanism (0° = Locked, 90° = Unlocked)
- **Buzzer:** Audio feedback (Port E, Pin 4)
- **EEPROM:** Password and configuration storage
- **UART:** Communication with HMI unit

### HMI Unit (User Interface)
- **Microcontroller:** TM4C123GH6PM (ARM Cortex-M4)
- **LCD Display:** 16x2 character display for status feedback
- **Keypad:** 4x4 matrix keypad for password input
- **ADC:** Analog sensors for environmental monitoring
- **UART:** Communication with Control unit

---

## 🔐 Key Features

### Authentication & Security
- **Password Protection:** Master password set during initialization
- **EEPROM Storage:** Passwords stored in EEPROM for persistence
- **Timeout Feature:** Automatic re-lock after configurable timeout period
- **Input Validation:** Secure password comparison with protection against timing attacks

### User Interface
- **LCD Feedback:** Real-time status messages and prompts
- **Keypad Input:** Numeric and function button input (4x4 matrix)
- **Visual Feedback:** Buzzer alerts for invalid attempts
- **Display Modes:** Password entry, status display, error messages

### System Control
- **Servo Control:** Smooth door lock/unlock mechanism
- **Auto-Lock:** Automatic timeout-based re-locking
- **Inter-ECU Communication:** UART-based secure messaging protocol
- **Real-time Response:** Immediate feedback on authentication

---

## 🔄 Communication Protocol

### UART Configuration
- **Baud Rate:** 115200 bps (typical)
- **Data Bits:** 8
- **Stop Bits:** 1
- **Parity:** None

### Message Format
Commands sent between units follow this pattern:

```
[COMMAND]:[DATA]\n
```

**Common Commands:**
- `SETPWD:password` - Set master password
- `VERIFY:password` - Verify entered password
- `UNLOCK` - Command to unlock door
- `LOCK` - Command to lock door
- `TIMEOUT:seconds` - Set auto-lock timeout
- `ACK` - Acknowledgment
- `NACK` - Negative acknowledgment
- `CONTROL_READY` - Control unit initialization complete
- `HMI_READY` - HMI unit initialization complete

---

## 📊 Module Descriptions

### Control Unit Modules

#### **main.c**
- Main control loop and system initialization
- Password authentication logic
- Door lock state management
- EEPROM read/write operations
- Auto-timeout mechanism

#### **Servo.c/h**
- PWM-based servo motor control
- Lock position management (0° = Closed, 90° = Open)
- Smooth servo movement

#### **buzzer.c/h**
- Buzzer driver for audio feedback
- Configurable beep duration
- System alerts and notifications

#### **eeprom.c/h**
- EEPROM read/write abstraction
- Block and offset-based access
- Password persistence

#### **uart.c/h**
- UART2 initialization and configuration
- Character transmission and reception
- String operations with timeout handling

#### **dio.c/h**
- GPIO initialization and control
- Port and pin management
- LED control (if used)

#### **systick.c/h**
- System tick timer configuration
- Delay functions (milliseconds/microseconds)
- Interrupt-based timing

### HMI Unit Modules

#### **main.c**
- HMI initialization and main loop
- Password input handling
- Screen navigation logic
- State machine implementation

#### **lcd.c/h**
- 4-bit mode LCD control
- Character and string display
- Cursor positioning
- Display clear operations

#### **keypad.c/h**
- 4x4 Keypad matrix scanning
- Button debouncing
- Key press detection and encoding

#### **adc.c/h**
- Analog-to-Digital conversion
- Sensor input sampling
- Value acquisition and filtering

#### **uart.c/h**
- UART2 communication driver
- Synchronized send/receive with timeout
- Response handling

---

## 🚀 Getting Started

### Prerequisites
- **IDE:** IAR Embedded Workbench (EWARM)
- **Compiler:** ARM C/C++ Compiler
- **Hardware:** TM4C123GH6PM LaunchPad (x2)
- **Programmer:** JTAG/SWD debugger

### Building the Project

1. **Open IAR Embedded Workbench**
   ```
   File → Open → Workspace → Yarab.eww
   ```

2. **Build Control Unit**
   - Select "Control" project
   - Project → Rebuild All (Ctrl+Shift+F7)

3. **Build HMI Unit**
   - Select "HMI" project
   - Project → Rebuild All (Ctrl+Shift+F7)

4. **Verify Build**
   - Check that both projects compile without errors
   - Output files located in `Control/Debug/Exe/` and `HMI/Debug/Exe/`

### Programming the Microcontrollers

1. **Program Control Unit**
   - Connect first TM4C123GH6PM to debugger
   - Select Control project
   - Project → Download and Debug (Ctrl+D)

2. **Program HMI Unit**
   - Connect second TM4C123GH6PM to debugger
   - Select HMI project
   - Project → Download and Debug (Ctrl+D)

3. **Connect UART Interface**
   - Connect UART TX from Control to UART RX of HMI
   - Connect UART RX from Control to UART TX of HMI
   - Connect GND between both units

---

## 💾 Configuration

### Password Settings
Edit `Control/main.c`:
```c
#define PASSWORD_MAX_LENGTH     20
#define EEPROM_PASSWORD_BLOCK   0
#define EEPROM_PASSWORD_OFFSET  0
```

### Timeout Configuration
```c
#define EEPROM_TIMEOUT_BLOCK    1
#define EEPROM_TIMEOUT_OFFSET   0
```
Default timeout: 5 seconds (configurable via HMI)

### UART Configuration
Edit `Control/uart.c` and `HMI/uart.c`:
- Default Baud Rate: 115200
- Data Bits: 8
- Stop Bits: 1
- Parity: None

---

## 🔍 Usage Instructions

### Initial Setup
1. Power on both microcontroller units
2. HMI displays "Welcome" message
3. Follow on-screen prompts to set master password
4. Control unit stores password in EEPROM

### Unlocking the Door
1. Press any key on keypad to activate HMI
2. Enter master password (digits followed by #)
3. Press * to clear/delete last character
4. Upon successful authentication, servo unlocks door
5. LCD displays "UNLOCKED" status
6. Door auto-locks after timeout period

### Error Handling
- **Invalid Password:** Buzzer beeps 3 times, LCD shows error
- **UART Communication Timeout:** System attempts retry
- **EEPROM Error:** System defaults to failsafe (locked)

---

## 📈 Performance Specifications

| Parameter | Value |
|-----------|-------|
| Password Length | Up to 20 characters |
| Authentication Time | < 100ms |
| Servo Response Time | ~500ms |
| LCD Update Rate | 60Hz |
| Keypad Scan Rate | 100Hz |
| UART Baud Rate | 115200 bps |
| Default Timeout | 5 seconds |
| EEPROM Write Time | ~5ms per block |

--
## 🐛 Troubleshooting

### LCD Not Displaying
- Check LCD connection (Data pins PB0-PB3, Control pins PB5,PB6,PB7)
- Verify LCD initialization in `lcd.c`
- Check power supply voltage

### Keypad Not Responding
- Check keypad matrix connections (rows: PA0-PA3, columns: PA4-PA7)
- Verify keypad scanning in `keypad.c`
- Test individual button connections

### Servo Not Moving
- Check servo PWM pin (PB5) connection
- Verify servo power supply (5V recommended)
- Check PWM frequency and duty cycle settings in `Servo.c`

### UART Communication Issues
- Verify TX/RX connections between units
- Check baud rate configuration (115200)
- Test with serial monitor at same baud rate
- Ensure GND is connected between both units

### EEPROM Data Not Persisting
- Verify EEPROM I2C connections
- Check EEPROM address configuration
- Test EEPROM read/write operations in debug mode

---

## 📝 Development Notes

### Code Organization
- Each module is self-contained with clear header files
- Configuration constants defined in header files
- HAL (Hardware Abstraction Layer) for easy porting
- Comments explain critical sections

### Debugging Tips
- Use serial printf redirection for debug output
- Enable IAR breakpoints on critical functions
- Monitor watch variables during authentication
- Use Logic Analyzer for UART protocol verification

### Extension Ideas
- Add fingerprint sensor authentication
- Implement multi-user password management
- Add temperature/humidity sensors
- Implement wireless (WiFi/Bluetooth) unlock
- Add access log storage in EEPROM
- Implement 2-factor authentication

---

## 👥 Team & Contributors

**Project:** Introduction to Embedded Systems (7th Semester)  
**Institution:** University  
**Date:** December 2025

---

## 📄 License

This project is for educational purposes in the embedded systems course.

---

## 📞 Support & References

### IAR Embedded Workbench Documentation
- Built-in Help (F1 key)
- Project Configuration Guide
- C Language Reference

### TM4C123GH6PM Documentation
- Device Datasheet
- TivaWare Peripheral Driver Library
- LaunchPad User Guide

### Useful Resources
- ARM Cortex-M4 Architecture Reference
- UART Protocol Overview
- PWM Control Principles
- EEPROM Storage Best Practices

---

## 🔗 Related Files

- `Yarab.eww` - IAR Workspace file (main entry point)
- `.dep`, `.ewd`, `.ewp`, `.ewt` - Project configuration files
- `Debug/` directories - Compilation artifacts and executables

---

**Last Updated:** December 20, 2025  
**Version:** 1.0
