# Bi-Stable-Relay-Driver
An ATINY based driver for bi-stable relays (spdt-switches), e.g. 2SE1T11JB from Ducommun Technologies.

For Microwave transverters a RX/TX-relay is essential. There are a lot of spdt-switch-type relays available with a performance up to 24GHz. For such relays a driver with a ATINY85 as base was designed. The program flashed to ATINY85 is shown here. Programming and flashing was done with ATMEL Studio 7.0 and with great help of DL8DTL.
BOM (minimum)
1. ATINY85
2. voltage regulator 5V
3. elco 100µF
4. 100nF
5. BTS 432E2 2x (High Side Switch)
extra:
LED

For debuging reasons there are also LEDs triggered to show in which status the driver is. The high side switches are needed, for the relay used. A low side switch can be made with a single transistor.
