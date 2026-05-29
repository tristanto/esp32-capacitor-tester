# Capacitor Measurement System: Working Principles

This document explains the technical theories and operational workflows for measuring **Capacitance** and **Dielectric Leakage Current** using an ESP32 microcontroller under the ESP-IDF framework.

---

## 1. Capacitance Measurement Principle

### 1.1 Fundamental Theory (RC Time Constant)
The measurement of capacitance ($C$) is based on the transient response of an **RC (Resistor-Capacitor) Charging Circuit**. A microcontroller cannot measure capacitance directly; instead, it measures the exact time duration required to charge an unknown capacitor through a known precision resistor ($R$).

According to electronic physics, when an empty capacitor is connected to a DC voltage source ($V_{in}$) through a resistor, the voltage across the capacitor ($V_c$) increases over time following an exponential curve:

$$V_c(t) = V_{in} \left(1 - e^{-\frac{t}{RC}}\right)$$

When the elapsed time ($t$) exactly equals the product of $R \times C$, the exponent becomes $-1$:

$$V_c(\tau) = V_{in} \left(1 - e^{-1}\right) \approx V_{in} \times 0.632$$

This specific duration is called the **Time Constant ($\tau$)**. At exactly $1\tau$, the capacitor will reach **63.2% of its maximum source voltage**. By rearranging the formula, the unknown capacitance can be calculated mathematically:

$$C = \frac{\tau}{R}$$

### 1.2 Operational Hardware Workflow

```text
[ Discharge Pin LOW ] ──► [ Charge Pin HIGH ] ──► [ Start MCPWM Timer ]
                                                            │
[ Capture Stop Time ] ◄── [ Comparator Triggers ] ◄─────────┘ (When Vc = 2.08V)
```

1. **Discharging Phase:** A charging GPIO pin is set as a digital *Output LOW* ($0\text{V}$) for a fixed duration to completely drain any residual charges inside the capacitor to Ground ($GND$).
2. **Excitation & Timing Initiation:** The charging GPIO pin switches instantly to *Output HIGH* ($3.3\text{V}$) to begin filling the capacitor. At the exact same microsecond, an internal hardware timer (**MCPWM Capture Timer**) records the initial timestamp ($T_{start}$).
3. **Threshold Detection:** An analog reference voltage is set precisely at **$2.08\text{V}$** (which is $63.2\%$ of $3.3\text{V}$). This reference voltage is fed into one input of a high-speed comparator (e.g., IC LM393), while the other input monitors the rising voltage of the capacitor.
4. **Hardware Capture:** The moment the capacitor voltage crosses $2.08\text{V}$, the comparator's output toggles from *LOW* to *HIGH*. This sharp rising edge triggers an instant hardware interrupt, capturing the end timestamp ($T_{end}$) with nanosecond-level precision.
5. **Calculation:** The software subtracts the timestamps to find the duration ($\tau = T_{end} - T_{start}$). Knowing the exact resistance value ($R$), the system calculates the capacitance value in microfarads ($\mu\text{F}$) or nanofarads ($\text{nF}$).

---

## 2. Leakage Current Measurement Principle

### 2.1 Fundamental Theory (Self-Discharge Method)
An ideal capacitor retains its stored electric charge indefinitely when disconnected from a circuit. However, real-world capacitors possess an imperfect internal dielectric material that exhibits a very high, yet finite, parallel resistance. This imperfection allows a minuscule current to flow through the dielectric layer, known as **Leakage Current ($I_{leak}$)**.

Since leakage current is extremely small—typically in the microampere ($\mu\text{A}$) or nanoampere ($\text{nA}$) range—it cannot be measured accurately using standard shunt resistors and a microcontroller's built-in ADC. Instead, the **Indirect Self-Discharge Method** is employed.

By isolating a fully charged capacitor, it will naturally discharge itself solely through its internal leakage pathways. The average leakage current over a monitored timeframe can be calculated using the fundamental relationship between charge, capacitance, voltage, and time:

$$I_{leak} = C \times \frac{\Delta V}{\Delta t}$$

Where:
*   $C$ = The pre-measured capacitance value (Farads).
*   $\Delta V$ = The voltage drop over the observation period ($V_{initial} - V_{final}$).
*   $\Delta t$ = The precise duration of the observation window (seconds).

### 2.2 Operational Hardware Workflow

```text
[ Saturation: 3.3V ] ──► [ Read V_initial ] ──► [ Switch GPIO to Hi-Z (Floating) ]
                                                              │
[ Compute Leakage ] ◄─── [ Read V_final ] ◄───── [ Wait Observation Window (dt) ]
```

