#!/bin/bash

pipe=/tmp/my-logger.fifo
logbook=/tmp/my-logger.log

function save-logs() {
    logs=("$@")
    for log in "${logs[@]}"; do
        echo ${log} >> ${logbook}
    done
}

if [[ ! -p ${pipe} ]]; then
    mkfifo ${pipe}
fi

if [[ ! -f ${logbook} ]]; then
    touch ${logbook}
fi

# echo "LISTEN_FDS: ${LISTEN_FDS}" >> ${logbook}
# echo "LISTEN_PID: ${LISTEN_PID}" >> ${logbook}
# echo "LISTEN_FDNAMES: ${LISTEN_FDNAMES}" >> ${logbook}

while true; do
    logs=()
    for i in {1..5}; do
        # if read line <&3 ; then
        if read line < ${pipe}; then
            if [[ ${line} == 'quit' ]]; then
                # Store remaining logs.
                save-logs "${logs[@]}"
                exit 0
            fi
            logs+=("[$(date +%Y-%m-%d\ %H:%M:%S)]  > ${line} <")
        fi
    done
    save-logs "${logs[@]}"
done
