//=========================================================================
//  cg_solver.cu
//
//  This file contains the cuda code for the cg solver
//
//  Author: Jihwan Kim
//  CS6963, Spring 2011
//  Final Project
//=========================================================================

#include <stdio.h>
#include <iostream>
#include <cuda.h>
#include <cutil.h>
#include "cg_constants.h"


__device__ void laplacian2DSharedTwo(int index, int tileNum, float (&sx)[SHARED_WIDTH][SHARED_WIDTH], 
              float (&sx_next)[TILE_WIDTH][TILE_WIDTH], float* x, int oneSideLength, int by, int tx, int ty){
   float val = 0;
              
      if(tileNum == 0 ){
         sx[ty+1][tx+1] = x[index];

         syncthreads();
                  
         if(ty == 0){
            if(by == 0){
               sx[0][tx+1] = 0;//sx[2][tx+1]; //top
            }else{
               sx[0][tx+1] = x[index - oneSideLength];
            }
         }
         
         if(ty == TILE_WIDTH -1){            
            sx[tx+1][0] = 0;//sx[tx+1][2]; //left
            if(by < gridDim.y-1){
               sx[TILE_WIDTH + 1][tx+1] = x[index + oneSideLength];//bottom
            }else{
               sx[TILE_WIDTH + 1][tx+1] = 0;//sx[TILE_WIDTH-1][tx+1];
            }
         }
      }else{
         if(ty == 0){
            sx[tx+1][0] = sx[tx+1][TILE_WIDTH]; //left
         }
         
         syncthreads();
         sx[ty+1][tx+1] = sx_next[ty][tx];

         syncthreads();
         if(ty == 0){
            if(by == 0){
               sx[0][tx+1] = 0;//sx[2][tx+1]; //top
            }else{
               sx[0][tx+1] = x[index - oneSideLength];
            }            
         }
         
         if(ty == TILE_WIDTH -1){
            if(by < gridDim.y-1){
               sx[TILE_WIDTH + 1][tx+1] = x[index + oneSideLength];//bottom
            }else{
               sx[TILE_WIDTH + 1][tx+1] = 0;//sx[TILE_WIDTH-1][tx+1];
            }
         }
         
      }
      
      if(tileNum < oneSideLength/TILE_WIDTH - 1){
         val = x[index+TILE_WIDTH];
         sx_next[ty][tx] = val;
         if(tx == 0){
            sx[ty+1][TILE_WIDTH +1] = val;//sx_next[ty][0];
         }
         
      }else if(ty == (TILE_WIDTH -1)){
         sx[tx+1][TILE_WIDTH+1] = 0;//sx[tx+1][TILE_WIDTH-1];//right
      }
      /*
      syncthreads();

      if(ty == (TILE_WIDTH -1) && tileNum < oneSideLength/TILE_WIDTH - 1){
         sx[tx+1][TILE_WIDTH+1] = sx_next[tx][0]; //right
      }else if(ty == (TILE_WIDTH -1) && tileNum == oneSideLength/TILE_WIDTH - 1){
         sx[tx+1][TILE_WIDTH+1] = 0;//sx[tx+1][TILE_WIDTH-1];//right
      }
      */
      syncthreads();
      
}



