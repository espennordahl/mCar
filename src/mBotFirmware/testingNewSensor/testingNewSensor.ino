#include <MeMCore.h>


MeRGBLed rgb(0,16);
MeBuzzer buzzer;
MeIR ir;
MeDCMotor MotorL(M1);
MeDCMotor MotorR(M2);
MeUltrasonicSensor ultr(PORT_4);
#define trigPin A2
#define echoPin A3


void setup() {
  // put your setup code here, to run once:
  // Pin stuff
  pinMode(13,OUTPUT);
  pinMode(7,INPUT);
  digitalWrite(13,HIGH);
  delay(300);
  digitalWrite(13,LOW);
  rgb.setpin(13);
  ir.begin(); 
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);


  // Blink and play sounds to show we're alive
  rgb.setColor(0,0,0);
  rgb.show();
  rgb.setColor(10, 0, 0);
  rgb.show();
//  buzzer.tone(350, 300); 
  delay(300);
  rgb.setColor(0, 10, 0);
  rgb.show();
//  buzzer.tone(294, 300);
  delay(300);
  rgb.setColor(0, 0, 10);
  rgb.show();
  buzzer.tone(350, 300);
  delay(300);
  rgb.setColor(10,10,10);
  rgb.show();
  buzzer.noTone();
}

void loop() {
  // put your main code here, to run repeatedly:

  delay(500);
  int depthA = ultr.distanceCm(100);
  Serial.println("Old sensor:");
  Serial.println(depthA);
  
    long duration, distance;
  digitalWrite(trigPin, LOW);  // Added this line
  delayMicroseconds(2); // Added this line
  digitalWrite(trigPin, HIGH);
//  delayMicroseconds(1000); - Removed this line
  delayMicroseconds(10); // Added this line
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration/2) / 29.1;

  Serial.println("New sensor:");
  Serial.println(distance);
}
