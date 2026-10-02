# ESPHOME-universal-board

Modular, multi-purpose automation control boards designed around the **Seeed Studio XIAO** form-factor (ESP32-C3 / S3 / RP2040), tailored for **ESPHome**, **Home Assistant**, and standalone embedded automation projects.

The system consists of three hardware revisions/modules:
1. **Universal Board (Full Version)** – High-density I/O, mains-powered, precision analog/PWM expansion, gate-driven power MOSFETs, and relays.
2. **Universal Board (Light Version)** – Compact DC-powered board with direct logic-level SMD MOSFETs, relays, and inputs.
3. **Relay Expansion Board (Stackable)** – Dedicated 5-channel relay daughterboard controlled via the Full version's I2C PWM expander.

---

## Hardware Overview & Specifications

### 1. Board One: Full Version (Flagship)

Designed as a central automation node capable of direct 230 V mains operation, multi-channel high-current PWM control, mixed-signal ADC/DAC sensing, and galvanic isolation.

* **Core MCU:** Socket for Seeed Studio XIAO module (ESP32-C3, ESP32-S3, etc.).
* **Power Supply Options:**
  * **Mains AC Input:** Onboard **Hi-Link HLK-20M12** (20 W, 12 V / 1.66 A) isolated AC/DC power supply.
  * **Mains Protection:** 10D561K Metal Oxide Varistor (MOV) for surge and transient suppression.
  * **DC Input:** 2.1 mm DC Barrel Jack (`PJ-102AH`) for external 12 V DC supply.
  * **Step-Down:** Footprint for LM7805 LDO or pin-compatible high-efficiency DC-DC switching buck regulator (12 V to 5 V).
* **Power Outputs (4x High-Current PWM):**
  * **MOSFETs:** TO-220 footprint (tested with **IRL540N** / **IRFB4110PBF**).
  * **Gate Drivers:** Dual high-speed drivers (**TC4427** running at 12 V rail). Delivers up to 1.5 A peak gate drive current for fast switching transitions and minimal thermal loss during high-frequency PWM.
  * **Ratings:** Up to 60 V DC, 10 A burst / 5 A continuous per channel (dependent on PCB copper weight and heatsinking).
* **Analog & Precision I/O:**
  * **Analog Devices AD5593R** (12-bit, 8-channel configurable ADC / DAC / GPIO via I2C).
  * Onboard 2.5 V low-drift reference (`VREF`), filtered analog supplies, and dedicated 8-pin terminal/pin header.
* **Expanded PWM:**
  * **NXP PCA9685PW** (16-channel, 12-bit I2C PWM controller).
  * Channel `LED0` can be routed to high-current output/dimming, remaining channels (`EX_PWM1`–`EX_PWM15`) routed to stack header `JP1`.
* **Relays (2x Isolated):**
  * **Songle SRD-05VDC-SL-C** SPDT relays (rated 250 VAC / 10 A, 30 VDC / 10 A).
  * **Isolation & Drive:** Opto-isolated via **CPC1018N** 1-Form-A Solid State Relays (SSR) to isolate MCU logic from inductive switching spikes.
  * Flyback protection diodes on each coil.
* **Digital Inputs (2x Terminals):**
  * Dedicated screw terminals with 1k series current limiting.
  * **Nexperia PESD3V3L2BT** TVS/ESD diode array for surge/ESD protection on 3.3 V logic.
* **Expansion:** All unused XIAO GPIOs routed to standard 2.54 mm pin headers (`PHD1`, `PHD2`).

---

### 2. Board Two: Light Version

A low-profile, cost-optimized DC-powered board ideal for decentralized sub-nodes, LED strip dimming, and valve/motor switching.

* **Core MCU:** Seeed Studio XIAO module footprint.
* **Power Supply:**
  * DC Barrel Jack (`PJ-102AH`) feeding a dedicated pin-compatible DC-DC Step-Down module.
  * USB-C 5 V powering option with selectable `U_SEL` power routing jumper.
* **Power Outputs (3x SMD PWM):**
  * **MOSFETs:** **AO3400A** (N-channel, 30 V / 5.7 A).
  * Driven directly from MCU GPIOs.
  * Ideal for 12 V / 24 V LED strips (up to 3 A continuous per channel without forced cooling).
* **Relays (2x):**
  * 2x **SRD-05VDC-SL-C** driven via **CPC1018N** solid-state optical switches.
* **Digital Inputs (2x):**
  * Terminal block inputs with pull-ups/pull-downs for reed switches, dry contacts, or external buttons.
* **Expansion:** Unused pins broken out to side headers for I2C or SPI sensors.

---

### 3. Board Three: Expansion Board (Stackable)