__device__ void laplacian2DShared(int index, int tileNum, float (&sx)[SHARED_WIDTH][SHARED_WIDTH], 
              /*float (&sx_next)[TILE_WIDTH][TILE_WIDTH],*/ float* x, int oneSideLength, int by, int tx, int ty){
              
      if(tileNum == 0 ){
         sx[ty+1][tx+1] = x[index];

         syncthreads();
                  
         if(ty == 0){
            if(by == 0){
               sx[0][tx+1] = 0;//sx[2][tx+1]; //top
            }else{
               sx[0][tx+1] = x[index - oneSideLength];
            }
         }
         
         if(ty == TILE_WIDTH -1){            
            sx[tx+1][0] = 0;//sx[tx+1][2]; //left
            if(by < gridDim.y-1){
               sx[TILE_WIDTH + 1][tx+1] = x[index + oneSideLength];//bottom
            }else{
               sx[TILE_WIDTH + 1][tx+1] = 0;//sx[TILE_WIDTH-1][tx+1];
            }
         }
      }else{
         if(ty == 0){
            sx[tx+1][0] = sx[tx+1][TILE_WIDTH]; //left
         }
         
         syncthreads();
         sx[ty+1][tx+1] = x[index];   //sx_next[ty][tx];

         syncthreads();
         if(ty == 0){
            if(by == 0){
               sx[0][tx+1] = 0;//sx[2][tx+1]; //top
            }else{
               sx[0][tx+1] = x[index - oneSideLength];
            }            
         }
         
         if(ty == TILE_WIDTH -1){
            if(by < gridDim.y-1){
               sx[TILE_WIDTH + 1][tx+1] = x[index + oneSideLength];//bottom
            }else{
               sx[TILE_WIDTH + 1][tx+1] = 0;//sx[TILE_WIDTH-1][tx+1];
            }
         }
         
      }
      /*
      if(tileNum < oneSideLength/TILE_WIDTH - 1){
         sx_next[ty][tx] = x[index+TILE_WIDTH];
      }*/
             
//      syncthreads();

      if(ty == (TILE_WIDTH -1) && tileNum < oneSideLength/TILE_WIDTH - 1){
         int ind=index-tx + (TILE_WIDTH);   
         
         sx[tx+1][TILE_WIDTH+1] = x[ind - (TILE_WIDTH-tx-1)*oneSideLength];    //sx_next[tx][0]; //right
      }else if(ty == (TILE_WIDTH -1) && tileNum == oneSideLength/TILE_WIDTH - 1){
         sx[tx+1][TILE_WIDTH+1] = 0;//sx[tx+1][TILE_WIDTH-1];//right
      }
            
      syncthreads();
      
}


__global__ void residual_init_laplacian2D(float *b, float* x, float* r, float* innerR, int oneSideLength, int tileWith_oneSideWidth){
   __shared__ float sx[SHARED_WIDTH][SHARED_WIDTH];
//   __shared__ float sx_next[TILE_WIDTH][TILE_WIDTH];
   
   int by = blockIdx.y;
   int tx = threadIdx.x;
   int ty = threadIdx.y;
   int index = 0;
   float residual = 0;
   
   int ind = by*tileWith_oneSideWidth + ty*oneSideLength + tx;   
   
   for(int tileNum = 0; tileNum < oneSideLength/TILE_WIDTH; tileNum++){
      index = ind + tileNum*TILE_WIDTH; 
      
      laplacian2DShared(index, tileNum, sx, /*sx_next,*/ x, oneSideLength, by, tx, ty);
//      laplacian2DSharedTwo(index, tileNum, sx, sx_next, x, oneSideLength, by, tx, ty);      

      residual = b[index] + (sx[ty+1][tx] + sx[ty+1][tx+2] -4*sx[ty+1][tx+1] + sx[ty][tx+1] + sx[ty+2][tx+1]);

      r[index] = residual;
      innerR[index] = residual*residual;
   }
}



__global__ void laplacian2D(float* x, float* answer, float* answerSq, int oneSideLength, int tileWith_oneSideWidth){
   __shared__ float sx[SHARED_WIDTH][SHARED_WIDTH];
//   __shared__ float sx_next[TILE_WIDTH][TILE_WIDTH];
   
   int by = blockIdx.y;
   int tx = threadIdx.x;
   int ty = threadIdx.y;
   int index = 0;
   float residual = 0;
   int ind = by*tileWith_oneSideWidth + ty*oneSideLength + tx;   
   for(int tileNum = 0; tileNum < oneSideLength/TILE_WIDTH; tileNum++){
      index = ind + tileNum*TILE_WIDTH;
      
      laplacian2DShared(index, tileNum, sx, /*sx_next,*/ x, oneSideLength, by, tx, ty);
//      laplacian2DSharedTwo(index, tileNum, sx, sx_next, x, oneSideLength, by, tx, ty);

      residual = -(sx[ty+1][tx] + sx[ty+1][tx+2] -4*sx[ty+1][tx+1] + sx[ty][tx+1] + sx[ty+2][tx+1]);

      answer[index] = residual;
      answerSq[index] = residual*sx[ty+1][tx+1];
   }
}


