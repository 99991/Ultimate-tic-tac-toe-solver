#pragma once

// Can't use std::chrono because "high resolution" clock with gcc on windows
// only has 1 millisecond resolution
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct Timer {
    LARGE_INTEGER t0;
    LARGE_INTEGER frequency;

    Timer(){
        QueryPerformanceFrequency(&frequency);
        stop();
    }

    double stop(){
        LARGE_INTEGER t1;
        QueryPerformanceCounter(&t1);
        double dt = (t1.QuadPart - t0.QuadPart)/(double)frequency.QuadPart;
        t0 = t1;
        return dt;
    }
};
#else
#include <time.h>

struct Timer {
    struct timespec t0;

    Timer(){
        stop();
    }

    double stop(){
        struct timespec t1;
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double dt = t1.tv_sec - t0.tv_sec + 1e-9*(t1.tv_nsec - t0.tv_nsec);
        t0 = t1;
        return dt;
    }
};
#endif
