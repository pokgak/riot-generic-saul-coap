# README

1. `make BOARD=pba-d-01-kw2x SERIAL=02000203C3644E7E3E98B386 flash term` on the yellow board
2. `make BOARD=samr21-xpro SERIAL=ATML2127031800002115 flash term` on samr21-xpro`
3. On yellow board:
  - run `gsc` to init the resources
  - run `ifconfig` and get the IPv6 adress
4. On samr21-xpro:
  - `coap get [ip yellow board] 5683 /.well-known/core`
5. Feel free to select test reading with the available endpoints
