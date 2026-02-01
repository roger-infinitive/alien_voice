#ifndef _PROFILER_TIMER_H_
#define _PROFILER_TIMER_H_ 

// WINDOWS ONLY

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include "windows.h"

struct Timer {
    LARGE_INTEGER start;
};

Timer StartTimer() {
    Timer timer = {};
    QueryPerformanceCounter(&timer.start);
    return timer;
}

double StopTimer(Timer timer) {
    LARGE_INTEGER stop;
    QueryPerformanceCounter(&stop);
    
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    
    LONGLONG elapsedParts = stop.QuadPart - timer.start.QuadPart;
    double time = (elapsedParts * 1000.0f) / frequency.QuadPart; 
    return time;
}

#endif

#endif  //_PROFILER_TIMER_H_