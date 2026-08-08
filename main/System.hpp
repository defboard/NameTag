#pragma once

#include <Preferences.h>
#include <StreamString.h>


extern StreamString eventLog;


constexpr char PREFS_NAMESPACE[] = "NameTag";
static_assert(sizeof(PREFS_NAMESPACE) <= 16);
