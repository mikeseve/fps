#include "FPS.h"
#include <iostream>
using std::cerr;
using std::cout;
char* Response::getResponse(int fd){
	char* resp = new char[12];
	char firstByte = 0;
	bool loop = true;
	while(loop){
		firstByte = serialGetchar(fd);
		if(firstByte == 0x55) loop = false;
	}
	resp[0] = firstByte;
	int i = 1;
	while(i < 12){
		if(serialDataAvail(fd)){
			resp[i] = serialGetchar(fd);
			i++;
		}
	}	
	return resp;
}
Command::Command(){	
	for(int i = 0; i < 4; i++){
		parameters[i] = 0;
}
}
char* Command::getPacket(){
	char * bytes = new char[12];
	bytes[0] = COMMAND_START_1;
	bytes[1] = COMMAND_START_2;
	bytes[2] = COMMAND_DEVICE_1;
	bytes[3] = COMMAND_DEVICE_2;
	bytes[4] = parameters[0];
	bytes[5] = parameters[1];
	bytes[6] = parameters[2];
	bytes[7] = parameters[3];
	bytes[8] = command & 0xff;
	bytes[9] = (command >> 8) & 0xff;
	
	//Calculate Check Sum:
	short checkSum = 0;
	for(int i = 0; i < 10; i++){
		checkSum += bytes[i];
	}
	
	bytes[10] = checkSum & 0xff;
	bytes[11] = (checkSum >> 8) & 0xff;
	return bytes;
}
Response::Response(){
	for(int i = 0; i < 4; i++){
		params[i] = 0;
	}
}
void Response::printParams(){
	cout <<"Command: " << command <<  " Parameters: ";
	for(int i = 3; i >=0; i--) cout << std::hex << (int)params[i];
	cout << "\n"; 
}
void Response::ParseBytes(int fd){
	char* bytes = getResponse(fd);
	params[0] = bytes[4];
	params[1] = bytes[5];
	params[2] = bytes[6];
	params[3] = bytes[7];
	if(bytes[8] == 0x30)failed = false;
	else{
		failed = true;
		unsigned int error = params[3];
		error = (error << 8) + params[2];
		error = (error << 8) + params[1];
		error = (error << 8) + params[0];
		errorCode = (Errors_Enum)error;
	}
}
bool Response::getData(char* &array){
	if(data != NULL && data->getSize() !=0){
		array = data->returnedValues;
		return true;
	}
	return false;
}

Data::Data(){
	length = 0;
}
Data::Data(commandList cmd, int fd){
	length = 0;
	command = cmd;
	getValues(fd);
}
int Data::getSize(){
	return length;
}
char* Data::getValues(int fd){
	int size;
	
	if(command == Enroll3) size = 498;
	else if(command == MakeTemplate) size = 498;
	else if(command == GetImage) size = 52116;
	else if(command == GetRawImage)	size = 19200;
	else if(command == GetTemplate)	size = 498;
	else if(command == Open) size = 24;
	else return new char[0];
	char* resp = new char[size + 6];
	char firstByte = 0;
	bool loop = true;
	while(loop){
		firstByte = serialGetchar(fd);
		if(firstByte == DATA_START_1) loop = false;
	}
	resp[0] = firstByte;
	int i = 1;
	while(i < (size + 6)){
		if(serialDataAvail(fd)){
			resp[i] = serialGetchar(fd);
			i++;
		}
	}
	char* returnVal = new char[size];
	for(int i = 0; i < size; i++){
		returnVal[i] = resp[i+4];
		length++;
	}
	delete resp;
	returnedValues = returnVal;
	return returnVal;
}
FPS::FPS(){
}
FPS::FPS(int FD){
	fd = FD;
}
void FPS::setFD(int FD){
	fd = FD;
}
	
