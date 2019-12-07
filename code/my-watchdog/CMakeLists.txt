project(test-watchdog)

add_executable(test-watchdog main.cpp watchdog.cpp)
target_link_libraries(test-watchdog systemd)
