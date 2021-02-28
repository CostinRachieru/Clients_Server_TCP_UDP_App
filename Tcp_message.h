// #include "helpers.h"
#include "Udp_message.h"

class Tcp_message {
public:
    uint16_t msg_len;

    uint32_t cli_ip;
    uint16_t cli_port; // Se face ntohs la destinatie
    
    char topic[TOPIC_MAX_LEN];

    char data_type;

    uint16_t content_len;
    
    uint32_t number32;
    uint16_t number16;
    char negative_power;
    char sign;

    // Constructor
    Tcp_message(uint32_t cli_ip, uint16_t cli_port, Udp_message *udp_message) {
        this -> msg_len = sizeof(Tcp_message);
        this -> cli_ip = cli_ip;
        this -> cli_port = cli_port;
        this -> data_type = udp_message -> data_type;
        memcpy(this -> topic, udp_message -> topic, TOPIC_MAX_LEN);
        this -> number32 = udp_message -> number32;
        this -> number16 = udp_message -> number16;
        this -> negative_power = udp_message -> negative_power;
        this -> sign = udp_message -> sign;
        this -> content_len = udp_message -> content_len;
    }

    Tcp_message() {
    }

    // Destructor
    ~Tcp_message() {
    }
};