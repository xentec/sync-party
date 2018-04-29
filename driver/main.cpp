#include <Arduino.h>

#define WD_TIMEOUT_MS 200
#define PING_PIN 7
#define PING_TIMEOUT_MIS 2000


// UltraSonic 
//############
unsigned us_distance(int pin = PING_PIN)
{
  // start
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delayMicroseconds(2);
  digitalWrite(pin, HIGH);
  delayMicroseconds(5);
  digitalWrite(pin, LOW);

  // wait for echo
  pinMode(pin, INPUT);
  unsigned long dur = pulseIn(pin, HIGH, PING_TIMEOUT_MIS); // 18500

  return dur*100 / 29 / 2;
}

// Watchdog
//##########
struct Watchdog
{
  void reset(unsigned long new_timeout = WD_TIMEOUT_MS)
  {
    timeout = millis() + new_timeout;
  }
  bool check() 
  {
    bool trigger = millis() > timeout;
    if(trigger) 
      timeout = -1; // never trigger until reset()
    return trigger; 
  }
private:
  unsigned long timeout;
};

Watchdog wd;

// Serial communication
//######################
namespace comm
{
enum {
  SYNC_BYTE = '[',
};

enum Type
{
  PING  = 0x0,
  MOTOR = 0x1,
  GAP   = 0x2,

  ERR = 0xFF
};

enum Error
{
  INVALID_ARG = 0x0,
};

enum State
{
  SYNC, DATA
} state {};


void send(byte type, byte data)
{
  Serial.write(SYNC_BYTE);
  Serial.write(type);
  Serial.write(data);
  Serial.write(']');
//  Serial.flush();
}

void handle()
{
  // 5D<cmd><param>
 
  byte type, data;
  int b;
  switch(state)
  {
  case State::SYNC:
    do { 
      b = Serial.read();
      if(b == -1)
        return;
    } while(b != SYNC_BYTE);
    state = State::DATA;

  case State::DATA:
    if(Serial.available() < 2) 
      return;

    state = State::SYNC;
    
    type = Serial.read();
    data = Serial.read();

    switch(type)
    {
    case Type::PING:
      wd.reset();
      return;
    case Type::MOTOR:
      // TODO: control motor
      break;
    case Type::GAP: 
      data = us_distance();
      break;
    default:
      type = Type::ERR;
      data = Error::INVALID_ARG;
    }

    send(type, data);
    break;
  default: break;
  }
}

}

// Main logic
//############
void setup()
{
  wd.reset();
  Serial.begin(9600);
}

void loop()
{
  if(Serial.available()) 
    comm::handle();

  if(wd.check())
  {
    comm::send(comm::Type::PING, 0xAA);
    wd.reset();
    // TODO: block motor
  }
}
