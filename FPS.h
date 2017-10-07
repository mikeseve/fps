#include "wiringSerial.h"
#include "wiringPi.h"
//Enum of all commands the fingerprint scanner can read.
enum commandList{
	NotSet				= 0x00,		// Default value for enum. Scanner will return error if sent this.
	Open				= 0x01,		// Open Initialization
	Close				= 0x02,		// Close Termination
	UsbInternalCheck		= 0x03,		// UsbInternalCheck Check if the connected USB device is valid
	ChangeBaudRate			= 0x04,		// ChangeBaudrate Change UART baud rate
	SetIAPMode			= 0x05,		// SetIAPMode Enter IAP Mode In this mode, FW Upgrade is available
	CmosLed				= 0x12,		// CmosLed Control CMOS LED
	GetEnrollCount			= 0x20,		// Get enrolled fingerprint count
	CheckEnrolled			= 0x21,		// Check whether the specified ID is already enrolled
	EnrollStart			= 0x22,		// Start an enrollment
	Enroll1				= 0x23,		// Make 1st template for an enrollment
	Enroll2				= 0x24,		// Make 2nd template for an enrollment
	Enroll3				= 0x25,		// Make 3rd template for an enrollment, merge three templates into one template, save merged template to the database
	IsPressFinger			= 0x26,		// Check if a finger is placed on the sensor
	DeleteID			= 0x40,		// Delete the fingerprint with the specified ID
	DeleteAll			= 0x41,		// Delete all fingerprints from the database
	Verify1_1			= 0x50,		// Verification of the capture fingerprint image with the specified ID
	Identify1_N			= 0x51,		// Identification of the capture fingerprint image with the database
	VerifyTemplate			= 0x52,		// Verification of a fingerprint template with the specified ID
	IdentifyTemplate		= 0x53,		// Identification of a fingerprint template with the database
	CaptureFinger			= 0x60,		// Capture a fingerprint image(256x256) from the sensor
	MakeTemplate			= 0x61,		// Make template for transmission
	GetImage			= 0x62,		// Download the captured fingerprint image(256x256)
	GetRawImage			= 0x63,		// Capture & Download raw fingerprint image(320x240)
	GetTemplate			= 0x70,		// Download the template of the specified ID
	SetTemplate			= 0x71,		// Upload the template of the specified ID
	GetDatabaseStart		= 0x72,		// Start database download, obsolete
	GetDatabaseEnd			= 0x73,		// End database download, obsolete
	UpgradeFirmware			= 0x80,		// Not supported
	UpgradeISOCDImage		= 0x81,		// Not supported
	SetSecurityLevel		= 0xF0,		// Set Security Level
	GetSecurityLevel		= 0xF1,		// Get Security Level
	Ack				= 0x30,		// Acknowledge.
	Nack				= 0x31		// Non-acknowledge
};
//Enum of all Errors the fingerprint scanner can transmitt
enum Errors_Enum
{
	NO_ERROR				= 0x0000,	// Default value. no error
	NACK_TIMEOUT				= 0x1001,	// Obsolete, capture timeout
	NACK_INVALID_BAUDRATE			= 0x1002,	// Obsolete, Invalid serial baud rate
	NACK_INVALID_POS			= 0x1003,	// The specified ID is not between 0~199
	NACK_IS_NOT_USED			= 0x1004,	// The specified ID is not used
	NACK_IS_ALREADY_USED			= 0x1005,	// The specified ID is already used
	NACK_COMM_ERR				= 0x1006,	// Communication Error
	NACK_VERIFY_FAILED			= 0x1007,	// 1:1 Verification Failure
	NACK_IDENTIFY_FAILED			= 0x1008,	// 1:N Identification Failure
	NACK_DB_IS_FULL				= 0x1009,	// The database is full
	NACK_DB_IS_EMPTY			= 0x100A,	// The database is empty
	NACK_TURN_ERR				= 0x100B,	// Obsolete, Invalid order of the enrollment (The order was not as: EnrollStart -> Enroll1 -> Enroll2 -> Enroll3)
	NACK_BAD_FINGER				= 0x100C,	// Too bad fingerprint
	NACK_ENROLL_FAILED			= 0x100D,	// Enrollment Failure
	NACK_IS_NOT_SUPPORTED			= 0x100E,	// The specified command is not supported
	NACK_DEV_ERR				= 0x100F,	// Device Error, especially if Crypto-Chip is trouble
	NACK_CAPTURE_CANCELED			= 0x1010,	// Obsolete, The capturing is canceled
	NACK_INVALID_PARAM			= 0x1011,	// Invalid parameter
	NACK_FINGER_IS_NOT_PRESSED		= 0x1012,	// Finger is not pressed
	INVALID					= 0XFFFF	// Used when parsing fails
};
//Command:
//Class used to format finger print scanner commands more easily
class Command{
	public:
		Command();
		commandList command;
		//getPacket:
		//returns a char array using the previously set command and paraemters that is formatted correctly for the scanner
		char* getPacket();
		char parameters[4];
	private:
		//Fingerprint scanner Constants
		static const char COMMAND_START_1 = 0x55;
		static const char COMMAND_START_2 = 0xAA;
		static const char COMMAND_DEVICE_1 = 0x01;	// the device id is static at 1.
		static const char COMMAND_DEVICE_2 = 0x00;
};
//Data
//Class used to represent data transmitted from the fingerprint scanner
class Data{
public:
	//Data:
	//Default Constructor for Data
	Data();
	//Data:
	//Constructor that also sets the command
	Data(commandList command, int fd);
	//command:
	//The command run that reslts in the data. Determines the number of bytes to be transmitted
	commandList command;

