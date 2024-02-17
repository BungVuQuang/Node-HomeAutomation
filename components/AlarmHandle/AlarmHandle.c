#include "AlarmHandle.h"

int compareDateTime(const void *a, const void *b)
{
    struct Alarm *alarmA = (struct Alarm *)a; // time goc
    struct Alarm *alarmB = (struct Alarm *)b; // time can so sanh

    if (strcmp(alarmA->date, alarmB->date) == 0)
    {
        if (strcmp(alarmA->time, alarmB->time) == 0)
        {
            return 0;
        }
        else if (strcmp(alarmA->time, alarmB->time) > 0)
        {
            return strcmp(alarmA->time, alarmB->time);
        }
        else
        {
            return strcmp(alarmA->time, alarmB->time);
        }
    }
    else if (strcmp(alarmA->date, alarmB->date) > 0)
    {
        return strcmp(alarmA->date, alarmB->date);
    }
    else
    {
        return strcmp(alarmA->date, alarmB->date);
    }
}

// Hàm thêm một báo thức vào Min-Heap
void insertAlarm(struct Alarm alarms[], int *heapSize, struct Alarm newAlarm)
{
    (*heapSize)++;
    alarms[*heapSize] = newAlarm;
    int i = *heapSize;

    // Duyệt lên trên để đảm bảo Min-Heap property
    while (i > 0 && compareDateTime(&alarms[i], &alarms[i / 2]) < 0)
    {
        // Swap alarms[i] và alarms[i/2]
        struct Alarm temp = alarms[i];
        alarms[i] = alarms[i / 2];
        alarms[i / 2] = temp;
        i = i / 2;
    }
}

// Hàm xóa một báo thức theo alarmID và cập nhật Min-Heap
void deleteAlarm(struct Alarm alarms[], int *heapSize, int alarmID)
{
    int i;
    for (i = 0; i <= *heapSize; i++)
    {
        if (alarms[i].alarmID == alarmID)
        {
            // Swap alarms[i] và alarms[*heapSize], sau đó giảm heapSize
            // if (*heapSize == -1)
            // {

            //     sprintf(alarms[0].time, "23:00:00");
            //     sprintf(alarms[0].date, "30/12/2099");
            // }
            // else
            //{
            struct Alarm temp = alarms[i];
            alarms[i] = alarms[*heapSize];
            alarms[*heapSize] = temp;
            (*heapSize)--;
            // Duyệt xuống để đảm bảo Min-Heap property
            int parent = i;
            int leftChild = 2 * parent;
            int rightChild = 2 * parent + 1;

            while (leftChild <= *heapSize)
            {
                int smallest = parent;

                if (compareDateTime(&alarms[leftChild], &alarms[smallest]) < 0)
                {
                    smallest = leftChild;
                }

                if (rightChild <= *heapSize && compareDateTime(&alarms[rightChild], &alarms[smallest]) < 0)
                {
                    smallest = rightChild;
                }

                if (smallest != parent)
                {
                    // Swap alarms[parent] và alarms[smallest]
                    struct Alarm temp = alarms[parent];
                    alarms[parent] = alarms[smallest];
                    alarms[smallest] = temp;
                    parent = smallest;
                    leftChild = 2 * parent;
                    rightChild = 2 * parent + 1;
                }
                else
                {
                    break;
                }
            }
            if (*heapSize == -1)
            {

                sprintf(alarms[0].time, "23:00:00");
                sprintf(alarms[0].date, "30/12/2099");
            }
            //}
            return;
        }
    }
}