1. **Saturation Phase:** The GPIO pin connected to the capacitor is set to *Output HIGH* ($3.3\text{V}$) for a prolonged period (e.g., 10 seconds). This long duration is mandatory to overcome the **dielectric absorption effect** and ensure the molecular structures within the capacitor are fully saturated with charge.
2. **Initial Voltage Registration:** The ADC reads the initial stabilized voltage across the capacitor, recording it as $V_{initial}$ (ideally close to $3300\text{ mV}$).
3. **High-Impedance Isolation:** The microcontroller instantly reconfigures the connected GPIO pin from an active output into a **Floating Input Mode (High-Impedance / Hi-Z)**. In this state, the pin acts as an open circuit, preventing any current from leaking into the microcontroller.
4. **The Settling Window:** The system enters a passive countdown phase (e.g., 10 seconds). During this window, the capacitor is completely isolated, forcing it to discharge strictly through its own internal dielectric flaws.
5. **Final Voltage Registration & Computation:** Once the timer expires, the ADC reads the remaining voltage, recording it as $V_{final}$. The software calculates the total voltage drop ($\Delta V = V_{initial} - V_{final}$). Using the predefined capacitance ($C$) and the time window ($\Delta t$), the system computes the exact leakage current ($I_{leak}$).

---

## 3. Summary of Measurement Methods


| Measurement Target | Sensor Core / Peripheral | Primary Hardware Advantage | Typical Target Range |
| :--- | :--- | :--- | :--- |
| **Capacitance** | MCPWM Capture Timer + External LM393 | Nanosecond-level timestamping independent of FreeRTOS software delays. | $10\text{ nF}$ to $10,000\text{ }\mu\text{F}$ |
| **Leakage Current** | High-Impedance GPIO + ADC Oneshot | Total electrical isolation prevents the ESP32 from absorbing the charge. | $0.1\text{ }\mu\text{A}$ to $50\text{ }\mu\text{A}$ |





# Wi-Fi Configuration Manager (ESP-IDF v6.0+)

A robust and production-ready Wi-Fi manager for ESP32 devices built on the **ESP-IDF v6.0** framework. It automatically handles Dual-Mode Wi-Fi switching: **Access Point (AP) Mode** for initial provisioning and **Station (STA) Mode** for normal client operation. 

Instead of deprecated EEPROM, this project utilizes **NVS (Non-Volatile Storage)** for secure and wear-leveled credential storage.

---

## 🚀 How It Works

1. **First Boot / Factory Reset**: The device boots up \(\rightarrow\) checks NVS \(\rightarrow\) credentials are empty \(\rightarrow\) triggers **Setting Mode (AP Mode)**.
2. **Normal Operation**: The device boots up \(\rightarrow\) credentials found in NVS \(\rightarrow\) connects directly to your local Wi-Fi router as a client (**STA Mode**).
3. **Manual Re-configuration**: Pressing the physical hardware button during boot forces the device to wipe the saved credentials and re-enter **Setting Mode**.

---

## 🛠️ Hardware Requirements

* **ESP32 Development Board** (e.g., ESP32-WROOM-32, ESP32-S3, etc.)
* **Physical Reset/Boot Button**: Defaulted to **GPIO 0** (The built-in `BOOT` button on most ESP32 boards).

---

## 📲 Step-by-Step Usage Guide

### Scenario A: Initial Setup (First-Time Configuration)
1. Power up your ESP32 device.
2. Open your smartphone or computer's Wi-Fi settings and scan for available networks.
3. Connect to the ESP32 configuration hotspot:
   * **SSID:** `ESP32_Config_AP`
   * **Password:** `12345678`
4. Once connected, open your favorite web browser and navigate to: **`http://192.168.4.1`**
5. The ESP32 will automatically scan and display a dropdown list of available local Wi-Fi networks.
6. **Select your Wi-Fi SSID**, type your **Password**, and click **"Simpan dan Hubungkan"** (Save and Connect).
7. The ESP32 will save the configuration to NVS and reboot automatically into **Normal Operation Mode**.

---

### Scenario B: Changing Wi-Fi Network Later (Force Configuration Mode)
If you change your home router or Wi-Fi password, you can manually trigger the setting page again:
1. Turn off the power or prepare your finger on the **BOOT** button.
2. Press and **HOLD the BOOT button (GPIO 0)**, then power up the device (or press the `EN/RST` button once while holding BOOT).
3. Keep holding the **BOOT** button for **at least 3 seconds**.
4. Release the button when you see the logs or the device resets. 
5. The old credentials are now erased from the NVS memory. The device will spin up the `ESP32_Config_AP` network again, allowing you to follow the steps in **Scenario A** to input new details.

---

## 💻 Developer Integration Notes

### File Structure
Ensure your component directory contains the following files:
```text
main/
├── CMakeLists.txt
├── main.c
├── wifi_config_mgr.c
└── wifi_config_mgr.h
```

### CMake Configuration
To prevent `implicit declaration` or `No such file or directory` errors in ESP-IDF v6.0, your component's **`CMakeLists.txt`** must explicitly declare the network and storage requirements:

```cmake
idf_component_register(SRCS "main.c" "wifi_config_mgr.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_http_server esp_wifi nvs_flash driver)
```

---

## 📊 Serial Monitor Logs Reference
* **On First Boot:** `[WIFI_APP] Starting access point mode for configuration...`
* **On Button Hold:** `[WIFI_APP] Reset button detected! Hold for 3 seconds...` $\rightarrow$ `[WIFI_APP] Factory reset triggered! Wi-Fi credentials cleared.`
* **On Normal Operation:** `[WIFI_APP] Starting client/station mode...`
