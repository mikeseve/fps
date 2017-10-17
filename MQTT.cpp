#include "MQTT.h"
#include <iostream>
#include <unistd.h>

#include <stdio.h>
#include <string>
using std::string;
using std::cout;
using std::cerr;
using std::endl;

queue<string> MQTT::messages;
void MQTT::message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	//printf("got message '%.*s' for topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);
	string s((char*) message->payload);
	messages.push(s);
}

MQTT::MQTT(string hostname_, int port_, string username_, string password_){
    mosquitto_lib_init();
    mosq = mosquitto_new (NULL, true, NULL);
    if(!mosq){
        cerr << "Can't initalize mosquitto lib" << endl;
   }
    mosquitto_message_callback_set(mosq, message_callback);
    mosquitto_username_pw_set (mosq, &username_[0], &password_[0]);
    int ret = mosquitto_connect (mosq, &hostname_[0], port_, 60);
    if(ret){
        cerr << "Can't connect to mosquitto server" << endl;
    }
    else{
        hostname = hostname_;
        port = port_;
        username = username_;
        password = password_;
    }
}

bool MQTT::publish(string topic, string message){
    int ret = mosquitto_publish (mosq, NULL, &topic[0],message.size(), &message[0], 0, false);
    if  (ret){
        cerr << "Can't publish to mosquitto server" << endl;
        return false;
    }
    return true;
}

void MQTT::cleanup(){
    sleep (1);
    // Clean up Mosquitto
    mosquitto_disconnect (mosq);
    mosquitto_destroy (mosq);
    mosquitto_lib_cleanup();
}

void MQTT::subscribe(string topic){
    //mosquitto_message_callback_set(mosq, message_callback);
    mosquitto_subscribe (mosq, NULL, &topic[0], 0);
}
void MQTT::refresh(){
    	int rc = mosquitto_loop(mosq, -1,1);
	if(rc){
		printf("connection error!\n");
		sleep(10);
		mosquitto_reconnect(mosq);
	}
}
string MQTT::getMessage(){
    string newMessage = MQTT::messages.front();
    MQTT::messages.pop();
    return newMessage;
}
bool MQTT::empty(){
    return MQTT::messages.empty();
}