void FPS::sendBytes(char * bytes){
	for(int i = 0; i < 12; i++){
		serialPutchar(fd, bytes[i]);
	}
}
Response* FPS::OpenFPS(bool moreData){
	Command * cmd = new Command();
	cmd->command = Open;
	if(moreData) cmd->parameters[0] = 0x01;
	char* bytes = cmd->getPacket();
	sendBytes(bytes);
	delete cmd;
	delete bytes;
	Response *rp = new Response();
	rp->command = Open;
	rp->ParseBytes(fd);
	if(moreData) rp->data = new Data(Open, fd);
	return rp;
}

Response* FPS::ToggleLED(bool on){
	Command * cmd = new Command();
	cmd->command = CmosLed;
	cmd -> parameters[0] = 0x00;
	if(on) cmd->parameters[0] = 0x01;
	char* bytes = cmd->getPacket();
	sendBytes(bytes);
	delete cmd;
	delete bytes;
	Response *rp = new Response();
	rp->command = CmosLed;
	rp->ParseBytes(fd);
	return rp;
}

void  FPS::BlinkLED(int delayTime){
	ToggleLED(false);
	delay(delayTime);
	ToggleLED(true);
}

Response* FPS::CloseFPS(){
	Command * cmd = new Command();
	cmd->command = Close;
	char* bytes = cmd->getPacket();
	sendBytes(bytes);
	delete cmd;
	delete bytes;
	Response *rp = new Response();
	rp->command = Close;
	rp->ParseBytes(fd);
	return rp;
}
bool FPS::CheckEnrolledID(int id){
	Command * cmd = new Command();
	cmd->command = CheckEnrolled;
	cmd->parameters[0] = id & 0xff;
	cmd->parameters[1] = (id >> 8) & 0xff;
	cmd->parameters[2] = (id >> 16) & 0xff;
	cmd->parameters[3] = (id>>24) & 0xff;
	char* bytes = cmd->getPacket();
	sendBytes(bytes);
	delete cmd;
	delete bytes;
	Response *rp = new Response();
	rp->ParseBytes(fd);
	if(rp->failed) return false;
	return true;
}
Response* FPS::StartEnroll(int id){
	Command * cmd = new Command();
	cmd->command = EnrollStart;
	cmd->parameters[0] = id & 0xff;
	cmd->parameters[1] = (id >> 8) & 0xff;
	cmd->parameters[2] = (id >> 16) & 0xff;
	cmd->parameters[3] = (id >>24) & 0xff;
	char* bytes = cmd->getPacket();
	sendBytes(bytes);
	delete cmd;
	delete bytes;
	Response *rp = new Response();
	rp->command = EnrollStart;
	rp->ParseBytes(fd);
	return rp;
}
bool FPS::IsFingerPressed(){
	Command * cmd = new Command();
	cmd->command = IsPressFinger;
	char * bytes = cmd->getPacket();
	sendBytes(bytes);
	delete cmd;
	delete bytes;
	Response *rp = new Response();
	rp->ParseBytes(fd);
	bool returnValue = false;
	int value = rp->params[0];
	value+= rp->params[1];
	value+= rp->params[2];
	value +=rp->params[3];
	if(value == 0) returnValue = true;
	return returnValue;
}
Response* FPS::captureFinger(bool useBestImage){
	Command * cmd = new Command();
	cmd->command = CaptureFinger;
	if(useBestImage) cmd-> parameters[0] = 0x01;
	else cmd->parameters[0] = 0x00;
	char * bytes = cmd->getPacket();
	sendBytes(bytes);
	delete cmd;
	delete bytes;
	Response *rp = new Response();
	rp->command = CaptureFinger;
	rp->ParseBytes(fd);
	return rp;
}
Response* FPS::EnrollPart(int enrollNum){
	Response *rp = new Response();
	if(enrollNum <1 || enrollNum > 3){
		cerr << "An Error Occured: Enroll Number must be between 1-2\n";
		return rp;
	}
	Command * cmd = new Command();
	if(enrollNum == 1){
		cmd->command = Enroll1;
		rp->command = Enroll1;
	}
	else if(enrollNum == 2){
		cmd->command = Enroll2;
		rp->command = Enroll2;
	}
	else{
		cmd->command = Enroll3;
		rp->command = Enroll3;
	}
	char* bytes = cmd->getPacket();
	sendBytes(bytes);
	delete bytes;
	delete cmd;
	rp->ParseBytes(fd);
	return rp;
}
Response* FPS::Identify(){
	Response *rp = new Response();
	Command * cmd = new Command();
	cmd->command = Identify1_N;
	char* bytes = cmd->getPacket();
	sendBytes(bytes);
	delete bytes;
	delete cmd;
	rp->ParseBytes(fd);
	rp->command = Identify1_N;
	return rp;
}
Response* FPS::Verify(int id){
	Response *rp = new Response();
	Command * cmd = new Command();
	cmd->command = Verify1_1;
	for(int i = 0; i < 4; i++){
		cmd->parameters[i] = (id & 0xff);
		id = id >>8;
	}
	char * bytes = cmd->getPacket();
	sendBytes(bytes);
	delete bytes;
	delete cmd;
	rp->command = Verify1_1;
	rp->ParseBytes(fd);
	return rp;
}
Response* FPS::Enroll(int id){
	cout << "Please Place Your Finger Down\n";
	ToggleLED(true);
	while(!IsFingerPressed());
	Response *response = StartEnroll(id);
	if(response->failed){
		cout << "Start Enroll Failed.";
		return response;
	}
	for(int i = 1; i < 4; i++){
		if(!IsFingerPressed()) cout << "Please Place Your Finger Down\n";
		while(!IsFingerPressed());
		cout << "Enrolling Finger " << i << "\n";
		
		//Capture Fingerprint and Check For Errors: If any errors occur, check if the finger was not pressed and if not attempt again, otherwise return Reponse	
		response = captureFinger(true);
		if(response->failed){
			while(response->failed && response->errorCode == NACK_FINGER_IS_NOT_PRESSED){
				cout << "\nCapture Failed: Please Ensure Your Finger Is Centered And Try Again...\n\n";
				cout << "Please Remove Your Finger\n";
				while(IsFingerPressed())BlinkLED(250);
				cout << "Please Place Your Finger Down\n";
				while(!IsFingerPressed());
				response = captureFinger(true);
			}
			if(response->errorCode != NACK_FINGER_IS_NOT_PRESSED){
				return response;
			}
		}
		//Enroll The Captured Fingerprint and Check For Errors. If any errors occur, return the response;
		response = EnrollPart(i);
		if(response->failed){
			cout << "Enrollment Failed\n";
			ToggleLED(false);
			return response;
		}

		cout <<"Please Remove Your Finger\n";
		while(IsFingerPressed()) BlinkLED(250);
	}
	return response;
}
Response* FPS::deleteAll(){
	Response *rp = new Response();
	Command * cmd = new Command();
	cmd->command = DeleteAll;
	char * bytes = cmd->getPacket();
	sendBytes(bytes);
	delete bytes;
	delete cmd;
	rp->command = DeleteAll;
	rp->ParseBytes(fd);
	return rp;
}
Response* FPS::deleteID(int id){
	Response *rp = new Response();
	Command * cmd = new Command();
	cmd->command = DeleteID;
	for(int i =  0; i < 4; i++){
		cmd->parameters[i] = id & 0xff;
		id = id <<8;
	}
	char * bytes = cmd->getPacket();
	sendBytes(bytes);
	delete bytes;
	delete cmd;
	rp->command = DeleteID;
	rp->ParseBytes(fd);
	return rp;
}
Response* FPS::getEnrollCount(){
	Response *rp = new Response();
	Command *  cmd = new Command();
	cmd->command = GetEnrollCount;
	char * bytes = cmd->getPacket();
	sendBytes(bytes);
	delete bytes;
	delete cmd;
	rp->command = DeleteID;
	rp->ParseBytes(fd);
	return rp;
}
Response* FPS::getImage(){
	Response *rp = new Response();
	Command * cmd = new Command();
	cmd->command = GetImage;
	char * bytes = cmd->getPacket();
	sendBytes(bytes);
	delete bytes;
	delete cmd;
	rp->ParseBytes(fd);
	rp->data = new Data(GetImage, fd);
	rp->command = GetImage;
	return rp;
}
