#!/bin/bash

systemd-notify --status="Initiating…"

# Imitate time-absorbing initialization.
sleep 10

systemd-notify --ready

count=0
while true; do
    ((count++))
    systemd-notify --status="Working hard [${count}]"

    # Imitate time-absorbing processing.
    sleep 1
done
