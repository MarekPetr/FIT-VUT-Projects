/*
* Petr Marek, original, last edit on 19.12.2018.
*/


#include "MK60D10.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

/* Constants specifying delay loop duration */
#define DELAY_LED 2000

#define SET_MUX_DSE PORT_PCR_MUX(0x1) | PORT_PCR_DSE(0x1)
#define SET_MUX PORT_PCR_MUX(0x1)
#define BTN_SW6 0x800 // Port E, bit 11

/*
               P1  PCR
--------------
MCU_I2C0_SCL   17  PTD8
MCU_I2C0_SDA   18  PTD9
MCU_SPI2_CLK   19  PTD12
MCU_SPI2_MOSI  20  PTD13
MCU_SPI2_CS1   21  PTD15
MCU_SPI2_MISO  22  PTD14
PTA8           23  PTA8
PTA10          24  PTA10
PTA6           25  PTA6
PTA11          26  PTA11
PTA7           27  PTA7
PTA9           28  PTA9
*/

/*
            P1
segments    17 |PTD8  | PTD9 | 18
            19 |PTD12 | PTD13| 20
            21 |PTD15 | PTD14| 22
            23 |PTA8  | PTA10| 24
            ---------------------
displays    25 |PTA6  | PTA11| 26
            27 |PTA7  | PTA9 | 28

segments(7 + DP) emit light on logical 1
displays(4 digits with dots) emit light on logical 0
*/

/* 
DISPLAY (7 + 1(DP) segments)
---------------------------
D = PTD
A = PTA

   D12
   ---
 D9| |D8
   --- A10
D13| |A8
   ---
   D15  . D14
*/

int index = 0; // index in result (starts on 0)
char result[10] = "";

#define D1 9  // PTA9
#define D2 6  // PTA6
#define D3 11 // PTA11
#define D4 7  // PTA7 

// Turn on display with port controlled by bit on given position in GPIOA_PDOR register
void DigitON(int position)
{
    GPIOA_PDOR &= ~(1 << position);
}

// Turn off display with port controlled by bit on given position in GPIOA_PDOR register
void DigitOFF(int position)
{
    GPIOA_PDOR |= 1 << position;
}

void SetERR() {GPIOA_PDOR = 0b111011000000; GPIOD_PDOR = 0b0;}                 // only A10 segment on (- char)
void SetNOT() {GPIOA_PDOR = 0b101011000000; GPIOD_PDOR = 0b0;}                 // no segment on
void SetALL() {GPIOA_PDOR = 0b010100000000; GPIOD_PDOR = 0b1111001100000000;}  // every segment on (8.)
void SetDP()  {GPIOA_PDOR |= 0b101011000000; GPIOD_PDOR |= 0b100000000000000;} // only D14 segment on (. char)

void SetN0() {GPIOA_PDOR = 0b101111000000; GPIOD_PDOR = 0b1011001100000000;} // 0
void SetN1() {GPIOA_PDOR = 0b101111000000; GPIOD_PDOR = 0b100000000;}        // 1
void SetN2() {GPIOA_PDOR = 0b111011000000; GPIOD_PDOR = 0b1011000100000000;} // 2
void SetN3() {GPIOA_PDOR = 0b111111000000; GPIOD_PDOR = 0b1001000100000000;} // 3
void SetN4() {GPIOA_PDOR = 0b111111000000; GPIOD_PDOR = 0b1100000000;}       // 4
void SetN5() {GPIOA_PDOR = 0b111111000000; GPIOD_PDOR = 0b1001001000000000;} // 5
void SetN6() {GPIOA_PDOR = 0b111111000000; GPIOD_PDOR = 0b1011001000000000;} // 6
void SetN7() {GPIOA_PDOR = 0b101111000000; GPIOD_PDOR = 0b1000100000000;}    // 7
void SetN8() {GPIOA_PDOR = 0b111111000000; GPIOD_PDOR = 0b1011001100000000;} // 8
void SetN9() {GPIOA_PDOR = 0b111111000000; GPIOD_PDOR = 0b1001001100000000;} // 9

// pointers to void functions controlling display segments
typedef void (*funcPointers)(void);
funcPointers SetNumber[10] = {SetN0, SetN1, SetN2, SetN3, SetN4, SetN5, SetN6, SetN7, SetN8, SetN9};

/* A delay function */
void delay(long long bound) {

  long long i;
  for(i=0;i<bound;i++);
}

/*
 * Delay function using the LPTMR module
 * Source: https://community.nxp.com/thread/434930?fbclid=IwAR2secDLnjEOKNtGHKj-iHFaIg1pDLJL6dLHsJbx6P7UrAlBJihS4Ni4ss4
 */
