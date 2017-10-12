#include <wiringPi.h>
#include <wiringSerial.h>
#include <iostream>
#include "FPS.h"
#include <queue>
#include <mosquitto.h>
#include <string>
#include <unistd.h>

using std::string;
using std::queue;
using std::cout;
using std::cerr;
using std::cin;
using std::endl;
//Defines
#define BAUDRATE 9600
#define DEFAULTSTATE IDENTIFY

// Server connection parameters
#define MQTT_HOSTNAME "futurehaus.cs.vt.edu"
#define MQTT_PORT 1883
#define MQTT_USERNAME "futurehaus"
#define MQTT_PASSWORD "HokieDVE"
#define MQTT_TOPIC "doorTest"


enum Inbox{MQTT, SCANNER, MAIN};//Used to determine which thread a message is for. Also used to piLock and piUnlock the correct variables.
enum FPSCommandStates{IDLE, ENROLL, IDENTIFY, VERIFY, DELETEALL, DELETEID, ISPRESSFINGER, CLOSE}; //List of command states the FPS can perform
enum MessageTypes{COMMAND, RESPONSE}; //Used to tell the type of a message.

//Message:
//Structure used to organize communication between threads.
struct Message{
	MessageTypes messageType;
	FPSCommandStates command; //Command to be performed if messageType is COMMAND.
	int args; //Additional information (such as ID to be verified).
	Response response; //Information given by sensor after executing a command if messageType is RESPONSE
};
//Queues are used to hold messages for each thread.
queue<Message> MQTTInbox;
queue<Message> FPSInbox;
queue<Message> MainInbox;

//sendCmdMsg:
//sends a command type message containing the specified command and args to the inbox given.
void sendCmdMsg(Inbox messageDestination, FPSCommandStates command, int args = -1){
	Message newMessage;
	newMessage.messageType = COMMAND;
	newMessage.command = command;
	newMessage.args = args;
	piLock((int)messageDestination);
	if(messageDestination == SCANNER) FPSInbox.push(newMessage);
	else if(messageDestination == MQTT) MQTTInbox.push(newMessage);
	else MainInbox.push(newMessage);
	piUnlock((int)messageDestination);
}

//sendRspMsg:
//sends a response type message containing the specified response and args to the inbox given.
void sendRspMsg(Inbox messageDestination, Response response, int args = -1){
	Message newMessage;
	newMessage.messageType = RESPONSE;
	newMessage.response = response;
	newMessage.args = args;
	piLock(messageDestination);
	if(messageDestination == SCANNER) FPSInbox.push(newMessage);
	else if(messageDestination == MQTT) MQTTInbox.push(newMessage);
	else MainInbox.push(newMessage);
	piUnlock(messageDestination);
}

//checkMail:
//returns false if the queue of the inbox specified is empty, true otherwise.
bool checkMail(Inbox inbox){
		bool returnValue = true;
		piLock((int)inbox);

		if(inbox == MQTT && MQTTInbox.empty()) returnValue = false;
		else if(inbox == SCANNER && FPSInbox.empty()) returnValue = false;
		else if(inbox == MAIN && MainInbox.empty()) returnValue = false;

		piUnlock((int)inbox);
		return returnValue;
}

//getMessage:
//returns the front-most message in the inbox's queue. returns an empty message if the inbox's queue is empty.
Message getMessage(Inbox inbox){
	Message newMessage;
	if(checkMail(inbox)){
		piLock(inbox);
		if(inbox == SCANNER){
			newMessage = FPSInbox.front();
			FPSInbox.pop();
		}
		else if(inbox == MQTT){
				newMessage = MQTTInbox.front();
				MQTTInbox.pop();
		}
		else{
			newMessage = MainInbox.front();
			MainInbox.pop();
		}
		piUnlock(inbox);
	}
	return newMessage;
}

