//=========================================================================
//  Solver.h
//
//  This file is the header to the Solver Base Class. This class is
//  intended to unify all solvers under a common api, for polymorphism.
//
//  Author: Jonathan Bronson
//  CS6963, Spring 2011
//  Final Project
//=========================================================================


#ifndef __SOLVER_H__
#define __SOLVER_H__

class Solver
{
public:
    virtual bool runIteration() = 0;
};

#endif // __SOLVER_H__
