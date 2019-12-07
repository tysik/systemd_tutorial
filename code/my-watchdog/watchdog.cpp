#include "watchdog.h"

#include <systemd/sd-daemon.h>
#include <systemd/sd-journal.h>

#include <functional>
#include <iostream>

static std::function<void()> watchdogPingImpl = []() {};
static uint64_t watchdogUsec = 0;

void initializeWatchdog() {
  if (sd_watchdog_enabled(0, &watchdogUsec) > 0) {
    // Leave the empty implementation if a watchdog is not available.
    watchdogPingImpl = []() {
      sd_journal_print(LOG_NOTICE, "Watchdog ping.\n");
      sd_notify(0, "WATCHDOG=1");
    };
  }
}

void pingWatchdog() { watchdogPingImpl(); }

void triggerWatchdog() {
  sd_journal_print(LOG_ERR, "Watchdog trigger.\n");
  sd_notify(0, "WATCHDOG=trigger");
}

bool isWatchdogEnabled() { return watchdogUsec > 0; }

void setWatchdogTimeoutUsec(uint64_t usec) {
  sd_journal_print(LOG_INFO, "Watchdog timeout changed to %" PRIu64 ".\n", usec);
  sd_notifyf(0, "WATCHDOG_USEC=%" PRIu64, usec);
  watchdogUsec = usec;
}

uint64_t getWatchdogTimeoutUsec() { return watchdogUsec; }
