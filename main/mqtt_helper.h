#ifndef mqtt_helper_h
#define mqtt_helper_h

#include <stdint.h>

void initialize_mqtt(void);
void wait_for_mqtt_to_connect(void);
void publish_message(char datetime[], char topic[], char key[], char payload[]);

#endif