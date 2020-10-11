#ifndef LEDS_H
#define	LEDS_H

#include "BOARD.h"
#include <xc.h>

//LEDS_INIT() sets the TRISE and LATE registers to 0
#define LEDS_INIT() do  {   \
                        TRISE = 0x00;   \
                        LATE = 0x00;    \
                    } while (0)

//LEDS_SET(x) sets the LATE register to x
#define LEDS_SET(x) do  {   \
                        LATE = (x); \
                    } while (0)

//LEDS_GET returns the value of the LATE register
#define LEDS_GET    \
                LATE    

#endif

