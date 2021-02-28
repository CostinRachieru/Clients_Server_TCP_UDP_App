// #include "helpers.h"

class Udp_message {
public:
    char data_type;
    
    char topic[TOPIC_MAX_LEN];
    
    uint16_t content_len;
    char *content;

    uint32_t number32;
    uint16_t number16;
    char negative_power;
    char sign;

    uint32_t cli_ip;
    uint16_t cli_port;

    // Constructor
    Udp_message(char *buffer) {
        content = NULL;
        number32 = 0;
        number16 = 0;
        negative_power = 0;
        sign = 0;
        // Set topic.
        memset(topic, 0, TOPIC_MAX_LEN);
        memcpy(topic, buffer, TOPIC_MAX_LEN);

        // Set data type.
        memcpy(&data_type, buffer + TOPIC_MAX_LEN, sizeof(char));
        int x = data_type;

        if (data_type == 0) {
            sign = *(buffer + TOPIC_MAX_LEN + DATA_TYPE_LEN);
            int a = buffer[TOPIC_MAX_LEN + DATA_TYPE_LEN + 1];
            int b = buffer[TOPIC_MAX_LEN + DATA_TYPE_LEN + 2];
            int c = buffer[TOPIC_MAX_LEN + DATA_TYPE_LEN + 3];
            int d = buffer[TOPIC_MAX_LEN + DATA_TYPE_LEN + 4];
            b = b << 8;
            c = c << 16;
            d = d << 24;
            number32 = a | b;
            uint32_t mask = 1 << 16;
            mask--;
            number32 &= mask;
            uint32_t y = c | d;
            number32 |= y;
            number32 = ntohl(number32);
        }
        if (data_type == 1) {
            uint16_t *x = (uint16_t*)(buffer + TOPIC_MAX_LEN + DATA_TYPE_LEN);
            number16 = ntohs(*x);
        }
        if (data_type == 2) {
            sign = *(buffer + TOPIC_MAX_LEN + DATA_TYPE_LEN);
            int a = buffer[TOPIC_MAX_LEN + DATA_TYPE_LEN + 1];
            int b = buffer[TOPIC_MAX_LEN + DATA_TYPE_LEN + 2];
            int c = buffer[TOPIC_MAX_LEN + DATA_TYPE_LEN + 3];
            int d = buffer[TOPIC_MAX_LEN + DATA_TYPE_LEN + 4];
            b = b << 8;
            c = c << 16;
            d = d << 24;
            number32 = a | b;
            uint32_t mask = 1 << 16;
            mask--;
            number32 &= mask;
            uint32_t y = c | d;
            number32 |= y;
            number32 = ntohl(number32);
            negative_power = buffer[TOPIC_MAX_LEN + DATA_TYPE_LEN + 5];
        }
        if (data_type == 3) {
            for (content_len = 0; content_len < CONTENT_MAX_LEN; ++content_len) {
                if (buffer[content_len + TOPIC_MAX_LEN + DATA_TYPE_LEN] == 0) {
                    break;
                } 
            }
            content = new char[content_len];
            memcpy(content, buffer + TOPIC_MAX_LEN + DATA_TYPE_LEN, content_len);
        }
    }

    Udp_message() {
    }

    // Destructor
    ~Udp_message() {
        if (content != NULL) {
            delete content;
        }
    }

    void createCopy(Udp_message *copy) {
        // Udp_message copy = new Udp_message();
        copy -> data_type = data_type;
        memcpy(copy -> topic, topic, TOPIC_MAX_LEN);
        copy -> content_len = content_len;
        copy -> number32 = number32;
        copy -> number16 = number16;
        copy -> negative_power = negative_power;
        copy -> sign = sign;
        copy -> cli_ip = cli_ip;
        copy -> cli_port = cli_port;
        copy -> content = NULL;
        if (copy -> data_type == 3) {
            copy -> content = new char[content_len];
            memcpy(copy -> content, content, content_len); 
        }
        // return *copy;    
    }
};