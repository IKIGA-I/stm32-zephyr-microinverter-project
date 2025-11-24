# Zephyr RTOS Microinverter Controller

**A multi-threaded firmware simulation of a solar microinverter control system, built on Zephyr RTOS.**

![Status](https://img.shields.io/badge/Status-Active-success)
![Platform](https://img.shields.io/badge/Platform-STM32-blue)
![RTOS](https://img.shields.io/badge/RTOS-Zephyr-purple)

## 🚀 Overview
This project implements the control logic for a solar microinverter using **Zephyr RTOS** on an STM32F103 (Blue Pill). Instead of relying on external hardware, it utilizes a **Hardware-in-the-Loop (HIL) Mocking** strategy to simulate solar panel physics and grid conditions entirely in software.

This architecture demonstrates how firmware logic can be developed and tested in parallel with hardware design—a critical workflow in professional embedded engineering.

### Key Features
* **Multi-Threaded Architecture:** Separates "Physics Simulation" (generating mock sensor data) from "Control Logic" (reacting to data).
* **Clone-Safe Configuration:** Uses a custom Device Tree Overlay (`app.overlay`) to force the Internal High-Speed Oscillator (HSI), bypassing common crystal failures on STM32 clones.
* **Test Automation Interface:** Includes a UART Shell (CLI) to manually inject voltage faults and override the physics engine for unit testing.
* **Safety Logic:** Implements Over-Voltage (OV) and Under-Voltage (UV) protection states using LED indicators.

## 🏗️ System Architecture

The application runs two primary threads managed by the Zephyr Kernel:

| Thread | Priority | Frequency | Responsibility |
| :--- | :--- | :--- | :--- |
| **SolarPhysics** | High (7) | 10 Hz | Simulates voltage ramp (Day/Night cycle) 0V ↔ 50V. |
| **Controller** | Normal (5) | 5 Hz | Reads voltage, determines state (Generating/Fault), controls LED. |
| **Shell/Log** | Low | Event | Handles user input and system logging via UART. |



## 🛠️ Hardware & Config
* **Target:** STM32F103C8T6 "Blue Pill" (Clone/Genuine)
* **Clock Source:** Internal 8MHz HSI (PLL -> 48MHz System Clock)
* **Indicators:**
    * **Solid ON:** Normal Operation (20V - 45V)
    * **Blinking:** Fault Condition (<20V or >45V)

## 💻 Code Structure

### 1. Hardware Mocking (`src/main.c`)
Instead of blocking on ADC reads, the system uses a shared variable `simulated_voltage`.
```c
/* Physics Thread */
void solar_simulation_thread(void) {
    // Ramps voltage up and down to mimic sunlight intensity
    if (rising) simulated_voltage += 0.5f;
}

```
### 2. The Clone Fix (`app.overlay`)
To ensure reliability on non-genuine ST chips, the external crystal (HSE) is disabled via Device Tree.
```dts
/* Force Internal High-Speed Oscillator */
&clk_hse { status = "disabled"; };
&clk_hsi { status = "okay"; };
&pll {
    clocks = <&clk_hsi>;
    mul = <12>; /* 4MHz * 12 = 48MHz */
};
```

### 2. OS Configuration Section (`prj.conf`)

```markdown
### 3. OS Configuration (`prj.conf`)
Enables the Shell and floating-point support while optimizing stack sizes for the STM32F1's limited RAM.
```properties
CONFIG_SHELL=y
CONFIG_CBPRINTF_FP_SUPPORT=y
CONFIG_MAIN_STACK_SIZE=4096  # Increased for stability
```

### 3. Build Instructions Section

```markdown
## ⚙️ Build Instructions

### Prerequisites
* Zephyr SDK 0.16+
* West Build Tool
* OpenOCD

### 1. Build
```bash
west build -p auto -b stm32_min_dev

```
### 2. Flash
```bash
west flash --runner openocd
```
### 3. Manual Testing (Shell)

```bash
uart:~$ sensor set 48
Manual Override: 48 V
# LED should start blinking (Over Voltage Fault)
```

## 🐛 Troubleshooting "Clones"
If flashing fails with `AP write error` or reset issues:
1.  **Hold RESET:** Press and hold the physical RESET button on the board.
2.  **Run Flash:** Execute the `west flash` command.
3.  **Release:** Release the button once OpenOCD detects the ST-Link.

---
