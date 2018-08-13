# README

1. `make BOARD=pba-d-01-kw2x SERIAL=02000203C3644E7E3E98B386 flash term` on the yellow board
2. `make BOARD=samr21-xpro SERIAL=ATML2127031800002115 flash term` on samr21-xpro`
3. On yellow board:
  - run `dsc` to init the resources
  - run `ifconfig` and get the IPv6 adress
4. On samr21-xpro:
  - `coap get [ip yellow board] 5683 /.well-known/core`

Currently it will returns: 
```
2018-08-13 10:35:05,212 - INFO # coap get fe80::4bf0:6d4f:52a8:432a 5683 /.well-known/core
2018-08-13 10:35:05,215 - INFO # gcoap_cli: sending msg ID 3229, 23 bytes
2018-08-13 10:35:05,230 - INFO #  gcoap: response Success, code 2.05, 71 bytes
2018-08-13 10:35:05,236 - INFO # </cli/stats>,</riot/board>,<>,<>,<>,<>,<>,<>,<>,<>,<>,<>,<>,<>,<>,<>,<>
```
The resource URI is empty. Why???
