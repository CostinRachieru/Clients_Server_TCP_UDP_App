#include "helpers.h"
#include <algorithm>

class Subscriber_message {
public:
    uint16_t len;
    bool subscribe;
    char topic[TOPIC_MAX_LEN];
    char topic_len;
    bool SF;

    // Constructor
    Subscriber_message() {
        len = sizeof(Subscriber_message);
        this -> subscribe = false;
        memset(topic, 0, TOPIC_MAX_LEN);
        this -> SF = false;
        this -> topic_len = 0;
    }

    Subscriber_message(bool subscribe, bool SF, char* topic, int topic_len) {
        this -> subscribe = subscribe;
        memcpy(this->topic, topic, std::min(TOPIC_MAX_LEN, topic_len));
        this -> SF = SF;
    }

    // Destructor
    ~Subscriber_message() {
    }

    bool isSubscribing() {
        return subscribe;
    }

    // bool getSF() {
    //     return SF;
    // }

    // char* getTopic() {
    //     return topic;
    // }

    // char getTopicLen() {
    //     return topic_len;
    // }
};