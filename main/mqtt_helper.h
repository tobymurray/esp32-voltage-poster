#ifndef mqtt_helper_h
#define mqtt_helper_h

#include <stdint.h>

void initialize_mqtt(void);
void wait_for_mqtt_to_connect(void);
void publish_message(char datetime[], char topic[], char key[], char payload[], char key2[], char payload2[]);
void wait_for_all_messages_to_be_published(void);

#endif