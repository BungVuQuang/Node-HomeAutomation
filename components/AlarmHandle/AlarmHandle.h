#ifndef _ALARMHANDLE_H_
#define _ALARMHANDLE_H_

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include <string.h>
struct Alarm
{
    int alarmID;
    char device[15];
    int state;
    char time[15];
    char date[20];
};
int compareDateTime(const void *a, const void *b);
void insertAlarm(struct Alarm alarms[], int *heapSize, struct Alarm newAlarm);

void deleteAlarm(struct Alarm alarms[], int *heapSize, int alarmID);

#endif /* _ALARMHANDLE_H_ */
