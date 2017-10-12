#include <string>
#include <mosquitto.h>
#include <queue>

using std::queue;
using std::string;
class MQTT{
public:
    MQTT(string hostname_, int port_, string username_, string password_);
    void cleanup();
    bool publish(string topic, string message);
    void subscribe(string topic);
    void refresh();
    string getMessage();
    bool empty();


private:
    static void message_callback(struct mosquitto *mosq, void *obj,
    const struct mosquitto_message *message);
    queue<string> messages;
    string hostname;
    int port;
    string username;
    string password;
    struct mosquitto *mosq = NULL;
};
