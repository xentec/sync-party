#include <Arduino.h>

#define MOTOR_PIN 11
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

// Motor control
//###############
namespace motor
{
  enum Control
  {
	BACK_FULL    = 0x10,
	STOP         = 0x30,
	FORWARD_FULL = 0x90,
  };

  byte ctrl = Control::STOP;

  void init()
  {
	pinMode(MOTOR_PIN, OUTPUT);
	pinMode(LED_BUILTIN, OUTPUT);

	// setup TIMER2
	//enable the compare interrupt, not needed at first time
	//TIMSK2 = (1<<OCIE2A);
	//set the compare register to the stop value
	OCR2A = 0x30;
	TCNT2=0x00;
	//set up the TIMER2 FastPWM Mode (WGM22-WGM20) and the clear ~OC2A on compare match, set OC2A at BOTTOM (COM2A1-COM2A0)
	TCCR2A = (1<<COM2A1) | (0<<COM2A0) | (1<<WGM21) | (1<<WGM20);
	//set the prescaler of TIMER2 to 256
	TCCR2B = (0<<WGM22) | (1<<CS22) | (1<<CS21) | (0<<CS20);
  }

  void set_speed(byte speed)
  {
	//0x10 full reverse
	//0x30 stop
	//0x90 full forward

	if(speed < 0x10) speed = 0x10;
	else if(speed > 0x90) speed = 0x90;

	//change LED on the board when the motor is stopped
	digitalWrite(13, speed != Control::STOP ? HIGH : LOW);

	if(ctrl!=speed){
		ctrl=speed;
		//enable the compare interrupt
		TIMSK2=(1<<OCIE2A);
	}
  }

  inline void stop()
  {
	set_speed(Control::STOP);
  }
};

ISR(TIMER2_COMPA_vect)
{
  //set the compare register to the new value
  OCR2A=motor::ctrl;
  //disable the compare interrupt
  TIMSK2 &= ~(1<<OCIE2A);
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
  // [<type><param>]

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

	wd.reset();

	switch(type)
	{
	case Type::PING:
	  wd.reset();
	  return;
	case Type::MOTOR:
	  motor::set_speed(data);
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
  motor::init();
  wd.reset();
  Serial.begin(115200);
}

void loop()
{
  if(Serial.available())
	comm::handle();

  if(wd.check())
  {
	comm::send(comm::Type::MOTOR, motor::Control::STOP);
	motor::stop();
  }
}
