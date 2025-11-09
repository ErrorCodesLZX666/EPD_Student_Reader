#ifndef __GUI_EVENT_H__
#define __GUI_EVENT_H__

#include "SYS_GUI.h"
#include "SYS_KEY.h"
#include "FreeRTOS.h"
#include "task.h"

typedef enum {
    GUI_EVENT_NONE = 0,
    GUI_EVENT_KEY
} GUI_EventType_t;

typedef struct {
    GUI_EventType_t type;
    KeyCode_t key;
} GUI_Event_t;

extern QueueHandle_t guiEventQueue;

void GUI_HandleEvent(const GUI_Event_t *event);

#endif
