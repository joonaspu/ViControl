#include <string>
#include <iostream>
#include <unordered_map>
#include <time.h>

std::unordered_map<std::string, timespec*> map;

void timerStartTime(std::string key) {
    timespec* start = new timespec();
    clock_gettime(CLOCK_REALTIME, start);
    map[key] = start;
}

void timerEndTime(std::string key) {
    timespec end;
    timespec* start = map[key];

    clock_gettime(CLOCK_REALTIME, &end);
    time_t s = (end.tv_sec - start->tv_sec) * 1000;
    time_t ns = (end.tv_nsec - start->tv_nsec) / 1000000;
    time_t ms = s + ns;
    std::cout << key << ": " << ms << " ms" << std::endl;

    delete start;
}