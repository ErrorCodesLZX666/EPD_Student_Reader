#ifndef __SYS_GUI_H__
#define __SYS_GUI_H__
#include "gd32f4xx.h"
#include "APP_GUI.h"


typedef enum {
    UI_STATE_LOCK,
    UI_STATE_LOADING,
    UI_STATE_LIST,
    UI_STATE_READER
} UI_State_t;

extern UI_State_t uiStatus;

// 小说列表数据
extern NovelListBox_t box;

// 小说阅读界面数据
extern const char *novelText;
extern const char *novelProgress;
extern const char *novelContent;

// 定义用来刷新界面携带的数据
// void update_list(const char **novel_array, uint16_t count, uint16_t index, const char *time_str);
// void update_content(const char *title, const char *progress, const char *content);

// 尝试定义用来超控UI的
void UI_List_Page_down_action(void);
void UI_List_Page_up_action(void);
void UI_Content_Page_up_action(void);
void UI_Content_Page_down_action(void);

void SYSGUI_Entries(void);      // GUI初始化入口（在main中调用）
void UI_SwitchTo(UI_State_t newState);  // 状态切换函数
void UI_Refresh(void);          // 根据当前状态重绘界面


void GUI_Event_Init(void);

#endif