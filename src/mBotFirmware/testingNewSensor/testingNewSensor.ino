#include <MeMCore.h>


MeRGBLed rgb(0,16);
MeBuzzer buzzer;
MeIR ir;
MeUltrasonicSensor ultr(PORT_4);
#define trigPinLeft A2
#define echoPinLeft A3
#define trigPinRight 9
#define echoPinRight 10


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

  pinMode(trigPinLeft, OUTPUT);
  pinMode(echoPinLeft, INPUT);
  pinMode(trigPinRight, OUTPUT);
  pinMode(echoPinRight, INPUT);


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
  digitalWrite(trigPinLeft, LOW); 
  delayMicroseconds(2);
  digitalWrite(trigPinLeft, HIGH);
  delayMicroseconds(10); 
  digitalWrite(trigPinLeft, LOW);
  duration = pulseIn(echoPinLeft, HIGH);
  distance = (duration/2) / 29.1;

  Serial.println("New sensor left:");
  Serial.println(distance);


  digitalWrite(trigPinRight, LOW);
  delayMicroseconds(2); 
  digitalWrite(trigPinRight, HIGH);
  delayMicroseconds(10); 
  digitalWrite(trigPinRight, LOW);
  duration = pulseIn(echoPinRight, HIGH);
  distance = (duration/2) / 29.1;


  Serial.println("New sensor right:");
  Serial.println(distance);
}