void time_delay_ms(unsigned int count_val)
{
  /* Make sure the clock to the LPTMR is enabled */
  SIM_SCGC5|=SIM_SCGC5_LPTIMER_MASK;

  /* Set the compare value to the number of ms to delay */
  LPTMR0_CMR = count_val;

  /* Set up LPTMR to use 1kHz LPO with no prescaler as its clock source */
  LPTMR0_PSR = LPTMR_PSR_PCS(1)|LPTMR_PSR_PBYP_MASK;

  /* Start the timer */
  LPTMR0_CSR |= LPTMR_CSR_TEN_MASK;

  /* Wait for counter to reach compare value */
  while (!(LPTMR0_CSR & LPTMR_CSR_TCF_MASK));

  /* Clear Timer Compare Flag */
  LPTMR0_CSR &= ~LPTMR_CSR_TEN_MASK;

  return;
}

// Initialize the MCU - setting system clocks, turning the watchdog off
void MCUInit(void)  {
    // system clock initializatin
    MCG_C4 |= ( MCG_C4_DMX32_MASK | MCG_C4_DRST_DRS(0x1) );
    SIM_CLKDIV1 |= SIM_CLKDIV1_OUTDIV1(0x0);

    WDOG_STCTRLH &= ~WDOG_STCTRLH_WDOGEN_MASK; // disable watchdog
}

void PortsInit(void)
{   
    // Enable clock gate for for ports
    SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK | SIM_SCGC5_PORTE_MASK | SIM_SCGC5_PORTA_MASK;   

    PORTD_PCR8  = SET_MUX_DSE;
    PORTD_PCR9  = SET_MUX_DSE;
    PORTD_PCR12 = SET_MUX_DSE;
    PORTD_PCR13 = SET_MUX_DSE;
    PORTD_PCR15 = SET_MUX_DSE;
    PORTD_PCR14 = SET_MUX_DSE;

    PORTA_PCR6  = SET_MUX_DSE;
    PORTA_PCR7  = SET_MUX_DSE;
    PORTA_PCR8  = SET_MUX_DSE;
    PORTA_PCR9  = SET_MUX_DSE;
    PORTA_PCR10 = SET_MUX_DSE;
    PORTA_PCR11 = SET_MUX_DSE;

    PORTE_PCR11 = SET_MUX; // SW6

    GPIOA_PDDR  = 0b111111000000;
    GPIOD_PDDR  = 0b1111001100000000;
}

// calibrate the ADC
// Source: https://community.nxp.com/servlet/JiveServlet/download/339542-1-418948/DMA-examples-for-KDS.zip
int adc_cal(void)
{
    ADC0->SC3 |= ADC_SC3_CAL_MASK;       // Start calibration process
    
    while(ADC0->SC3 & ADC_SC3_CAL_MASK); // Wait for calibration to end
    
    if(ADC0->SC3 & ADC_SC3_CALF_MASK)   // Check for successful calibration
        return 1; 
    
    uint16_t calib = 0; // calibration variable 
    calib += ADC0->CLPS + ADC0->CLP4 + ADC0->CLP3 +
             ADC0->CLP2 + ADC0->CLP1 + ADC0->CLP0;
    calib /= 2;
    calib |= 0x8000;    // Set MSB 
    ADC0->PG = calib;
    calib = 0;
    calib += ADC0->CLMS + ADC0->CLM4 + ADC0->CLM3 +
             ADC0->CLM2 + ADC0->CLM1 + ADC0->CLM0;
    calib /= 2;
    calib |= 0x8000;    // Set MSB
    ADC0->MG = calib;
    
    return 0;
}

void ADC0_Init(void)
{   
    // Activate clocks for ADC
    SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;

    // AVGS 0x3 - 32 samples averaged
    // AVGE 0x1 - Hardware average function enabled
    // ADC0 0x1 - Continuous conversions
    ADC0_SC3 = ADC_SC3_AVGS(0x3) | ADC_SC3_AVGE(0x1) | ADC_SC3_ADCO(0x1);

    // ADICLK 0x1 - (bus clock)/2
    // ADIV   0x3 - (input clock)/8 => clock divider = 8
    // MODE   0x1 - 12-bit conversion
    //        0x2 - 10-bit
    //        0x3 - 16 bit
    ADC0_CFG1 = ADC_CFG1_ADICLK(0x1) | ADC_CFG1_ADIV(0x3) | ADC_CFG1_MODE(0x1) | ADC_CFG1_ADLSMP(0x1);

    adc_cal();

    // ADCH 0x10 - channel 16
    // AIEN 0x1  - Interrupt enable
    // DIFF 0x0  - Single-ended conversions and input channels
    ADC0_SC1A = ADC_SC1_DIFF(0x0) | ADC_SC1_ADCH(0x10) | ADC_SC1_AIEN(0x1);   

    NVIC_EnableIRQ(ADC0_IRQn);
}

