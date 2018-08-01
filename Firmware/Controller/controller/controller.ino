/*
 * Author: EEJeffMan
 * Created: 7/31/18
 * 
 * Description: Read inputs from nunchuck and send commands out via RFM69.
 * 
 * nunchuck code adopted from here: http://todbot.com/arduino/sketches/wii_nunchuck_servo/wii_nunchuck_servo.pde
 * 
 * RFM69 code adopted from here: https://github.com/LowPowerLab/RFM69
 * 
// **********************************************************************************
// Copyright Felix Rusu 2018, http://www.LowPowerLab.com/contact
// **********************************************************************************
// License
// **********************************************************************************
// This program is free software; you can redistribute it 
// and/or modify it under the terms of the GNU General    
// Public License as published by the Free Software       
// Foundation; either version 3 of the License, or        
// (at your option) any later version.                    
//                                                        
// This program is distributed in the hope that it will   
// be useful, but WITHOUT ANY WARRANTY; without even the  
// implied warranty of MERCHANTABILITY or FITNESS FOR A   
// PARTICULAR PURPOSE. See the GNU General Public        
// License for more details.                              
//                                                        
// Licence can be viewed at                               
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
 * 
 */

#include <Wire.h>

#include <RFM69.h>         //get it here: https://github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>     //get it here: https://github.com/lowpowerlab/RFM69
#include <RFM69_OTA.h>     //get it here: https://github.com/lowpowerlab/RFM69
#include <SPIFlash.h> //get it here: https://www.github.com/lowpowerlab/spiflash
#include <SPI.h> //included with Arduino IDE (www.arduino.cc)

//****************************************************************************************************************
//**** IMPORTANT RADIO SETTINGS - YOU MUST CHANGE/CONFIGURE TO MATCH YOUR HARDWARE TRANSCEIVER CONFIGURATION! ****
//****************************************************************************************************************
#define GATEWAYID   1
#define NODEID      11
#define NETWORKID   100
//#define FREQUENCY     RF69_433MHZ
//#define FREQUENCY     RF69_868MHZ
#define FREQUENCY       RF69_915MHZ //Match this with the version of your Moteino! (others: RF69_433MHZ, RF69_868MHZ)
#define ENCRYPTKEY      "sampleEncryptKey" //has to be same 16 characters/bytes on all nodes, not more not less!
#define IS_RFM69HW_HCW  //uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW!
//*****************************************************************************************************************************
//#define ENABLE_ATC      //comment out this line to disable AUTO TRANSMISSION CONTROL
#define ATC_RSSI        -75
//*****************************************************************************************************************************
#define RFM69_CS_PIN    1   

#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif

uint8_t outbuf[6];        // nunchuck data

int ledPin = 13;
int cnt = 0;

char sendBuf[32];
byte sendLen;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);     // UART for diagnostics
  Wire.begin();           // I2C for nunchuck
  nunchuck_init();        // Send the initialization handshake
  // SPI init for RFM69
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
#ifdef IS_RFM69HW_HCW
  radio.setHighPower(); //must include this only for RFM69HW/HCW!
#endif
  radio.encrypt(ENCRYPTKEY);

//Auto Transmission Control - dials down transmit power to save battery (-100 is the noise floor, -90 is still pretty good)
//For indoor nodes that are pretty static and at pretty stable temperatures (like a MotionMote) -90dBm is quite safe
//For more variable nodes that can expect to move or experience larger temp drifts a lower margin like -70 to -80 would probably be better
//Always test your ATC mote in the edge cases in your own environment to ensure ATC will perform as you expect
#ifdef ENABLE_ATC
  radio.enableAutoPower(ATC_RSSI);
#endif  

  //radio.sleep();
  pinMode(RFM69_CS_PIN, OUTPUT);
  digitalWrite(RFM69_CS_PIN, LOW);

  Serial.println ("Reset");
  radio.sendWithRetry(GATEWAYID, "RESET", 5);
}

int t = 0;  
void loop() {
  // put your main code here, to run repeatedly:
  t++;
  if( t == 25 ) {
    t = 0;
    Wire.requestFrom (0x52, 6);  // request data from nunchuck
    while (Wire.available ()) {
      // receive byte as an integer
      outbuf[cnt] = nunchuk_decode_byte (Wire.read ());
      digitalWrite (ledPin, HIGH);  // sets the LED on
      cnt++;
    }
    // If we recieved the 6 bytes, then go print them
    if (cnt >= 5) {
      printNunchuckData();            // uncomment this for debug
    }
    cnt = 0;
    send_zero(); // send the request for next bytes
    
  } // if(t==)

//************************************************************************************************
// Example of sending data and going back to sleep with diagnostics on RF data:

    sendLen = strlen(sendBuf);

    if (radio.sendWithRetry(GATEWAYID, sendBuf, sendLen))
    {
      Serial.println("MOTION ACK:OK! RSSI:");
      Serial.println(radio.RSSI);
    }
    else Serial.println("MOTION ACK:NOK...");

  radio.sleep();
  
//************************************************************************************************

  delay(1);
}

void nunchuck_init()
{
  Wire.beginTransmission (0x52);  // transmit to device 0x52
  Wire.write (0x40);   // sends memory address
  Wire.write (0x00);   // sends sent a zero.  
  Wire.endTransmission ();  // stop transmitting
}

int i=0;
void printNunchuckData()
{
  int joy_x_axis = outbuf[0];
  int joy_y_axis = outbuf[1];
  int accel_x_axis = outbuf[2]; // * 2 * 2; 
  int accel_y_axis = outbuf[3]; // * 2 * 2;
  int accel_z_axis = outbuf[4]; // * 2 * 2;

  int z_button = 0;
  int c_button = 0;

  // byte outbuf[5] contains bits for z and c buttons
  // it also contains the least significant bits for the accelerometer data
  // so we have to check each bit of byte outbuf[5]
  if ((outbuf[5] >> 0) & 1) 
    z_button = 1;
  if ((outbuf[5] >> 1) & 1)
    c_button = 1;

  if ((outbuf[5] >> 2) & 1) 
    accel_x_axis += 2;
  if ((outbuf[5] >> 3) & 1)
    accel_x_axis += 1;

  if ((outbuf[5] >> 4) & 1)
    accel_y_axis += 2;
  if ((outbuf[5] >> 5) & 1)
    accel_y_axis += 1;

  if ((outbuf[5] >> 6) & 1)
    accel_z_axis += 2;
  if ((outbuf[5] >> 7) & 1)
    accel_z_axis += 1;

  Serial.print (i,DEC);
  Serial.print ("\t");

  Serial.print (joy_x_axis, DEC);
  Serial.print ("\t");
  Serial.print (joy_y_axis, DEC);
  Serial.print ("\t");

  Serial.print (accel_x_axis, DEC);
  Serial.print ("\t");
  Serial.print (accel_y_axis, DEC);
  Serial.print ("\t");
  Serial.print (accel_z_axis, DEC);
  Serial.print ("\t");

  Serial.print (z_button, DEC);
  Serial.print (" ");
  Serial.print (c_button, DEC);

  Serial.print ("\r\n");
  i++;
}

void send_zero()
{
  Wire.beginTransmission (0x52);  // transmit to device 0x52
  Wire.write (0x00);   // sends one byte
  Wire.endTransmission ();  // stop transmitting
}

// Encode data to format that most wiimote drivers except
// only needed if you use one of the regular wiimote drivers
char nunchuk_decode_byte (char x)
{
  x = (x ^ 0x17) + 0x17;
  return x;
}

// End of file.

