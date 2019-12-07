#pragma once

#include <cstdint>

void initializeWatchdog();
void pingWatchdog();
void triggerWatchdog();
bool isWatchdogEnabled();
void setWatchdogTimeoutUsec(uint64_t usec);
uint64_t getWatchdogTimeoutUsec();
