#!/bin/bash

outputFile=/tmp/my-memory-monitor.log

currentDate=$(date "+%H:%M:%S %d-%m-%Y")
read _ totalMem _ freeMem _< <(free -hw | head -2 | tail -1)
read _ totalDisk _ freeDisk _< <(df -h /dev/sda1 | head -2 | tail -1)

printf "[%s]\t" "${currentDate}" >> ${outputFile}
printf "Memory (free/total): %s/%s\t" "${freeMem}" "${totalMem}" >> ${outputFile}
printf "Disk (free/total): %s/%s\n" "${freeDisk}" "${totalDisk}" >> ${outputFile}

exit 0
