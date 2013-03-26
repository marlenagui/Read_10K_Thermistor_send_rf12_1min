/*
 * Inputs ADC Value from Thermistor and outputs Temperature in Celsius
 *  requires: include <math.h>
 * Utilizes the Steinhart-Hart Thermistor Equation:
 *    Temperature in Kelvin = 1 / {A + B[ln(R)] + C[ln(R)]3}
 *    where A = 0.001129148, B = 0.000234125 and C = 8.76741E-08
 *
 * These coefficients seem to work fairly universally, which is a bit of a 
 * surprise. 
 *
 * Schematic:
 *   [Ground] -- [10k-pad-resistor] -- | -- [thermistor] --[Vcc (5 or 3.3v)]
 *                                     |
 *                                Analog Pin 0
 *
 * In case it isn't obvious (as it wasn't to me until I thought about it), the analog ports
 * measure the voltage between 0v -> Vcc which for an Arduino is a nominal 5v, but for (say) 
 * a JeeNode, is a nominal 3.3v.
 *
 * The resistance calculation uses the ratio of the two resistors, so the voltage
 * specified above is really only required for the debugging that is commented out below
 *
 * Resistance = (1024 * PadResistance/ADC) - PadResistor 
 *
 * I have used this successfully with some CH Pipe Sensors (http://www.atcsemitec.co.uk/pdfdocs/ch.pdf)
 * which be obtained from http://www.rapidonline.co.uk.
 *
 */

#include <math.h>
#include <avr/sleep.h>
#include <JeeLib.h>

Port leds (1);

//Port ldr (3);
MilliTimer readoutTimer, aliveTimer;
byte radioIsOn;
#define ThermistorPIN 2                 // Analog Pin 0

float vcc = 3.3;                        // only used for display purposes, if used
                                        // set to the measured Vcc.
float pad = 10000;                      // balance/pad resistor value, set this to
                                        // the measured resistance of your pad resistor
float thermr = 10000.0;                   // thermistor nominal resistance
float AvgTemp = 0.0;                    //To average the mesured temp, will remove picks

static void sendLed (byte on) {         //to flash red LED when sending
    leds.mode(OUTPUT);
    leds.digiWrite(on);
}
    
float Thermistor(int RawADC) {
  long Resistance;  
  float Temp;  // Dual-Purpose variable to save space.
  
  Resistance=((1024 * pad / RawADC) - pad); 
  Temp = log(Resistance); // Saving the Log(resistance) so not to calculate  it 4 times later
  Temp = 1 / (0.001129148 + (0.000234125 * Temp) + (0.0000000876741 * Temp * Temp * Temp));
  Temp = Temp - 273.15;  // Convert Kelvin to Celsius  
  // Uncomment this line for the function to return Fahrenheit instead.
  //temp = (Temp * 9.0)/ 5.0 + 32.0;                  // Convert to Fahrenheit
  return Temp;                                      // Return the Temperature
}

void setup () {
    // initialize the serial port and the RF12 driver
    Serial.begin(57600);
    Serial.print("\n[Read_10K_Thermistor_send_rf12_1min]\n");
    rf12_config();
    // set up easy transmissions every 5 seconds
    rf12_easyInit(60);
    // enable pull-up on LDR analog input
    //ldr.mode2(INPUT);
    //ldr.digiWrite2(1);
    AvgTemp=Thermistor(analogRead(ThermistorPIN)); //set the Average to 
}

void loop () 
{
    float temp;
     
    // switch to idle mode while waiting for the next event
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_mode();  
    // keep the easy tranmission mechanism going
    if (radioIsOn && rf12_easyPoll() == 0) 
    {
        rf12_sleep(0); // turn the radio off
        radioIsOn = 0;
    }
    // only take a light level measurement once every 60 second
    if (readoutTimer.poll(60000)) 
    {
        // keep track of a running 60-second average
        temp=Thermistor(analogRead(ThermistorPIN));       // read ADC and  convert it to Celsius
        //if ( AvgTemp = 0.0 ) {
         // AvgTemp = temp;
        //} else {
          AvgTemp = (4 * AvgTemp + temp) / 5; 
        //}
        Serial.print("temp : ");
        Serial.print(temp);
        Serial.print("\nAvgTemp : ");
        Serial.print(AvgTemp);
        Serial.print("\n");
        // send measurement data, but only when it changes
        sendLed(1); // LED on
        float sending = rf12_easySend(&AvgTemp, sizeof AvgTemp);
        delay(50);
        sendLed(0); //LED off
        // force a "sign of life" packet out every 60 seconds
        //if (aliveTimer.poll(60000))
            //sending = rf12_easySend(0, 0); // always returns 1
        if (sending) 
        {
            // make sure the radio is on again
            if (!radioIsOn)
                rf12_sleep(-1); // turn the radio back on
            radioIsOn = 1;
        }
    }
}
    
