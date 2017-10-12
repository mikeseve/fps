#include <string>
#include <mosquitto.h>

using std::string;
class MQTT{
public:
    MQTT(string hostname_, string port_, string username_, string password_);
    void cleanup();
    bool publish(string topic, string message);

private:
    string hostname;
    string port;
    string username;
    string password;
    struct mosquitto *mosq = NULL;
};
