#include <MeMCore.h>

MeRGBLed rgb(0,16);
MeBuzzer buzzer;
MeIR ir;
MeDCMotor MotorL(M1);
MeDCMotor MotorR(M2);
MeUltrasonicSensor ultr(PORT_3);
MeLineFollower line(PORT_2);
int16_t moveSpeed = 150;

#define DIRECTION int
#define DIR_FORWARD 0
#define DIR_LEFT 1
#define DIR_RIGHT 2

#define ACTIONTIME 100,moveSpeed*3

class Action;

struct State{
  Action *nextAction;
  Action *previousAction;
  uint8_t depth;


  void update(){
      depth = ultr.distanceCm(100);
      //uint8_t lineStatus = line.readSensors();
  }
};

class Action{
  public:
    Action(MeDCMotor *motorL, MeDCMotor *motorR){
      m_motorL = motorL;
      m_motorR = motorR;
    }

    int pdf(){
      return m_pdf;
    }

    virtual DIRECTION direction(){};

    virtual void perform(){};

    virtual void updatePdf(State *state){};

  protected:
    MeDCMotor *m_motorL;
    MeDCMotor *m_motorR;
    int m_pdf;
};

class ActionForward : public Action{
  public:
    ActionForward(MeDCMotor *motorL, MeDCMotor *motorR)
    : Action(motorL, motorR) { 
          Serial.println("Initializing ActionForward");
      };
      
    void perform(){
      m_motorL->run(-moveSpeed);
      m_motorR->run(moveSpeed);
      delay(random(ACTIONTIME));
    }

    void updatePdf(State *state){
      m_pdf = max(1, state->depth/5);
      Serial.println("Forward pdf: ");
      Serial.println(m_pdf);
    }

    DIRECTION direction(){
      return DIR_FORWARD;
    }
};

class ActionBackward : public Action{
  public:
    ActionBackward(MeDCMotor *motorL, MeDCMotor *motorR)
    : Action(motorL, motorR) { 
          Serial.println("Initializing ActionBackward");
      };
      
    void perform(){
      m_motorL->run(moveSpeed);
      m_motorR->run(-moveSpeed);
      delay(random(ACTIONTIME));
    }

    void updatePdf(State *state){
      m_pdf = max(1, 2-state->depth/10);
      Serial.println("Backward pdf: ");
      Serial.println(m_pdf);
    }

    DIRECTION direction(){
      return DIR_FORWARD;
    }
};

class ActionRotateLeft : public Action{
  public:
    ActionRotateLeft(MeDCMotor *motorL, MeDCMotor *motorR)
    : Action(motorL, motorR) { 
      Serial.println("Initializing ActionRotateLeft");
    };

    void perform(){
      m_motorL->run(moveSpeed);
      m_motorR->run(moveSpeed);
      delay(random(ACTIONTIME));
    }

    void updatePdf(State *state){
      if (state->previousAction && state->previousAction->direction() == DIR_RIGHT){
        m_pdf = 1;
      } else {
        m_pdf = max(1, 10-state->depth/10);
      }
      Serial.println("Left pdf: ");
      Serial.println(m_pdf);
    }

    DIRECTION direction(){
      return DIR_LEFT;
    }

};

class ActionRotateRight : public Action{
  public:
      ActionRotateRight(MeDCMotor *motorL, MeDCMotor *motorR)
    : Action(motorL, motorR) { 
      Serial.println("Initializing ActionRotateRight");
      };

    void perform(){
      m_motorL->run(-moveSpeed);
      m_motorR->run(-moveSpeed);
      delay(random(ACTIONTIME));
    }

    void updatePdf(State *state){
      if (state->previousAction && state->previousAction->direction() == DIR_LEFT){
        m_pdf = 1;
      } else {
        m_pdf = max(1, 10-state->depth/10);
      }
      Serial.println("Right pdf: ");
      Serial.println(m_pdf);
    }

    DIRECTION direction(){
      return DIR_RIGHT;
    }
};

class Brain{
  public:
    Brain(){
      Serial.println("Initializing brain");
      numActions = 4;
      actionPool[0] = new ActionForward(&MotorL, &MotorR);
      actionPool[1] = new ActionBackward(&MotorL, &MotorR);
      actionPool[2] = new ActionRotateLeft(&MotorL, &MotorR);
      actionPool[3] = new ActionRotateRight(&MotorL, &MotorR);
    }
    
    Action *pickAction(State *state){
      Serial.println("Updating PDFs");
      int pdfSum = 0;
      for(int i=0; i<numActions; ++i){
        actionPool[i]->updatePdf(state);
        pdfSum += actionPool[i]->pdf();
      }

      int randnum = random(pdfSum);
      Serial.println("randum num: ");
      Serial.println(randnum);

      int accumPdf = 0;
      Serial.println("Picking action.");
      for(int i=0; i<numActions; ++i){
        accumPdf += actionPool[i]->pdf();
        if (accumPdf >= randnum){
          return actionPool[i];
        }
      }
      Serial.println("Ooops, found no action.");
    }
  private:  
    Action *actionPool[4];
    int numActions;
};

State *state;
Brain *brain;

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

  brain = new Brain();
  state = new State();
  state->nextAction = NULL;
  state->previousAction = NULL;

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
  Serial.println("New loop iter");
  if (ir.decode())
  {
    Serial.println("reading IR");
    readIR();
  }
  //delay(200); // not sure why I need this, but ir sensor doesn't seem to like reading too frequently

  Serial.println("updating state");
  state->update();
  Serial.println("picking action");
  state->nextAction = brain->pickAction(state);
  Serial.println("performing action");
  state->nextAction->perform();
  Serial.println("Action performed");
  state->previousAction = state->nextAction;
}
