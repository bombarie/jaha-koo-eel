
# Bill Of Materials

## 3Print files

> 💡 Print all these files in PETG

| Quantity | Model             | Color       |
| -------- | ----------------- | ----------- |
| 1        | Motor Mount       | Transparent |
| 1        | Head Upper        | Transparent |
| 1        | Head Lower        | Transparent |
| 1        | Switch cap        | Transparent |
| 2        | Eye Light Guide   | Black       |
| 2        | Mouth Light Guide | Black       |
| 2        | Front Wheel       | Transparent |
| 1        | Tail              | Transparent |
| 19       | Segment           | Transparent |
| 8        | Segment with n00d | Transparent |
| 1        | Segment battery   | Transparent |
| 4        | Axle              | Transparent |
| 8        | Wheel             | Transparent |

## Non-electronic materials

| Quantity | Component               | Comments                                                                                                         |
| -------- | ----------------------- | ---------------------------------------------------------------------------------------------------------------- |
| 8        | O-ring for wheels       | Transparent silicone                                                                                             |
| 2        | O-ring for front wheels | Transparent silicone                                                                                             |
| 10       | MR63ZZ ball bearings    | [link](https://www.amazon.nl/Miniatuur-Kogellagers-Afgeschermde-Afgedichte-3D-Printermodellering/dp/B078RFDJXP/) |

## Electronics

### Eel

#### Mainboard (contains the microcontroller)

[Interactive BOM / connection guide (is handy when assembling)](./kicad/eel_head_pcbs_assembly/eel_esp32_switch_host_board_v02/_export/bom%20(interactive)/ibom.html)

| Quantity | Component                    | Comments                                                                          |
| -------- | ---------------------------- | --------------------------------------------------------------------------------- |
| 1        | XIAO ESP32-C3                | microcontroller                                                                   |
| 1        | 100nF capacitor              | 0805 SMD                                                                          |
| 1        | 330ohm resistor              | 0805 SMD                                                                          |
| 1        | 1kohm resistor               | 0805 SMD                                                                          |
| 1        | Nidec 2x03 dpdt CL-SB-22B-11 | SMD, on/off switch, [link](https://nl.rs-online.com/web/p/slide-switches/2443543) |
| 1        | BC817 NPN transistor         | SOT-23                                                                            |


#### Additional components

| Quantity | Component                                                  | Comments                                                                                                                                                                                      |
| -------- | ---------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 2        | N20 motor                                                  | Only 3V / 6V motors have sufficient torque. Difficult to find, most motors are 12V                                                                                                            |
| 1        | DRV8833 motor driver                                       | [link](https://www.tinytronics.nl/en/mechanics-and-actuators/motor-controllers-and-drivers/stepper-motor-controllers-and-drivers/drv8833-bipolar-stepper-motor-and-dc-motor-motor-controller) |
| 1        | DC-DC Step-Up-Down Buck-Boost Converter - 0.6A - 5V Output | [link](https://www.tinytronics.nl/en/power/voltage-converters/buck-boost-(step-up-down)-converters/dc-dc-step-up-down-buck-boost-converter-0.6a-5v-output)                                    |
| 3        | WS2812b rgb led (neopixel)                                 | [link](https://www.hobbyelectronica.nl/product/rgb-led-ws2812b-mini-pcb/)                                                                                                                     |
| 2        | Nood pink                                                  | flexible LED filament - 30cm                                                                                                                                                                  |
| 2        | Nood blue                                                  | flexible LED filament - 30cm                                                                                                                                                                  |
| 1        | Vapcell INR18350 1100mAh Li-ion battery                    | [link](https://www.nkon.nl/vapcell-18350-1100-10a.html)                                                                                                                                       |

<!-- DO LATER
### Transmitter

| Quantity | Component | Comments |
| -------- | --------- | -------- |
|          |           |          |
|          |           |          |
-->
