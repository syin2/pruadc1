//  Attempt to duplicate Derek Molloy's
//  SPI ADC read program in C from assembly.
//  Chip Select:  P9.27 pr1_pru0_pru_r30_5
//  MOSI:         P9.29 pr1_pru0_pru_r30_1
//  MISO:         P9.28 pr1_pru0_pru_r31_3
//  SPI CLK:      P9.30 pr1_pru0_pru_r30_2
//  Sample Clock: P8.46 pr1_pru1_pru_r30_1  (testing only)

#include "resource_table_empty.h"
#include <stdint.h>

#define PRU_SHAREDMEM 0x00010000
  volatile register uint32_t __R30;
  volatile register uint32_t __R31;

//  uint32_t spiCommand = 0x80;  // Single-ended, Channel 0
  uint32_t spiCommand;
  uint32_t numSamples = 1000000;  // Number of samples

 int main(void){
//  1.  Enable OCP Master Port
//  2.  Initialization

   uint32_t bitMask; //    = 0x000003FF;  //  Keep only 10 bits using this mask.

   uint32_t data = 0x00000000;  // Incoming data stored here.
//  The data out line is connected to R30 bit 1.
  __R30 = 0x00000000;  //  Clear the output pin.

//  The sample clock is located at shared memory address 0x00010000.
//  Need a pointer for this address.  This is found in the linker file.
//  The address 0x0001_000 is PRU_SHAREDMEM.
   uint32_t * clockPointer = (uint32_t *) 0x00010000;
   __R30 = __R30 | (1 << 5);  // Initialize chip select HIGH.
   __delay_cycles(100000000);  //  Allow chip to stabilize.

//  3.  SPI Data capture loop.  This captures numSamples data samples from the ADC.

for(int i = 0; i < numSamples; i = i + 1) {  //  Outer loop.  This determines # samples.
 while(!(*clockPointer)){}  //  Hold until the Master clock from PRU1 goes high.

//  spiCommand is the MOSI preamble; must be reset for each sample.
   spiCommand = 0x80000000;  // Single-ended Channel 0
   data = 0x00000000;  //  Initialize data.
   __R30 = __R30 & 0xFFFFFFDF;  //  Chip select to LOW P9.27
   __R30 = __R30 & 0xFFFFFFFB;  //  Clock to LOW   P9.30
//  Start-bit is HIGH.  The Start-bit is manually clocked in here.
   __R30 = __R30 | (1 << 1);  //  Set a 0x10 on MOSI (the Start-bit)
   __delay_cycles(2500);  //  Delay to allow settling.
   __R30 = __R30 | 0x00000004;  //  Clock goes high; latch in start bit.
   __delay_cycles(2500);  //  Delay to allow settling.
   __R30 = __R30 & 0xFFFFFFFB;  //  Clock to LOW   P9.30
// The Start-bit cycle is completed.

//  Get 24 bits from the data line MISO R31.t3 (bit 3)
   for(int i = 0; i < 23; i = i + 1) {  //  Inner single sample loop

//  The first action will be to transmit on MOSI by shifting out spiCommand variable.
    if(spiCommand >> 31) //  If the MSB is 1 
    __R30 = __R30 | (1 << 1);  //  Set a 0x10 on MOSI
    else 
    __R30 = __R30 & (0xFFFFFFFD);  //  All 1s except bit 1
    spiCommand = spiCommand << 1;  // Shift left one bit (for next cycle).
   __delay_cycles(2500);
   __R30 = __R30 | 0x00000004;  //  Clock goes high; bit set on MOSI.
   __delay_cycles(2500);  //  Delay to allow settling.
    
//  The data needs to be "shifted" into the data variable.
    data = data << 1;  // Shift left; insert 0 at lsb.
   __R30 = __R30 & 0xFFFFFFFB;  //  Clock to LOW   P9.30
//   __delay_cycles(2500);  //  Delay to allow settling.

    if(__R31 & (1 << 3)) //  Probe MISO data from ADC.
    data = data | 1;
    else
    data = data & 0xFFFFFFFE;
  }  //  End of 24 cycle loop
   __R30 = __R30 | 1 << 5;  //  Chip select to HIGH
}
 //  End data acquisition loop.

/*
//   __R31 = 35;                      // PRUEVENT_0 on PRU0_R31_VEC_VALID
*/
   __halt();                        // halt the PRU
}