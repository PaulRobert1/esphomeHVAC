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
HVACSolarHeater is a 2 in 1 config, controls solar pump with pwm and controls HVAC.