// separate digits of given value (works only on the interval 0-999)
// then store them in the 'result' array
void save_value(int value)
{
    if (value < 0) {
        result[0] = '-';
        result[1] = '-';
        result[2] = '-';
        result[3] = '-';
    } else {
        int first_digit = value / 100;
        int second_digit = (value - first_digit*100) / 10;
        int third_digit  = (value - first_digit*100 - second_digit*10);

        result[0] = first_digit + '0';
        result[1] = second_digit + '0';
        result[2] = third_digit + '0';
    }
}

int number; // variable for saving the raw value of ADC
// on interrupt save a raw ADC value
void ADC0_IRQHandler(void) {
	number  = (ADC0_RA);
}

// detect heartbeats
int maxValue = 0;    // current maximum value measured
bool isPeak = false; // peak detected
bool heartbeatDetected(int delay)
{
    int rawValue = 0;
    bool result = false;
    rawValue = number;
    rawValue *= (10000/delay);

    // If sensor shifts, then max is out of whack.
    // Just reset max to a new baseline.
    if (rawValue * 4L < maxValue) {
        maxValue = rawValue * 0.8;
    }
    // Detect new peak
    if (rawValue > maxValue - (1000/delay)) {
        if (rawValue > maxValue) {
            maxValue = rawValue;
        }
        // Only return true once per peak.
        if (isPeak == false) {
            result = true;
        }
        isPeak = true;
    } else if (rawValue < maxValue - (3000/delay)) {
          isPeak = false;
          // Decay max value to adjust to sensor shifting
          // Note that it may take a few seconds to re-detect
          // the signal.
          maxValue-=(1000/delay);
    }
    return result;
}

// set number (0-9) to display
// Digit (1-4 -> SET_D1 - SET_D4) has to be set separately
// Changes index variable
void DisplayNumber(char *valStr)
{
    if (index >= strlen(valStr) || index > 4) {
        SetNOT();
        return;
    }
    if (isdigit(valStr[index])) {
        SetNumber[valStr[index]-'0']();
        index++;
        if (valStr[index] == '.' || valStr[index] == ',') {
            SetDP();
            index++;
        }
    } else {
        SetERR();
        index++;
    }
}

// display value (0-9) on the given digit (PTA port number)
void DisplayDigit(char *result, int digit)
{
    DisplayNumber(result); // set number (0-9) given by index in 'result'
    DigitON(digit);        // set digit to display that number
    delay(DELAY_LED);      // let the LED emit light
    DigitOFF(digit);       // turn the LED off
}

// display value (0-999) on display
void DisplayValue(char *result)
{
    DisplayDigit(result, D1);
    DisplayDigit(result, D2);
    DisplayDigit(result, D3);
    DisplayDigit(result, D4); 
}

int main(void)
{
    MCUInit();
    PortsInit();
    ADC0_Init();

    int pressed = 0;     // indicates button press to switch between measuring and display
    int delayMsec = 60;  // delay between two samples (in miliseconds)
    double alpha = 0.75; // average ratio variable
    double oldValue = 0; // previous average value 
    int beatMsec = 0;

    save_value(0); // set value to be displayed to 0
    while(1) {

       // display BPM on display
       // on startup display 0
        while(1) {
            index = 0;
            // Display value, changes index variable
            DisplayValue(result);

            // on SW6 button press, initiate new measuring by turning LEDs off
            if (!pressed && !(GPIOE_PDIR & BTN_SW6)) {
                pressed = 1;
                break;
            } else if (GPIOE_PDIR & BTN_SW6) {
                pressed = 0;
            }
        }
        // measure BPM, LEDs are off
        int countdown = 30000;
        while(1) {
            double value = alpha * oldValue + (1 - alpha) * number;

            oldValue = value;
            int heartRateBPM = 0;

            if (heartbeatDetected(delayMsec)) {
                heartRateBPM = 60000 / beatMsec; // compute BPM from beats per ms (beatMsec)
                save_value(heartRateBPM);
                beatMsec = 0; // restart measuring
            }

            time_delay_ms(delayMsec);
            beatMsec += delayMsec;
            countdown -= delayMsec;
            if (countdown <= 0) break;

            // on SW6 button press leave measuring
            if (!pressed && !(GPIOE_PDIR & BTN_SW6)) {
                pressed = 1;
                break;
            } else if (GPIOE_PDIR & BTN_SW6) {
                pressed = 0;
            }
        }
    }
    return 0;
}
