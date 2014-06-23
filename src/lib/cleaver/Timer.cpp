//=====================================================
//  Copyright (c) 2012,  Jonathan Bronson
//
//  This source code is not to be used, copied,
//  or  shared,  without explicit permission from
//  the author.
//=====================================================

#include "Timer.h"
#include <ctime>
#include <stdint.h>

#ifdef WIN32
#define NOMINMAX
 #include <Windows.h>
#else
 #include <sys/time.h>
#endif

#ifndef int64_t
  #ifdef WIN32
    #define int64_t __int64  // VS 2008
  #endif
#endif

#ifndef uint64
  #ifdef WIN32
    #define uint64 unsigned __int64  // VS 2008
  #endif
#endif


namespace{


int64_t GetTime()
{
#ifdef WIN32
 /* Windows */
 FILETIME ft;
 LARGE_INTEGER li;

 /* Get the amount of 100 nano seconds intervals elapsed since January 1, 1601 (UTC) and copy it
  * to a LARGE_INTEGER structure. */
 GetSystemTimeAsFileTime(&ft);
 li.LowPart = ft.dwLowDateTime;
 li.HighPart = ft.dwHighDateTime;

 uint64 ret = li.QuadPart;
 ret -= 116444736000000000LL; /* Convert from file time to UNIX epoch time. */
 ret /= 10000; /* From 100 nano seconds (10^-7) to 1 millisecond (10^-3) intervals */

 return ret;
#else
 /* Linux */
 struct timeval tv;

 gettimeofday(&tv, NULL);

 uint64_t ret = tv.tv_usec;
 /* Convert from micro seconds (10^-6) to milliseconds (10^-3) */
 ret /= 1000;

 /* Adds the seconds (10^0) after converting them to milliseconds (10^-3) */
 ret += (tv.tv_sec * 1000);

 return ret;
#endif
}

}

namespace cleaver
{

class TimerImp
{
public:
    TimerImp(){}

    int64_t begin_time;
    int64_t   end_time;
    double  total_time;

    bool active;
};

Timer::Timer() : m_pimpl(new TimerImp())
{
    reset();
}

Timer::~Timer()
{
    delete m_pimpl;
}

void Timer::start()
{
    m_pimpl->begin_time = GetTime();
    m_pimpl->active = true;
}

void Timer::stop()
{
    m_pimpl->active = false;
    m_pimpl->end_time = GetTime();
    m_pimpl->total_time = (m_pimpl->end_time - m_pimpl->begin_time)/(double)1000;
}

void Timer::reset()
{
    m_pimpl->total_time = 0;
    m_pimpl->active = false;
}

double Timer::time() const
{
    if(!m_pimpl->active)
        return m_pimpl->total_time;
    else {
        int64_t current_time = GetTime();
        return (current_time - m_pimpl->begin_time)/(double)1000;
    }
}

} // namespace cleaver
