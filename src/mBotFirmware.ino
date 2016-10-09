#include <MeMCore.h>
MeRGBLed rgb(0,16);
MeBuzzer buzzer;
MeIR ir;
MeDCMotor MotorL(M1);
MeDCMotor MotorR(M2);
MeUltrasonicSensor ultr(PORT_3);
MeLineFollower line(PORT_2);

#define DIR_FORWARD 0
#define DIR_LEFT 1
#define DIR_RIGHT 2

int16_t moveSpeed = 150;
int direction = DIR_FORWARD;

void setup() {
  // Pin stuff
  pinMode(13,OUTPUT);
  pinMode(7,INPUT);
  digitalWrite(13,HIGH);
  delay(300);
  digitalWrite(13,LOW);
  rgb.setpin(13);
  ir.begin(); 
  Serial.begin(115200);

  // Blink and play sounds to show we're alive
  rgb.setColor(0,0,0);
  rgb.show();
  rgb.setColor(10, 0, 0);
  rgb.show();
  buzzer.tone(350, 300); 
  delay(300);
  rgb.setColor(0, 10, 0);
  rgb.show();
  buzzer.tone(294, 300);
  delay(300);
  rgb.setColor(0, 0, 10);
  rgb.show();
  buzzer.tone(350, 300);
  delay(300);
  rgb.setColor(10,10,10);
  rgb.show();
  buzzer.noTone();
}

void driveForward()
{
  MotorL.run(-moveSpeed);
  MotorR.run(moveSpeed);
}

void driveBackward()
{
  MotorL.run(moveSpeed); 
  MotorR.run(-moveSpeed);
}

void turnLeft()
{
  MotorL.run(moveSpeed);
  MotorR.run(moveSpeed);
}

void turnRight()
{
  MotorL.run(-moveSpeed);
  MotorR.run(-moveSpeed);
}

void turn(int direction){
  if(direction == DIR_LEFT){
    turnLeft();
  } else {
    turnRight();
  }
}

void driveStop()
{
  MotorL.run(0);
  MotorR.run(0);
}

void readIR(){
      uint32_t value = ir.value;
    switch (value >> 16 & 0xff)
    {
      case IR_BUTTON_3:
        rgb.setColor(10,0,0);
        break;
      case IR_BUTTON_2:
        rgb.setColor(0,10,0);
        break;
      case IR_BUTTON_1:
        rgb.setColor(0,0,10);
        break;
      case IR_BUTTON_PLUS:
        if (moveSpeed < 250){
          moveSpeed += 50;
        }
        break;
      case IR_BUTTON_MINUS:
        if (moveSpeed){
          moveSpeed -= 50;
        }
        break;
    }
}

void loop() {
  if (ir.decode())
  {
    readIR();
  }
  delay(200); // not sure why I need this, but ir sensor doesn't seem to like reading too frequently
  
  uint8_t dist = ultr.distanceCm(70);
  uint8_t lineStatus = line.readSensors();

  if (dist > 30){// && lineStatus != S1_OUT_S2_OUT){
    direction = DIR_FORWARD;
    rgb.setColor(0,0,10);
    driveForward();
  } else {
    rgb.setColor(10,0,0);
    if(direction == DIR_FORWARD){
      if(millis() % 2){
        direction = DIR_RIGHT;
      } else {
        direction = DIR_LEFT;
      }
    }
    turn(direction);
    if (dist < 20){
      delay(200);
    } else {
      delay(500);
    }
  }
  rgb.show();
}