PI_THREAD (FPScanner){
	//Variables used to prevent a single finger press from being treated multiple times.
	enum fingerStates{PRESSED, RELEASED} fingerState = RELEASED;
	bool newFingerPressed = false;

	int fd; //Id returned from serial Open. Used to create new FPS object.
	FPS *fps = new FPS(); //Object representing the Scanner.

	bool loop = true; //loop variable for main while loop.

	Response *returnedResponse; //Used to get responses from commands.

	FPSCommandStates currentCommand = DEFAULTSTATE; //Holds the current command to be executed by the scanner.
	int args; //Holds arguments fed into the various commands.

	//Open a new serial device and create the finger print scanner.
	if((fd = serialOpen("/dev/serial0",BAUDRATE)) < 0){
		cerr << "An error occured: Couldn't open serial device";
	}
	else{
		fps->setFD(fd);
		(fps->OpenFPS(false));
	}

	fps->ToggleLED(true);
	while(loop){

		//Check for new mail and get new commands/arguments if available.
		if(checkMail(SCANNER)){
			Message newMessage = getMessage(SCANNER);
			currentCommand = newMessage.command;
			args = newMessage.args;
		}

		//State machine that determines if a new finger is pressed on the scanner.
		switch(fingerState){
			case RELEASED:
				if(fps->IsFingerPressed()){
					newFingerPressed = true;
					fingerState = PRESSED;
				}
			break;
			case PRESSED:
				if(!(fps->IsFingerPressed())) fingerState = RELEASED;
			break;
		}

		//Main switch statement representing the the different possible commands.
		switch(currentCommand){
			case IDLE:
				//Do nothing. Available just to be used as an option for DEFAULTSTATE.
				delay(100);
				break;
			case ENROLL:
				//Enroll the next available ID.
				args = 0;
				fps->BlinkLED(250);
				while(fps->CheckEnrolledID(args)) args++;
				returnedResponse = fps->Enroll(args);
				sendRspMsg(MQTT, *returnedResponse);
				currentCommand = DEFAULTSTATE;
				break;
			case IDENTIFY:
				//Identify The finger on the scanner.
				if(!newFingerPressed) break;
				else{
					newFingerPressed = false; //Used with fingerState statemachine above to keep the scanner from constantly identifying a finger that was only placed once.
					returnedResponse = fps->captureFinger(false);
					if(returnedResponse->failed) sendRspMsg(MQTT, *returnedResponse);
					else{
						returnedResponse = fps->Identify();
						sendRspMsg(MQTT, *returnedResponse);
					}
					currentCommand = DEFAULTSTATE;
				}
				break;
			case VERIFY:
				//Verify the finger on the scanner with the id supplied.
				fps->BlinkLED(250);
				if(!fps->IsFingerPressed()) break;
				else{
					returnedResponse = fps->captureFinger(true);
					if(returnedResponse->failed) sendRspMsg(MQTT,*returnedResponse);
					else{
						returnedResponse = fps->Verify(args);
						sendRspMsg(MQTT, *returnedResponse);
					}
					currentCommand = DEFAULTSTATE;
				}
				break;
			case DELETEALL:
				//Detele all IDs on the scanner.
				returnedResponse = fps->deleteAll();
				currentCommand = DEFAULTSTATE;
				sendRspMsg(MQTT, *returnedResponse);
				break;
			case DELETEID:
				//Delete the specified ID on the scanner.
				returnedResponse = fps->deleteID(args);
				currentCommand = DEFAULTSTATE;
				sendRspMsg(MQTT, *returnedResponse);
				break;
			case ISPRESSFINGER:
				//Determines if there is a finger on the scanner. If so, params[0] in the response is 1, otherwise its 0.
				if(fps->IsFingerPressed())returnedResponse->params[0] = 1;
				else returnedResponse->params[0] = 0;
				returnedResponse->command = IsPressFinger;
				sendRspMsg(MQTT, *returnedResponse);
				currentCommand = DEFAULTSTATE;
				break;
			case CLOSE:
				//Close and Delete The Scanner.
				fps->ToggleLED(false);
				returnedResponse = fps->CloseFPS();
				delete fps;
				serialClose(fd);
				loop = false;
				sendRspMsg(MAIN, *returnedResponse);
				break;
		}
	}
	return 0;
}

PI_THREAD (MQTTThread){
	bool run = true;
	Response returnedResponse;
	struct mosquitto *mosq = NULL;
	while(run){
		//Check for mail and perform the required operation if there is mail.
		mosquitto_lib_init();
		mosq = mosquitto_new (NULL, true, NULL);
		if (!mosq){
			cerr << "Can't initialize Mosquitto Library" << endl;
			sendCmdMsg(SCANNER, CLOSE);
			run = false;

		}
		mosquitto_username_pw_set (mosq, MQTT_USERNAME, MQTT_PASSWORD);

		int ret = mosquitto_connect (mosq, MQTT_HOSTNAME, MQTT_PORT, 0);
		if (ret){
			cerr << "Can't connect to Mosquitto server" << endl;
			sendCmdMsg(SCANNER, CLOSE);
			run = false;
		}

		for (int i = 0; i < 5; i++){
			string testMQTTMessage = "From FPS Libarary";
			cout << "sending message!" << endl;
			ret = mosquitto_publish (mosq, NULL, MQTT_TOPIC,testMQTTMessage.size(), &testMQTTMessage[0], 0, false);
		}
		if(checkMail(MQTT)){
			Message newMessage = getMessage(MQTT);
			if(newMessage.messageType == RESPONSE){
				//What to do with response... For test I print response parameters.
				newMessage.response.printParams();
			}
			else{
				//Command was received from user input
				sendCmdMsg(SCANNER, newMessage.command, newMessage.args);
				if(newMessage.command == CLOSE){
					sendCmdMsg(SCANNER, CLOSE);
					run = false;
				}
			}
		}
	}
	sleep (1);

	// Clean up Mosquitto
	mosquitto_disconnect (mosq);
	mosquitto_destroy (mosq);
	mosquitto_lib_cleanup();
	return 0;
}

int main(){
	piThreadCreate (FPScanner);
	piThreadCreate (MQTTThread);
	int input;
	int args;
	while(1){
		args = 0;
		cout << "Please Enter Command:\n (1)Enroll (2)Identify (3)Verify (4)DeleteAll\n (5)DeleteID (6)Is Press Finger (7)Quit\n";
		cin >> input;
		if(input == 3){
			cout << "What ID would you like to Verify?: \n";
			cin >> args;
		}
		else if(input == 5){
			cout << "What ID would you like to Delete?: \n";
			cin >> args;
		}
		sendCmdMsg(MQTT, (FPSCommandStates)input, args);
		if(input == 7){
				while(1){
					if(checkMail(MAIN)) return 1;
				}
		}
	}
return 0;
}
