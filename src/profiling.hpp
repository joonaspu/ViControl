#include <string>

#define START_TIMER(desc) timerStartTime(desc)
#define END_TIMER(desc) timerEndTime(desc)

void timerStartTime(std::string key);
void timerEndTime(std::string key);