An add-on hat designed to mount directly onto the **Full Version** via headers `PHD1`, `PHD2`, and `JP1`.

* **5x Additional Relays:** **SRD-05VDC-SL-C** (230 VAC / 10 A contact rating).
* **Control Bus:** Driven directly by the Full Board’s **PCA9685** PWM/GPIO expander (`EX_PWM1` through `EX_PWM5`).
* **Isolation:** Each relay coil is switched through an independent **CPC1018N** optical isolation relay.
* **Pass-through Routing:**
  * Leaves `EX_PWM6`–`EX_PWM15` available for additional servo or PWM peripherals.
  * Passes through AD5593R DAC/ADC lines and spare MCU GPIOs.

---


## Hardware Operating Limits & Safety Guidelines

> [!WARNING]
> **Mains Voltage Hazard:** The Full Version contains exposed 230 VAC tracks. Always install in an insulated DIN-rail or 3D-printed enclosure (**FR MATERIAL**). Maintain appropriate creepage and clearance distances between mains tracks and low-voltage logic.

* **Relay Contacts:** Rated for 10 A @ 250 VAC resistive load. For inductive loads (motors, pumps, contactors).
* **Full Board PWM Channels:** While the MOSFETs (IRFB4110 / IRL540N) have high silicon current ratings.
* **Light Board PWM Channels:** AO3400A is rated for 5.7 A peak; recommended continuous load per channel is **2.5 A to 3 A** without a copper heatsink pour.
* **Power Supply Headroom:** The HLK-20M12 provides up to 1.66 A at 12 V. When running multiple relays and external 12 V loads simultaneously, verify total DC power consumption remains under 20 W.

---

## AD5593R ESPHome Component

This repository includes a custom ESPHome component for the **Analog Devices AD5593R** (8-channel 12-bit configurable ADC / DAC / GPIO).

### How to Import into ESPHome

Add the `external_components` block to your YAML configuration:

#### Option A: Direct from GitHub
```yaml
external_components:
  - source: github://bafallocz/ESPHOME-universal-board
    components: [ ad5593r ]
```

#### Option B: Local Component (when compiling inside this repo)
```yaml
external_components:
  - source: components
    components: [ ad5593r ]
```

---

### AD5593R Configuration Guide

Any of the 8 pins (`0` to `7`) can be independently configured as an **ADC input**, **DAC output**, **Digital input (`binary_sensor`)**, or **Digital output (`switch`)**.

#### 1. Hub Definition
```yaml
ad5593r:
  - id: ad5593r_hub
    i2c_id: bus_a
    address: 0x10
    # reference_voltage: 2.5V  # Internal reference (default 2.5V)
    # gain_2x: true            # Default true -> Full range 0 to 5.0 V
```

#### 2. Analog Outputs (DAC) - Channels 0 to 7
Outputs range from `0.0` (0 V) to `1.0` (5.0 V):
```yaml
output:
  - platform: ad5593r
    ad5593r_id: ad5593r_hub
    channel: 0
    id: dac_out_0

  - platform: ad5593r
    ad5593r_id: ad5593r_hub
    channel: 1
    id: dac_out_1
```

Control DAC outputs using a Home Assistant template slider:
```yaml
number:
  - platform: template
    name: "DAC 0 Output"
    min_value: 0
    max_value: 1
    step: 0.01
    mode: slider
    set_action:
      - output.set_level:
          id: dac_out_0
          level: !lambda 'return x;'
```

#### 3. Analog Inputs (ADC) & Internal Temperature Sensor
Reports voltage values in **Volts (V)**:
```yaml
sensor:
  - platform: ad5593r
    ad5593r_id: ad5593r_hub
    channel: 2
    name: "AD5593R Analog Input 2"
    update_interval: 1s

  - platform: ad5593r
    ad5593r_id: ad5593r_hub
    channel: 3
    name: "AD5593R Analog Input 3"
    update_interval: 1s

  # Chip Internal Temperature Sensor (°C)
  - platform: ad5593r
    ad5593r_id: ad5593r_hub
    channel: temperature
    name: "AD5593R Internal Temperature"
    update_interval: 10s
```

#### 4. Digital Inputs (`binary_sensor`)
Reads logic state from pins (with optional 85 kΩ internal pull-down):
```yaml
binary_sensor:
  - platform: ad5593r
    ad5593r_id: ad5593r_hub
    channel: 4
    name: "AD5593R Digital Input 4"
    pulldown: true
```

#### 5. Digital Outputs (`switch`)
Controls digital output logic high/low on pins:
```yaml
switch:
  - platform: ad5593r
    ad5593r_id: ad5593r_hub
    channel: 5
    name: "AD5593R Digital Output 5"
```
