#include "watchdog.h"

#include <systemd/sd-daemon.h>
#include <systemd/sd-journal.h>

#include <chrono>
#include <iostream>
#include <thread>

int main() {
  initializeWatchdog();

  if (isWatchdogEnabled()) {
    sd_journal_print(LOG_NOTICE, "Watchdog configured with %" PRIu64 " μs.\n",
                     getWatchdogTimeoutUsec());
  } else {
    sd_journal_print(LOG_NOTICE, "Watchdog not configured.\n");
    return 0;
  }

  sd_notify(0, "READY=1");

  int counter = 0;
  while (true) {
    std::this_thread::sleep_for(
        std::chrono::microseconds(getWatchdogTimeoutUsec() / 2));

    pingWatchdog();
    if (++counter == 5) {
      setWatchdogTimeoutUsec(60 * 1000 * 1000);

    //   uint64_t newWdgTimeout = 0;
    //   if (sd_watchdog_enabled(0, &newWdgTimeout) > 0) {
    //     sd_journal_print(LOG_INFO, "New wdg timeout %" PRIu64 ".\n",
    //                      newWdgTimeout);
    //   }
    }
  }

  return 0;
}
