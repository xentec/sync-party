#ifndef __AVR_ATmega328P__
#define __AVR_ATmega328P__ // IDE helper
#endif

#include "proto-def.hpp"

#include <Arduino.h>
#include <avr/wdt.h>

#define FW_VERSION 3

#define MOTOR_PIN 11
#define PING_TIMEOUT_MS 200
#define PING_PIN 7
#define US_TIMEOUT_MIS 2000


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
  unsigned long dur = pulseIn(pin, HIGH, US_TIMEOUT_MIS); // 18500

  return dur*100 / 29 / 2;
}

// Motor control
//###############
namespace motor
{
	byte ctrl = proto::Speed::STOP;

	void init()
	{
		pinMode(MOTOR_PIN, OUTPUT);
		pinMode(LED_BUILTIN, OUTPUT);

		// setup TIMER2
		//enable the compare interrupt, not needed at first time
		//TIMSK2 = (1<<OCIE2A);
		//set the compare register to the stop value
		OCR2A = 0x30;
		TCNT2 = 0x00;
		//set up the TIMER2 FastPWM Mode (WGM22-WGM20) and the clear ~OC2A on compare match, set OC2A at BOTTOM (COM2A1-COM2A0)
		TCCR2A = (1<<COM2A1) | (0<<COM2A0) | (1<<WGM21) | (1<<WGM20);
		//set the prescaler of TIMER2 to 256
		TCCR2B = (0<<WGM22) | (1<<CS22) | (1<<CS21) | (0<<CS20);
	}

	void set_speed(byte speed)
	{

		if(ctrl != speed)
		{
			ctrl = speed;
			//enable the compare interrupt
			TIMSK2 = (1<<OCIE2A);
		}

		//change LED on the board when the motor is stopped
		digitalWrite(13, speed != proto::Speed::STOP ? HIGH : LOW);
	}

	inline void stop()
	{
		set_speed(proto::Speed::STOP);
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
	void reset(unsigned long new_timeout = PING_TIMEOUT_MS)
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

using namespace proto;

enum State
{
	SYNC, DATA
} state {};


void send(byte type, byte data)
{
	Serial.write(BYTE_SYNC);
	Serial.write(type);
	Serial.write(data);
	Serial.write(BYTE_END);

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
		} while(b != BYTE_SYNC);

		state = State::DATA;
	case State::DATA:
		if(Serial.available() < 3)
			return;

		state = State::SYNC;

		type = Serial.read();
		data = Serial.read();
		if(Serial.read() != BYTE_END)
			return;

		switch(Type(type))
		{
		case Type::PING:
			wd.reset();
			break;
		case Type::MOTOR:
			motor::set_speed(data);
			break;
		case Type::ULTRA_SONIC:
			data = us_distance(data);
			break;
		case Type::ANALOG:
			data = map(analogRead(data), 0, 1023, 0, 255);
			break;
		case Type::VERSION:
			data = FW_VERSION;
			break;
		default:
			type |= ERR_BIT;
			data = Error::INVALID_ARG;
			break;
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
	wdt_enable(WDTO_120MS);
}

void loop()
{
	if(Serial.available())
		comm::handle();

	if(wd.check())
	{
		motor::stop();
//		comm::send(comm::Type::MOTOR, motor::Control::STOP);
	}
	wdt_reset();
}
