#include "asio.hpp"
#include "types.hpp"
#include "util.hpp"

#include "controller.hpp"
#include "driver.hpp"
#include "steering.hpp"

#include <fmt/format.h>
//MQTT include
#include <stdio.h>
#include <mosquitto.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define PORT 4444


void mosq_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
	/* Pring all log messages regardless of level. */
  
  switch(level){
    //case MOSQ_LOG_DEBUG:
    //case MOSQ_LOG_INFO:
    //case MOSQ_LOG_NOTICE:
    case MOSQ_LOG_WARNING:
    case MOSQ_LOG_ERR: {
      printf("%i:%s\n", level, str);
    }
  }
  
	
}

struct mosquitto *mosq = NULL;
char *topic = NULL;
void mqtt_setup(){

	char *host = "localhost";
	int port = PORT;
	int keepalive = 60;
	bool clean_session = true;
  topic = "/motor";
  
  mosquitto_lib_init();
  mosq = mosquitto_new(NULL, clean_session, NULL);
  if(!mosq){
		fprintf(stderr, "Error: Out of memory.\n");
		exit(1);
	}
  
  mosquitto_log_callback_set(mosq, mosq_log_callback);
  
  if(mosquitto_connect(mosq, host, port, keepalive)){
		fprintf(stderr, "Unable to connect.\n");
		exit(1);
	}
  int loop = mosquitto_loop_start(mosq);
  if(loop != MOSQ_ERR_SUCCESS){
    fprintf(stderr, "Unable to start loop: %i\n", loop);
    exit(1);
  }
}

int mqtt_send(char *msg){
  return mosquitto_publish(mosq, NULL, topic, strlen(msg), msg, 0, 0);
}

int main()
{
	//MQTT
	mqtt_setup();
	
	io_context ioctx;

	Controller ctrl(ioctx, "/dev/input/js0");
	//Driver driver(ioctx, "/dev/ttyACM0");
	//Steering steering(ioctx);

	ctrl.on_axis = [&](u32, Controller::Axis num, i16 val)
	{
		static std::unordered_map<Controller::Axis, i16> input_state
		{
			{ Controller::LS_H, 0 },
			{ Controller::LT2, Controller::min },
			{ Controller::RT2, Controller::min },
		};

		constexpr i32 axis_min = Controller::min, axis_max = Controller::max;

		auto i = input_state.find(num);
		if(i == input_state.end())
			return;

		i->second = val;

		// motor control
		//###############
		i32 input = input_state[Controller::RT2] - input_state[Controller::LT2];
		static i32 motor_input_prev = 0;
		if(motor_input_prev != input)
		{
			motor_input_prev = input;

			u8 speed = Driver::Speed::STOP;
			if(input < 0)
				speed = map<u8>(input, 0, axis_min*2, Driver::Speed::STOP, Driver::Speed::BACK_FULL);
			else
				speed = map<u8>(input, 0, axis_max*2, Driver::Speed::STOP, Driver::Speed::FORWARD_FULL);

			static u8 speed_prev = Driver::Speed::STOP;
			if(speed != speed_prev)
			{
				speed_prev = speed;
				//fmt::print("MOTOR: {:5} => {:02x}\n", input, speed);
				//publish MQTT speed
				mosquitto_publish(mosq,NULL,"/controller/motor",sizeof(u8),speed,0,0);
				//driver.drive(speed);
			}
		}

		// steer control
		//###############
		static i16 steer_input_prev = 0;
		i16 &steer = input_state[Controller::LS_H];
		if(steer_input_prev != steer)
		{
			steer_input_prev = steer;
			//publish MQTT steering
			mosquitto_publish(mosq,NULL,"/controller/steering",strlen(steer),steer,0,0);
			//steering.steer(steer);
		}
	};

	ioctx.run();

	return 0;
}