__global__ void arraySqSum(float* array, float* sumArray, int oneSideLength, int tileWith_oneSideWidth){
   __shared__ float sArray[TILE_WIDTH][TILE_WIDTH];
   __shared__ float sRowSum[TILE_WIDTH];

   int by = blockIdx.y;
   int tx = threadIdx.x;
   int ty = threadIdx.y;
   int index = 0;
   float rowSum = 0;

   int ind = by*tileWith_oneSideWidth + ty*oneSideLength + tx;   
   for(int tileNum = 0; tileNum < oneSideLength/TILE_WIDTH; tileNum++){
      index = ind + tileNum * TILE_WIDTH;
      sArray[ty][tx] = array[index] * array[index];


      for(unsigned int stride = blockDim.x >>1; stride >=1; stride = stride>>1){
         syncthreads();
         if(tx < stride){
            sArray[ty][tx] += sArray[ty][tx+stride];
         }
      }


      if(tx == 0){
         rowSum += sArray[ty][0];
      }

      syncthreads();
   }

   if(tx == 0){
      sRowSum[ty] = rowSum;
   }

   syncthreads();

   if(ty == 0){
      for(unsigned int stride = blockDim.x >>1; stride >=1; stride = stride>>1){
         syncthreads();
         if(tx < stride){
            sRowSum[tx] += sRowSum[tx+stride];
         }
      }

      syncthreads();

      if(tx == 0){
         sumArray[by] = sRowSum[0];
      }
   }
}



__global__ void arraySum(float* array, float* sumArray, int oneSideLength, int tileWith_oneSideWidth){
   __shared__ float sArray[TILE_WIDTH][TILE_WIDTH];
   __shared__ float sRowSum[TILE_WIDTH];
   
   int by = blockIdx.y;
   int tx = threadIdx.x;
   int ty = threadIdx.y;
   int index = 0;
   float rowSum = 0;

   int ind = by*tileWith_oneSideWidth + ty*oneSideLength + tx;   
   for(int tileNum = 0; tileNum < oneSideLength/TILE_WIDTH; tileNum++){
      index = ind + tileNum * TILE_WIDTH;   
      sArray[ty][tx] = array[index];
      
      
      for(unsigned int stride = blockDim.x >>1; stride >=1; stride = stride>>1){
         syncthreads();
         if(tx < stride){
            sArray[ty][tx] += sArray[ty][tx+stride];
         }
      }


      if(tx == 0){
         rowSum += sArray[ty][0];
      }
         
      syncthreads();      
   }
   
   if(tx == 0){
      sRowSum[ty] = rowSum;
   }
   
   syncthreads();      
   
   if(ty == 0){
      for(unsigned int stride = blockDim.x >>1; stride >=1; stride = stride>>1){
         syncthreads();
         if(tx < stride){
            sRowSum[tx] += sRowSum[tx+stride];
         }
      }
      
      syncthreads();
      
      if(tx == 0){
         sumArray[by] = sRowSum[0];
      }
   }
}

//Should run by one block
__device__ float sumOfArray(int tx, int blockDimX, float* a){
   __shared__ float sInner[MAX_N/TILE_WIDTH];

   float innerSum = 0.0f;

   sInner[tx] = a[tx];

   syncthreads();
         
   for(unsigned int stride = blockDimX>>1; stride >=1; stride = stride>>1){
      syncthreads();
      if(tx < stride){
         sInner[tx] += sInner[tx+stride];
      }
   }

   if(tx == 0){
      innerSum += sInner[0];
   }
   
   return innerSum;
}


//Should run by one block
__global__ void arrayOneDSum(float* a, float* sum){
   float prod = 0;
   int tx = threadIdx.x;

   prod = sumOfArray(tx, blockDim.x, a);
   
   syncthreads();  
   
   if(tx == 0){
      sum[0]  = prod;
   }
}


__global__ void arrayOneDSum(float* a, float* nominator, float* sum){
   float prod = 0;
   int tx = threadIdx.x;

   prod = sumOfArray(tx, blockDim.x, a);
   
   syncthreads();  
   
   if(tx == 0){
      sum[0]  = nominator[0]/prod;
   }
}


__global__ void arrayOneDSum(float* a, float* denominator, float* sum, float* bnorm, float tolerance){
   float prod = 0;

   int tx = threadIdx.x;

   prod = sumOfArray(tx, blockDim.x, a);         
   syncthreads();  
   
   if(tx == 0){
      sum[0]  = prod/denominator[0];
      denominator[0] = prod;
      
      if(bnorm != NULL && prod/bnorm[0] < tolerance){
         bnorm[0] = -1;
      }
   }
}


