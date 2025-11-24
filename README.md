# Zephyr RTOS Microinverter Controller

**A multi-threaded firmware simulation of a solar microinverter control system, built on Zephyr RTOS.**

[Status: Active]
[Platform: STM32F103 (Blue Pill)]
[RTOS: Zephyr]

## 🚀 Overview
This project implements the control logic for a solar microinverter using **Zephyr RTOS** on an STM32F103 (Blue Pill). Instead of relying on external hardware, it utilizes a **Hardware-in-the-Loop (HIL) Mocking** strategy to simulate solar panel physics and grid conditions entirely in software.

It features a custom **UART Shell** that allows engineers to inject faults (Over-Voltage/Under-Voltage) manually via a serial terminal to verify safety logic.

## 🏗️ System Architecture

The application runs two primary threads managed by the Zephyr Kernel:

| Thread         | Priority | Frequency | Responsibility                                                      |
| :------------- | :------- | :-------- | :------------------------------------------------------------------ |
| **SolarPhysics** | High (7) | 10 Hz     | Simulates voltage ramp (Day/Night cycle) 0V ↔ 50V.                  |
| **Controller** | Normal (5)| 5 Hz      | Reads voltage, determines state (Generating/Fault), controls LED.   |
| **Shell/Log** | Low      | Event     | Handles user input and system logging via UART.                     |

## 🛠️ Hardware & Config
* **Target:** STM32F103C8T6 "Blue Pill" (Clone/Genuine)
* **Clock Source:** Internal 8MHz HSI (PLL -> 48MHz System Clock) to bypass faulty external crystals on clone boards.
* **Communication:** UART Console (9600 Baud) via Arduino Bridge.

## 💻 Technical Implementation

### 1. The Clone Fix (`app.overlay`)
To ensure reliability on non-genuine ST chips, the external crystal (HSE) is disabled via Device Tree, and the system is forced to run on the internal oscillator.

    /* Force Internal High-Speed Oscillator */
    &clk_hse { status = "disabled"; };
    &clk_hsi { status = "okay"; };

    /* Configure PLL to boost 4MHz (HSI/2) to 48MHz */
    &pll {
        clocks = <&clk_hsi>;
        mul = <12>; 
        status = "okay";
    };

    /* Slow down UART for SoftwareSerial Bridge compatibility */
    &usart1 {
        current-speed = <9600>;
        status = "okay";
    };

### 2. OS Configuration (`prj.conf`)
Optimized for constrained RAM and clean serial output.

    CONFIG_SHELL=y
    CONFIG_CBPRINTF_FP_SUPPORT=y
    CONFIG_MAIN_STACK_SIZE=4096

    # Disable ANSI colors to prevent garbage text in non-VT100 terminals (Arduino Serial Monitor)
    CONFIG_SHELL_VT100_COLORS=n
    CONFIG_LOG_BACKEND_SHOW_COLOR=n

## 🔌 Setup without USB-TTL Adapter (Arduino Uno Bridge)
Since a dedicated USB-to-UART adapter was unavailable, an Arduino Uno is programmed to act as a 9600 Baud SoftwareSerial bridge.

### Wiring
| STM32 Pin      | Arduino Pin | Function                      |
| :------------- | :---------- | :---------------------------- |
| **PA9 (TX)** | **D10** | Signal travels STM32 -> PC    |
| **PA10 (RX)** | **D11** | Signal travels PC -> STM32    |
| **GND** | **GND** | Common Ground                 |

### Arduino Bridge Firmware
Upload this sketch to the Arduino to enable passthrough mode:

    #include <SoftwareSerial.h>
    SoftwareSerial stmSerial(10, 11); // RX, TX

    void setup() {
      Serial.begin(9600);     // PC Connection
      stmSerial.begin(9600);  // STM32 Connection
    }

    void loop() {
      // Pass data bi-directionally
      if (stmSerial.available()) Serial.write(stmSerial.read());
      if (Serial.available()) stmSerial.write(Serial.read());
    }

## ⚙️ Build & Run

### 1. Build
    west build -p auto -b stm32_min_dev

### 2. Flash
    west flash --runner openocd
    
*Tip: If flashing fails on clone chips, hold the physical RESET button on the Blue Pill while running the command, then release it once OpenOCD starts.*

### 3. Manual Testing (Shell)
Open a Serial Monitor (9600 Baud, CRLF Line Endings).

**Test Case 1: Normal Operation**
    uart:~$ sensor set 35
    Manual Override: 35 V
    # Result: LED Solid ON

**Test Case 2: Fault Injection**
    uart:~$ sensor set 55
    Manual Override: 55 V
    # Result: LED Blinking Fast (Over-Voltage)

**Test Case 3: Return to Simulation**
    uart:~$ sensor auto
    Auto Simulation Enabled
    # Result: LED animates through day/night cycle

---
