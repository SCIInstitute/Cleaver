#ifndef __STATUS_H__
#define __STATUS_H__

//@author Brig Bagey
//@date 14 Jan 2015
//@descrip This file is a simple "print progress" for verbose status of a process's progress.
//
#include <stdio.h>
#include <stdlib.h>

class Status {
  public:
    Status(int tot) : total_(double(tot)), count_(0), percent_(-1) {
    } ;
    ~Status() {}
    void printStatus() {
      double pcnt = double(count_) / total_;
      int val = int(std::min(1.,std::max(0.,pcnt))*100.+0.5);
      if (val != percent_) {
        percent_ = val;
        printf("\r|");
        for(int i = 0; i < 50; i++) 
          if(i < val/2) printf("-");
          else printf(" ");
        printf("| %d%%",val);
        fflush(stdout);
      }
      count_++;
    }
    void done() { printf("\n"); percent_ = 0; count_ = 0;}
  private:
    double total_;
    int count_;
    int percent_;
};
#endif
