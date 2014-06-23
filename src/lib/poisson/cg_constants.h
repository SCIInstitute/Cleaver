//=========================================================================
//  cg_constants.h
//
//  This file defines various constants used in the cuda cg solver.
//
//  Author: Jihwan Kim
//  CS6963, Spring 2011
//  Final Project
//=========================================================================

#ifdef __cplusplus
extern "C" {
#endif

float cg_solver(float* b, float* x, int max_iter, float tolerance, float* answer, const int width );

#ifdef __cplusplus
}
#endif

#define MAX_N 2048
/*
#define blockWidth 2
#define threadsInWarp 2
#define numOfBlock  N/blockWidths
*/

#define TILE_WIDTH 16
#define SHARED_WIDTH (TILE_WIDTH+2)