__global__ void innerCG_XR(float* dr,  float* dx, float* dd, float* dq, float* drSq, float* dalpha, int oneSideLength, int tileWith_oneSideWidth){ 
   float alpha = dalpha[0];
   
   int index = blockIdx.y*tileWith_oneSideWidth + threadIdx.y*oneSideLength + blockIdx.x*TILE_WIDTH + threadIdx.x;

   dx[index] = dx[index] + alpha*dd[index];
   float newR = dr[index] - alpha*dq[index];
   dr[index] = newR;
   drSq[index] = newR*newR;
}


__global__ void innerCG_D(float* dr,  float* dd, float* dbeta, int oneSideLength, int tileWith_oneSideWidth){ 
   float beta = dbeta[0];
   
   int index = blockIdx.y*tileWith_oneSideWidth + threadIdx.y*oneSideLength + blockIdx.x*TILE_WIDTH + threadIdx.x;

   dd[index] = dr[index] + beta*dd[index];
}





float cg_solver(float* b, float* x, int max_iter, float tolerance, float* answer, const int width){

   float* db;
   float* dx;
   float* dresidual;
   float* danswerSq;
   float* sumPerBlock;
   float* dd;
   float* dq;
   float* ddelta;
   float* dbnorm;
   float* dalpha;
   float* dbeta;
   const int tileWith_oneSideWidth = width * TILE_WIDTH;
   
   cudaEvent_t start_event, stop_event;
   float my_elapsed_time = 0;
   float sumTime = 0;
   
   cudaError_t error;

   cudaMalloc((void**)&db, sizeof(float) * width*width);
   cudaMalloc((void**)&dx, sizeof(float) * width*width);
   cudaMalloc((void**)&dresidual, sizeof(float) * width*width);
   cudaMalloc((void**)&danswerSq, sizeof(float) * width*width);
   cudaMalloc((void**)&dd, sizeof(float) * width*width);
   cudaMalloc((void**)&dq, sizeof(float) * width*width);
   cudaMalloc((void**)&sumPerBlock, sizeof(float)*width/TILE_WIDTH);
   
   cudaMalloc((void**)&ddelta, sizeof(float));
   cudaMalloc((void**)&dbnorm, sizeof(float));
   cudaMalloc((void**)&dalpha, sizeof(float));
   cudaMalloc((void**)&dbeta, sizeof(float));

   cudaMemcpy(db, b, sizeof(float) * width*width, cudaMemcpyHostToDevice);
   cudaMemcpy(dx, x, sizeof(float) * width*width, cudaMemcpyHostToDevice);



   //Performance Measure event
   CUDA_SAFE_CALL( cudaEventCreate(&start_event));
   CUDA_SAFE_CALL( cudaEventCreate(&stop_event));

   // start the timer for GPU code
   cudaEventRecord(start_event, 0);


   dim3 dimGrid(1, width/TILE_WIDTH, 1);
   dim3 dimBlock(TILE_WIDTH, TILE_WIDTH, 1);

   residual_init_laplacian2D<<<dimGrid, dimBlock>>>(db, dx, dresidual, danswerSq, width, tileWith_oneSideWidth);
   error = cudaThreadSynchronize();

   error = cudaGetLastError();
    if( cudaSuccess != error)
    {
        printf("Cuda error: %s.\n", cudaGetErrorString( error) );
        return 0;
    }

   arraySqSum<<<dimGrid, dimBlock>>>(db, sumPerBlock, width, tileWith_oneSideWidth);
   error = cudaThreadSynchronize(); 

   dimGrid.y = 1;
   dimBlock.x = (width/TILE_WIDTH);  //Assume width/TILE_WIDTH is less than 512 and width/TILE_WIDTH is also 2^n.
   dimBlock.y = 1;
   
   arrayOneDSum<<<dimGrid, dimBlock>>>(sumPerBlock, dbnorm);
   error = cudaThreadSynchronize();

   cudaMemcpy(dd, dresidual, sizeof(float) * width*width, cudaMemcpyDeviceToDevice);

   dimGrid.y = width/TILE_WIDTH;
   dimBlock.x = TILE_WIDTH;  
   dimBlock.y = TILE_WIDTH;
      
   arraySum<<<dimGrid, dimBlock>>>(danswerSq, sumPerBlock, width, tileWith_oneSideWidth);
   error = cudaThreadSynchronize();
   
   dimGrid.y = 1;
   dimBlock.x = (width/TILE_WIDTH);  //Assume width/TILE_WIDTH is less than 512 and width/TILE_WIDTH is also 2^n.
   dimBlock.y = 1;
   
   arrayOneDSum<<<dimGrid, dimBlock>>>(sumPerBlock, ddelta, dbeta, NULL, 0);
   error = cudaThreadSynchronize();

   int iter = 0;
   for (iter = 0; iter<max_iter; iter++){
           
      dimGrid.x = 1;
      dimGrid.y = width/TILE_WIDTH;

      dimBlock.x = TILE_WIDTH;  
      dimBlock.y = TILE_WIDTH;
      
      laplacian2D<<<dimGrid, dimBlock>>>(dd, dq, danswerSq, width, tileWith_oneSideWidth);
      error = cudaThreadSynchronize();

      arraySum<<<dimGrid, dimBlock>>>(danswerSq, sumPerBlock, width, tileWith_oneSideWidth);
      error = cudaThreadSynchronize();
   
      dimGrid.y = 1;
      dimBlock.x = (width/TILE_WIDTH);  //Assume width/TILE_WIDTH is less than 512 and width/TILE_WIDTH is also 2^n.
      dimBlock.y = 1;
   
      arrayOneDSum<<<dimGrid, dimBlock>>>(sumPerBlock, ddelta, dalpha);
      error = cudaThreadSynchronize();

      dimGrid.x = width/TILE_WIDTH;
      dimGrid.y = width/TILE_WIDTH;
      dimBlock.x = TILE_WIDTH;  
      dimBlock.y = TILE_WIDTH;
          
      innerCG_XR<<<dimGrid, dimBlock>>>(dresidual, dx, dd, dq, danswerSq, dalpha, width, tileWith_oneSideWidth);
      error = cudaThreadSynchronize();

      dimGrid.x = 1;
      arraySum<<<dimGrid, dimBlock>>>(danswerSq, sumPerBlock, width, tileWith_oneSideWidth);
      error = cudaThreadSynchronize();
   
      dimGrid.y = 1;
      dimBlock.x = (width/TILE_WIDTH);  //Assume width/TILE_WIDTH is less than 512 and width/TILE_WIDTH is also 2^n.
      dimBlock.y = 1;
   
      arrayOneDSum<<<dimGrid, dimBlock>>>(sumPerBlock, ddelta, dbeta, dbnorm, tolerance);
      error = cudaThreadSynchronize();
           
      cudaMemcpy(answer, dbnorm, sizeof(float), cudaMemcpyDeviceToHost);
      if(answer[0] < 0){
         break;
      }
      
      dimGrid.x = width/TILE_WIDTH;
      dimGrid.y = width/TILE_WIDTH;
      dimBlock.x = TILE_WIDTH;  
      dimBlock.y = TILE_WIDTH;
            
      innerCG_D<<<dimGrid, dimBlock>>>(dresidual, dd, dbeta, width, tileWith_oneSideWidth);
      error = cudaThreadSynchronize();       
   }

    // stop timing
    cudaEventRecord(stop_event, 0);
    cudaEventSynchronize(stop_event);

    CUDA_SAFE_CALL( cudaEventElapsedTime(&my_elapsed_time, start_event, stop_event));

    std::cout<< "Break the loop after "<< iter <<" iteration" << std::endl;

    my_elapsed_time = my_elapsed_time/iter; //sumTime;
   
   cudaMemcpy(answer, dx, sizeof(float) * width*width, cudaMemcpyDeviceToHost);

/*
   printf("answer in the cuda: \n");
   for(i=0; i<width*width; i++){
       printf("%f, ", answer[i]);
   }
   printf("\n");
*/
   cudaFree(db);
   cudaFree(dx);
   cudaFree(dresidual);
   cudaFree(danswerSq);
   cudaFree(sumPerBlock);
   cudaFree(dd);
   cudaFree(dq);
   cudaFree(ddelta);
   cudaFree(dbnorm);
   cudaFree(dalpha);
   cudaFree(dbeta);   

   return my_elapsed_time;
}

