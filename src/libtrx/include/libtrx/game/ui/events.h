#pragma once

#include "../../event_manager.h"
#include "./widgets/base.h"

typedef void (*EVENT_LISTENER)(const EVENT *, void *user_data);

void UI_Events_Init(void);
void UI_Events_Shutdown(void);

int32_t UI_Events_Subscribe(
    const char *event_name, const UI_WIDGET *sender, EVENT_LISTENER listener,
    void *user_data);

void UI_Events_Unsubscribe(int32_t listener_id);

void UI_Events_Fire(const EVENT *event);
