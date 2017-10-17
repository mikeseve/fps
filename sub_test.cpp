#include <stdio.h>
#include <string>
#include <mosquitto.h>
#include <unistd.h>
using std::string;

#define MQTT_HOSTNAME "futurehaus.cs.vt.edu"
#define MQTT_PORT 1883
#define MQTT_USERNAME "futurehaus"
#define MQTT_PASSWORD "HokieDVE"
#define MQTT_TOPIC "doorTest"

void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	printf("got message '%.*s' for topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);
}

int main(int argc, char *argv[]){
	uint8_t reconnect = true;
	char clientid[24];
	struct mosquitto *mosq;
	int rc = 0;
	
	mosquitto_lib_init();
	mosq = mosquitto_new (NULL, true, NULL);

	if(mosq){
		mosquitto_message_callback_set(mosq, message_callback);
	
		 
	  	mosquitto_username_pw_set (mosq, MQTT_USERNAME, MQTT_PASSWORD);
	
	  	// Establish a connection to the MQTT server. Do not use a keep-alive ping
  		int ret = mosquitto_connect (mosq, MQTT_HOSTNAME, MQTT_PORT, 60);
		mosquitto_subscribe(mosq, NULL, "doorTest", 0);
		
		while(1){
			rc = mosquitto_loop(mosq, -1, 1);
			if(rc){
				printf("connection error!\n");
				sleep(10);
				mosquitto_reconnect(mosq);
			}
		}
		mosquitto_destroy(mosq);
	}
	mosquitto_lib_cleanup();
	return 0;


}