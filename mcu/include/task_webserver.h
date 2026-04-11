
#ifndef __TASK_WEBSERVER_H__
#define __TASK_WEBSERVER_H__


#include <global.h>
#include "task_handler.h" 



void Webserver_stop();
void Webserver_reconnect();
void Webserver_sendata(String data);
void task_webserver(void *pvParameters);
#endif