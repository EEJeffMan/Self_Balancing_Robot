/*
 * Author: eejeffman
 * Created: 7/31/18
 * 
 * Description: Read inputs from nunchuck and send commands out via RFM69.
 */

#include <Wire.h>

uint8_t outbuf[6];        // nunchuck data

int ledPin = 13;
int cnt = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);     // UART for diagnostics
  Wire.begin();           // I2C for nunchuck
  nunchuck_init();        // Send the initialization handshake
  // SPI init for RFM69
  Serial.println ("Reset");
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

