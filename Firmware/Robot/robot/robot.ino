#include "Wire.h"

#include "I2Cdev.h"
#include "MPU6050.h"
#include "math.h"
//#include <NewPing.h>

/*#define leftMotorPWMPin   6
#define leftMotorDirPin   7
#define rightMotorPWMPin  5
#define rightMotorDirPin  4*/

#define leftMotorFWDPin   3
#define leftMotorREVPin   5
#define rightMotorFWDPin  6
#define rightMotorREVPin  9

//#define TRIGGER_PIN 9
//#define ECHO_PIN 8
//#define MAX_DISTANCE 75

#define Kp  10//40
#define Kd  0//0.05
#define Ki  0//40
#define sampleTime  0.005
#define targetAngle 0//-2.5

MPU6050 mpu;
//NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

int16_t accY, accZ, gyroX;
volatile int motorPower, gyroRate;
volatile float accAngle, gyroAngle, currentAngle, prevAngle=0, error, prevError=0, errorSum=0;
volatile byte count=0;
int distanceCm;

unsigned long millis_time = 0;

void setMotors(int leftMotorSpeed, int rightMotorSpeed) {
  if(leftMotorSpeed >= 0) {
    //analogWrite(leftMotorPWMPin, leftMotorSpeed);
    //digitalWrite(leftMotorDirPin, LOW);
    analogWrite(leftMotorREVPin, 0);
    analogWrite(leftMotorFWDPin, leftMotorSpeed);   
  }
  else {
    analogWrite(leftMotorFWDPin, 0);
    analogWrite(leftMotorREVPin, leftMotorSpeed);
    //analogWrite(leftMotorPWMPin, 255 + leftMotorSpeed);
    //digitalWrite(leftMotorDirPin, HIGH);
  }
  if(rightMotorSpeed >= 0) {
    //analogWrite(rightMotorPWMPin, rightMotorSpeed);
    //digitalWrite(rightMotorDirPin, LOW);
    analogWrite(rightMotorREVPin, 0);
    analogWrite(rightMotorFWDPin, rightMotorSpeed);
  }
  else {
    //analogWrite(rightMotorPWMPin, 255 + rightMotorSpeed);
    //digitalWrite(rightMotorDirPin, HIGH);
    analogWrite(rightMotorFWDPin, 0);
    analogWrite(rightMotorREVPin, rightMotorSpeed);
  }
}

void init_PID() {  
  // initialize Timer1
  cli();          // disable global interrupts
  TCCR1A = 0;     // set entire TCCR1A register to 0
  TCCR1B = 0;     // same for TCCR1B    
  // set compare match register to set sample time 5ms
  OCR1A = 9999;    
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS11 bit for prescaling by 8
  TCCR1B |= (1 << CS11);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei();          // enable global interrupts
}

void setup() {
  // set the motor control and PWM pins to output mode
/*pinMode(leftMotorPWMPin, OUTPUT);
  pinMode(leftMotorDirPin, OUTPUT);
  pinMode(rightMotorPWMPin, OUTPUT);
  pinMode(rightMotorDirPin, OUTPUT);
  // set the status LED to output mode 
  pinMode(13, OUTPUT);*/
  // initialize the MPU6050 and set offset values
  mpu.initialize();
  mpu.setYAccelOffset(328);
  mpu.setZAccelOffset(1513);
  mpu.setXGyroOffset(80);
  // initialize PID sampling loop
  init_PID();

  Serial.begin(9600);

  millis_time = millis();
}

void loop() {

unsigned long temp_time;
  
  // read acceleration and gyroscope values
  accY = mpu.getAccelerationY();
  accZ = mpu.getAccelerationZ();  
  gyroX = mpu.getRotationX();
  // set motor power after constraining it
  
  //motorPower = constrain(motorPower, -255, 255);
  motorPower = 255;
  
  //setMotors(motorPower, motorPower);
  setMotors(255, -255);
  
  // measure distance every 100 milliseconds
  /*if((count%20) == 0){
    distanceCm = sonar.ping_cm();
  }
  if((distanceCm < 20) && (distanceCm != 0)) {
    setMotors(-motorPower, motorPower);
  }*/

  temp_time = millis() - millis_time;

  if (temp_time > 1000)
  {
    millis_time = millis();
    Serial.println(currentAngle);
  }
}
// The ISR will be called every 5 milliseconds
ISR(TIMER1_COMPA_vect)
{
  // calculate the angle of inclination
  accAngle = atan2(accY, accZ)*RAD_TO_DEG;
  gyroRate = map(gyroX, -32768, 32767, -250, 250);
  gyroAngle = (float)gyroRate*sampleTime;  
  currentAngle = 0.9934*(prevAngle + gyroAngle) + 0.0066*(accAngle);
  
  error = currentAngle - targetAngle;
  errorSum = errorSum + error;  
  errorSum = constrain(errorSum, -300, 300);
  //calculate output from P, I and D values
  motorPower = Kp*(error) + Ki*(errorSum)*sampleTime - Kd*(currentAngle-prevAngle)/sampleTime;
  prevAngle = currentAngle;
  // toggle the led on pin13 every second
  count++;
  if(count == 200)  {
    count = 0;
    digitalWrite(13, !digitalRead(13));
  }

  //Serial.println(currentAngle);
}
