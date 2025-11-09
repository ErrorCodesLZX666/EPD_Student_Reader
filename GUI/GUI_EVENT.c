#include "GUI_EVENT.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "SYS_GUI.h"
#include "log.h"

QueueHandle_t guiEventQueue;

void GUI_HandleEvent(const GUI_Event_t *event)
{
    LOGD("GUI_HandleEvent: event type = %d", event->type);
    if (event->type != GUI_EVENT_KEY)
        return;
    switch (uiStatus)
    {
    case UI_STATE_LOCK:
        if (event->key == KEY_HOME)
            UI_SwitchTo(UI_STATE_LIST);
        break;
    case UI_STATE_LIST:
        if (event->key == KEY_UP)
        {
            // 向上选择小说
            UI_List_Page_up_action();
        }
        else if (event->key == KEY_DOWN)
        {
            // 向下选择小说
            UI_List_Page_down_action();
        }
        else if (event->key == KEY_OK)
        {
            // 载入小说阅读界面
            UI_SwitchTo(UI_STATE_READER);
        }
        break;
    case UI_STATE_READER:
        if (event->key == KEY_UP)
            UI_Content_Page_up_action();    // 小说上一页
        else if (event->key == KEY_DOWN)
            UI_Content_Page_down_action();  // 小说下一页
        else if (event->key == KEY_HOME)
            UI_SwitchTo(UI_STATE_LOCK);
        else if (event->key == KEY_OK)
            UI_SwitchTo(UI_STATE_READER);
        else if (event->key == KEY_HOME)
            UI_SwitchTo(UI_STATE_LOCK);
        break;
    default:
        break;
    }
}

/* 初始化队列 */
// __attribute__((constructor))
void GUI_Event_Init(void)
{
    guiEventQueue = xQueueCreate(8, sizeof(GUI_Event_t));
}
