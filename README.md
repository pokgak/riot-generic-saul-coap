# README

## Done

- auto detect registered sensors from SAUL registry and create
corresponding CoAP resource (/0, /0/val)
- read multidimensional value from sensors
- write value to single dimension actuator (switch)
- Write TD using CBOR
- send and handle CBOR response
- Parse decoded CBOR and print (map, int, uint, string, bool)

## TODO

- compare TD JSON and CBOR size
- recognize if the sensor already registered/CoAP resource created
- write to multidimensional actuator (RGB LED)
- Thing fields not fully implemented (id, description, support, links, security)
- create TD when registering the sensor, not when requested
- CBOR parser cannot parse arrays, simple, tag, array
- Memory allocation can be optimized (stack size, cbor context size)
