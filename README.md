# esphomeHVAC
esphome yaml to control a diy water to water heat pump with power output among other sensors

Hello, guess what? I've almost managed to semi-sort-of-not-really finish setting up the house heating extravaganza! 
A 300-liter tank with not one, but two snake-like doodads (aka serpentine thingies), two hot water panels, and a MiniSol controller 
(which turned out to be a mega bummer; note to self: build my own next time).

I cobbled together a DIY water-to-water heat pump from the guts of an ancient compressor from a portable AC unit. 
Add in two heat exchangers with plates, two circulation pumps, and a piping route of 250 meters buried three meters deep for the cold side—voilà!
Controlling this madness involved some tech wizardry with an esp8266 for the heat pump, and esp32 integrated into Home Assistant for the heating circuits. 
Plus, temperature sensors everywhere—Dallas sensors measuring temps for the motor, the hot/cold heat exchanger, and the storage tank.

Picture this: 6 Dallas temperature sensors, a 4-relay board, an ESP8266, 
and not one, but TWO generic water flow sensors (they're like the quirky sidekicks in this story).

Sharing the code, maybe it will help others achieve a more efficient cost effective heating solution

************************************************************
Meanwhile esp8266 did not live up to the task so I switched to a esp32 and updated the code, also added a 2in1 controller for both solar and heat pump controls.
****************************
UPDATE:
HVACSolarHeater is a 2 in 1 config, controls solar pump with pwm and controls HVAC. The DIY Heatpump has been comissioned 2 years ago, last fall gas pressure was same as one year prior, so no leaks.
It is able to heat 120 square meters, house is wagon type so not the most power efficient. Added Solar heater functionality because commrcial solar controller lacks HA compatibility and SW dewelopers showed no interest to add it. Recirculation pump for the solar water panels is a grundfos which takes pwm input.
HVAC Error handling has been also grately worked on, encountered alot of bugs in 2 years of operation.
If one of the following is abnormal the system will halt and the device in HA will show the error.
Hot and Cold flow, compressor too hot, hot and cold abnormal temperature when operating.

In the near future I also want to add a hal sensor to the compressor to detect if it is stuck or not starting up and some sort of pressure sensors for the HVAC refrigerant, was thinking about automotive AC sensors.
Solar heater sensor and tank backup sensors I used bi-metal temp sensors due to their high temperature resistance. (In summer solar panels when not operating heat upto 200 celsius)
1) Parts you need
Core

ESP32 DevKit (ESP32-WROOM style)

2× MAX6675 modules + 2× K-type thermocouples
5× DS18B20 (Dallas 1-Wire) temperature sensors (you have 5 addresses)
2× water flow sensors (pulse output)
Power / wiring essentials
Stable 5V supply for ESP32 (USB or buck converter)
3.3V reference available (ESP32 3V3 pin)
4.7k resistor for DS18B20 data pull-up (mandatory)
Pull-up resistors for flow sensors if they are open-collector (often 4.7k–10k to 3.3V)
Switching / driving loads
Relays / SSR / MOSFET drivers for:
Compressor
Hot pump
Cold pump
“compressorcool” valve/relay
Optional DS18B20 power switching (GPIO17)
A proper MOSFET driver stage for the PWM solar pump if it’s a DC pump (don’t drive a pump directly from ESP32 pin)

pin layout configured at the time

Inputs (sensors)

GPIO2: 1-Wire bus for DS18B20 sensors
GPIO36: Hot flow pulse input (flowhot)
GPIO39: Cold flow pulse input (flowcold)
MAX6675 #1 CS: GPIO18 (tank_temp2)
MAX6675 #2 CS: GPIO22 (collector_temp)
SPI bus:
CLK: GPIO1
MISO: GPIO3

Outputs (actuators)

GPIO0: PWM solar pump (LEDC output, pwm_pump)
GPIO25: Compressor relay (compressor)
GPIO14: Hot pump relay (pumphot)
GPIO27: Cold pump relay (pumpcold)
GPIO4: Compressorcool relay/valve (inverted: active LOW)
GPIO23: Alarm LED output
GPIO17: Dallas temp “power” switch output (dallas_temppower)