	//getData
	//Returns an array containing the data transmitted by the scanner.
	//fd: the id value returned when a new serial connection is opened. Allows this method to receive data from the sensor.
	char* getValues(int fd);

	//data
	//holds the char array returned by getData.
	char* returnedValues;
	
	//getSize
	//returns the length of data returned
	int getSize();
private:
	//Fingerprint scanner constants
	static const char DATA_START_1 = 0x5A;
	static const char DATA_START_2 = 0xA5;
	static const char DATA_DEVICE_1 = 0x01;
	static const char DATA_DEVICE_2 = 0x02;
	//size:
	//holds the number of bytes returned
	int length;
};
//Response:
//Class used to extract information from the fingerprint scanner more easily.
class Response{
	public:
		Response();
		//getResponse():
		//returns a char array containing the data retrieved from the scanner.
		char* getResponse(int fd);
		//ParseBytes:
		//Extracts useful information from the data given by the fingerprint scanner after a command is executed
		void ParseBytes(int fd);
		bool failed;					//True if an error occured, false otherwise 					
		char params[4]; 				//if ACK, then this is the output parameter, if NACK ,then this represents the error
		Errors_Enum errorCode;
		//printParams:
		//prints the values of the parameters to std::cout
		//used mainly for debugging
		void printParams();
		
		//command:
		//Command that was executed resulting in this response
		commandList command;

		//data:
		//If data beyond the parameters are returned from a command, they are placed here.
		Data* data;

		//getData():
		//If data was returned by the scanner,array will be set to the data returned and the method returns true. Otherwise, false is returned.
		bool getData(char* &array);
	private:
		//Fingerprint scanner constants
		static const char RESPONSE_START_1 = 0x55;
		static const char RESPONSE_START_2 = 0xAA;
		static const char RESPONSE_DEVICE_1 = 0x01;	// the device id is static at 1.
		static const char RESPONSE_DEVICE_2 = 0x00;
		
};

class FPS{
	public:
		//Constructors:
		FPS();
		//Sets the FD
		//FD: Value returned when a new serial connection is opened.	
		FPS(int FD);
		
		//OpenFPS:
		//Opens the finger print scanner
		//moreData: if true, this function will return the FirmwareVersion, IsoAreaMaxSize, and SerialNumber
		Response* OpenFPS(bool moreData);
		
		//ToggleLED:
		//Changes the state of the CMOS Led ( backlight )
		//on: if true, the LED turns on if false, the LED turns off.
		Response* ToggleLED(bool on);
		
		//BlinkLED:
		//Blinks the LED once with delay specified. Ends with ON state: (OFF, ON)
		//int delayTime: delay between blinks
		void BlinkLED(int delayTime);
		
		//CloseFPS:
		//Closes the finger print scanner
		Response* CloseFPS();
		
		//sendBytes:
		//Sends a command packet to the scanner over UART
		//bytes: char array containing the bytes to be sent to the scanner. (See Command::getPacket)
		void sendBytes(char* bytes);
		
		//setFD:
		//Sets the device ID for wiringPi's UART
		//FD: the id returned with a UART connection is opened using wiringPi
		void setFD(int FD);
		
		//CheckEnrolledID:
		//Checks if the ID specified is currently enrolled in the system.
		//id: the id to be checked ( 0-199 )
		//returns true if the ID is enrolled, false otherwise.
		bool CheckEnrolledID(int id);
		
		//StartEnroll(int id)
		//Command used to initiate an enroll of a new id.
		//id: the new ID to be specified (0-199) 
		Response* StartEnroll(int id);
		
		//IsFingerPressed:
		//Checks if a finger is currently on the scanner
		//Returns false if finger is not pressed, true if it is.
		//NOTE: LED must be on to get an accurate reading.
		bool IsFingerPressed();
		
		//captureFinger:
		//Takes a picture to be used with other methods
		//useBestImage: When true, a higher quality image is taken, but it is slower. Suggested use is high quality for enrollment, and low for verification and identification
		//NOTE: LED must be on to get an accurate reading.
		Response* captureFinger(bool useBestImage);

		//Enroll Part
		//Performs the Enroll1, Enroll2, and Enroll3 functions of the fingerprint scanner. Must run 3 times to fully enroll a finger. See the Enroll method for full isntructions
		//enrollNum: The enroll number (1-3) to perform.
		//NOTE: LED myst be on to get an accurate reading.
		Response* EnrollPart(int enrollNum);
		
		//Enrooll:
		//Runs whole process for enrolling a finger to the specified ID.
		//Steps Involved: StartEnroll, captureFinger, Enroll1, captureFinger, Enroll2, captureFinger, Enroll3
		//id: the number id currently being enrolled (0-199)
		Response* Enroll(int id);

		//Verify:
		//Determines whether the captred fingerprint matches the id specified
		//id:The number id (0-199) to be verified.
		Response* Verify(int id);
		
		//Identify:
		//Determines the id that cooresponds to the captured fingerprint image.
		Response* Identify();
		
		//deleteAll:
		//Delets all fingerprints from the database
		Response* deleteAll();
	
		//deleteID:
		//Delete the fingerprint with the specified ID.
		//id: The numer id (0-199) to be deleted.
		Response* deleteID(int id);		
		
		//getEnrollCount:
		//Returns the number of fingerprints enrolled in the system.
		Response* getEnrollCount();

		//Response makeTemplate();
		//Response getTemplate();
		//Response setTemplate();
		//Response verifyTemplate();
		//Response identifyTemplate();

		//I could get the char array pretty easily I just dont know how to create an image from that....
		Response* getImage();	
		private:
		int fd;
};



