//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
// Cleaver - A MultiMaterial Tetrahedral Mesher
// -- Generalized Stencil Table
//
//  Author: Jonathan Bronson (bronson@sci.utah.edu)
//
//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
//  Copyright (C) 2011, 2012, Jonathan Bronson
//  Scientific Computing & Imaging Institute
//  University of Utah
//
//  Permission is  hereby  granted, free  of charge, to any person
//  obtaining a copy of this software and associated documentation
//  files  ( the "Software" ),  to  deal in  the  Software without
//  restriction, including  without limitation the rights to  use,
//  copy, modify,  merge, publish, distribute, sublicense,  and/or
//  sell copies of the Software, and to permit persons to whom the
//  Software is  furnished  to do  so,  subject  to  the following
//  conditions:
//
//  The above  copyright notice  and  this permission notice shall
//  be included  in  all copies  or  substantial  portions  of the
//  Software.
//
//  THE SOFTWARE IS  PROVIDED  "AS IS",  WITHOUT  WARRANTY  OF ANY
//  KIND,  EXPRESS OR IMPLIED, INCLUDING  BUT NOT  LIMITED  TO THE
//  WARRANTIES   OF  MERCHANTABILITY,  FITNESS  FOR  A  PARTICULAR
//  PURPOSE AND NONINFRINGEMENT. IN NO EVENT  SHALL THE AUTHORS OR
//  COPYRIGHT HOLDERS  BE  LIABLE FOR  ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
//  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
//  USE OR OTHER DEALINGS IN THE SOFTWARE.
//-------------------------------------------------------------------
//-------------------------------------------------------------------

#ifndef STENCILTABLE_H
#define STENCILTABLE_H


// MACRO's to simplify Stencil Tables
#ifndef _O
#define _O -1        // ___      NO MORE VERTICES
#define _A 0         //
#define _B 1         //     \__  Lattice Vertices
#define _C 2         //     /
#define _D 3         // ___/
                     // ___
#define _AB 4        //
#define _AC 5        //
#define _AD 6        //      \__ Cutpoint Vertices
#define _BC 7        //      /
#define _CD 9        //
#define _BD 8        // ___/
                     // ___
#define _ABC 13      //
#define _ACD 11      //     \__  TriplePoint Vertices
#define _ABD 12      //     /
#define _BCD 10      // ___/

#define _ABCD 14     // QuadPoint Vertex

#endif

const int stencilTable[24][4] =

{{_ABCD,_AB ,_ABD,_A},
 {_A,_AB ,_ABC,_ABCD},
 {_ABCD,_AC,_ABC,_A},
 {_A,_AD,_ABD,_ABCD},
 {_ABCD,_AD,_ACD,_A},
 {_A,_AC,_ACD,_ABCD},
 {_B,_BD,_BCD,_ABCD},
 {_ABCD,_BD,_ABD,_B},
 {_B,_AB ,_ABD,_ABCD},
 {_ABCD,_BC,_BCD,_B},
 {_B,_BC,_ABC,_ABCD},
 {_ABCD,_AB ,_ABC,_B},
 {_C,_BC,_BCD,_ABCD},
 {_ABCD,_BC,_ABC,_C},
 {_C,_AC,_ABC,_ABCD},
 {_ABCD,_CD,_BCD,_C},
 {_C,_CD,_ACD,_ABCD},
 {_ABCD,_AC,_ACD,_C},
 {_D,_CD,_BCD,_ABCD},
 {_ABCD,_CD,_ACD,_D},
 {_D,_AD,_ACD,_ABCD},
 {_ABCD,_BD,_BCD,_D},
 {_D,_BD,_ABD,_ABCD},
 {_ABCD,_AD,_ABD,_D}};

const int materialTable[24] = {_A,_A,_A,_A,_A,_A, _B,_B,_B,_B,_B,_B, _C,_C,_C,_C,_C,_C, _D,_D,_D,_D,_D,_D};

const int completeInterfaceTable[12][2] = {{_AB, _ABC},
                                           {_AB, _ABD},
                                           {_AC, _ABC},
                                           {_AC, _ACD},
                                           {_AD, _ABD},
                                           {_AD, _ACD},
                                           {_BC, _ABC},
                                           {_BC, _BCD},
                                           {_BD, _ABD},
                                           {_BD, _BCD},
                                           {_CD, _ACD},
                                           {_CD, _BCD}};


#endif // STENCILTABLE_H
