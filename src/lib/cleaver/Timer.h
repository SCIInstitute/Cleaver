//=====================================================
//  Copyright (c) 2012,  Jonathan Bronson
//
//  This source code is not to be used, copied,
//  or  shared,  without explicit permission from
//  the author.
//=====================================================

#ifndef CLEAVER_TIMER_H
#define CLEAVER_TIMER_H

namespace cleaver
{
class TimerImp;

class Timer
{
public:
    Timer();
    ~Timer();

    void start();
    void stop();
    void reset();
    double time() const;

private:
    TimerImp *m_pimpl;
};

}

#endif // CLEAVER_TIMER_H
