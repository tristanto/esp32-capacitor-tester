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
