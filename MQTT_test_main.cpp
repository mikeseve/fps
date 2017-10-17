#include "MQTT.h"
#include <iostream>

using std::cout;
using std::endl;


int main(){

	MQTT mqtt = MQTT("futurehaus.cs.vt.edu", 1883, "futurehaus", "HokieDVE");
	
	mqtt.publish("doorTest", "custom Test publish");

	mqtt.subscribe("doorTest");

	while(1){

		mqtt.refresh();
		while(!mqtt.empty()){
			cout << mqtt.getMessage() << endl;
		}

	}
	




	mqtt.cleanup();

	return 0;
}


