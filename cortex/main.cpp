/*
  The mqqt task which gets data from  broker and sends it to arduino
  needs libmosquitto-dev
  $ gcc -o libmosq libmosq.c -lmosquitto
*/
#include <stdio.h>
#include <mosquitto.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "driver.hpp"
#include "steering.hpp"
#include "types.hpp"


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
	int port = 4444;
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

void my_message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
	if(message->payloadlen){
		
		if(strcmp(message->topic , "/controller/motor")!=0) driver.drive(int(message->payload));
		if(strcmp(message->topic, "/controller/steering")!=0) steering.steer(i8(message->payload));
		//printf("%s %s\n", message->topic, message->payload);
	}else{
		printf("%s (null)\n", message->topic);
	}
		fflush(stdout);
}

 io_context ioctx;
 Driver driver(ioctx, "/dev/ttyACM0");
 Steering steering(ioctx);

int main(int argc, char *argv[])
{
  		
  mqtt_setup();
	
	
  
  int subs0 = mosquitto_subscribe(mosq,NULL,"/controller/motor",0);
  int subs1 = mosquitto_subscribe(mosq,NULL,"/controller/steering",0);
  
  while(1){

    mosquitto_message_callback_set(mosq, my_message_callback);
  }
}
