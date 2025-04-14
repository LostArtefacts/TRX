#pragma once

#include "../../event_manager.h"

typedef void (*EVENT_LISTENER)(const EVENT *, void *user_data);

void UI2_InitEvents(void);
void UI2_ShutdownEvents(void);

int32_t UI2_Subscribe(
    const char *event_name, const void *sender, EVENT_LISTENER listener,
    void *user_data);

void UI2_Unsubscribe(int32_t listener_id);

void UI2_FireEvent(EVENT event);
