/* Copyright (C) 2005-2011 M. T. Homer Reid
 *
 * This file is part of SCUFF-EM.
 *
 * SCUFF-EM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * SCUFF-EM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * PBCGeometry.h  -- header file for libscuff implementation of 
 *                -- periodic boundary conditions
 *
 * homer reid  -- 4/2011 -- 7/2012
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <libhrutil.h>
#include <libMDInterp.h>
#include <libIncField.h>
#include <libscuff.h>

#ifndef PBCGEOMETRY_H
#define PBCGEOMETRY_H

namespace scuff{

/***************************************************************/
/* PBCGeometry is a class built atop RWGGeometry for handling  */
/* periodic boundary conditions.                               */
/***************************************************************/
class PBCGeometry
 {
   /*--------------------------------------------------------------*/
   /* public class methods                                        -*/
   /*--------------------------------------------------------------*/
public:

   // constructor / destructor.
   // LBV[i][j] = jth component of ith lattice basis vector (i,j = 0,1)
   PBCGeometry(RWGGeometry *G, double **LBV);
   ~PBCGeometry();

   // the routines for allocating the BEM matrix, allocating the RHS vector,
   // and assembling the RHS vector all simply devolve to calls to the 
   // corresponding RWGGeometry routines, but they deserve status as 
   // PBCGeometry class methods to encourage users not to call them
   // before the PBCGeometry class constructor has been invoked; the
   // reason this is important is that the size of the linear system
   // (i.e. the number of interior edges in the unit-cell surface mesh)
   // may differ before and after the call to the PBCGeometry constructor 
   // due to the addition of straddlers.
   HMatrix *AllocateBEMMatrix(int PureImagFreq = false)
    { return G->AllocateBEMMatrix(PureImagFreq); }
   HVector *AllocateRHSVector(int PureImagFreq = false)
    { return G->AllocateRHSVector(PureImagFreq); }
   HVector *AssembleRHSVector(cdouble Omega, IncField *IF, HVector *RHS)
    { return G->AssembleRHSVector(Omega, IF, RHS); }

   // this routine does not simply devolve to an RWGGeometry call.
   HMatrix *AssembleBEMMatrix(cdouble Omega, double *BlochP, HMatrix *M=0);

   // assemble RHS vector; this actually just devolves to a call to the
   // usual RWGGeometry routine for assembling the BEM matrix, but i will
   // make it a PBCGeometry routine for completeness


   // get fields
   HMatrix *GetFields(IncField *IF, HVector *KN,
                      cdouble Omega, double *BlochP,
                      HMatrix *XMatrix, HMatrix *FMatrix=0,
                      char *FuncString=0, int nThread=0);
 
   void GetFields(IncField *IF, HVector *KN, 
                  cdouble Omega, double *BlochP,
                  double *X, cdouble *EH, int nThread=0);

   /*--------------------------------------------------------------*/
   /*- class data which would be private if we were fastidious     */
   /*-            about such things)                               */
   /*--------------------------------------------------------------*/
   RWGGeometry *G;       // unit cell geometry, augmented to include straddlers

   double LBV[2][2];     // lattice basis vectors 

   // NumStraddlers[ 2*no+i ] = the number of basis functions on object #no
   //                           that straddle unit-cell boundary #i
   int *NumStraddlers;  

   // contributions to the BEM matrix from lattice sites 
   // (n1,n2) = { (+1,+1), (+1,-1), (+1,0), (0,+1), (0,0) }
   // (P, M, Z = 'plus 1, minus 1, zero')
   HMatrix *MPP, *MPM, *MPZ, *MZP, *MZZ;

   // this field stores the value of Omega that was passed to 
   // the most recent invocation of AssembleBEMMatrix(). we store it 
   // because, if the user makes a second call to AssembleBEMMatrix() 
   // with the same value of Omega (but presumably a different value
   // of the bloch vector P), then we can reuse the MPP...MZZ matrix 
   // blocks. 
   cdouble CurrentOmega;
   cdouble *EpsTF; // EpsTF[0] = epsilon of exterior medium at this frequency
   cdouble *MuTF;  // EpsTF[no+1] = epsilon of object #no at this frequency

   /* interpolation tables to accelerate the calculation of */
   /* the periodic green's function for the exterior medium */
   /* and for the medium interior to each object            */
   Interp3D *GBarAB9_Exterior;
   Interp3D **GBarAB9_Interior;

   // maximum and minimum values of cartesian coordinates of 
   // any object in the geometry, used to determine the ranges 
   // that must be covered by interpolation tables
   double RMax[3], RMin[3];

   /*--------------------------------------------------------------*/
   /*- class methods which would be private if we were fastidious  */
   /*- about such things; these are helper routines for the public */
   /*- class methods                                               */
   /*--------------------------------------------------------------*/
   void AddStraddlers(RWGSurface *O, double **LBV, int *NumStraddlers);
   void AssembleInnerCellBlocks();
   void AddOuterCellContributions(double *BlochP, HMatrix *M);

   /*--------------------------------------------------------------*/
   /*--------------------------------------------------------------*/
   /*--------------------------------------------------------------*/
   static int TriangleCubatureOrder;
   static double DeltaInterp;

 };

/***************************************************************/
/* routine for computing the periodic green's function via     */
/* ewald summation                                             */
/***************************************************************/
void GBarVDEwald(double *R, cdouble k, double *BlochP, double **LBV,
                 double E, int ExcludeFirst9, cdouble *GBarVD);

/***************************************************************/
/* this is an alternative interface to GBarVDEwald that has the*/
/* proper prototype for passage to my Interp3D class routines  */
/***************************************************************/
typedef struct GBarData 
 { 
   cdouble k;           // wavenumber 
   double *BlochP;      // bloch vector 
   double *LBV[2];      // lattice basis vectors 
   double E;            // ewald separation parameter
   bool ExcludeInner9;  
 
 } GBarData;

void GBarVDPhi3D(double X1, double X2, double X3, 
                 void *UserData, double *PhiVD);

/***************************************************************/
/***************************************************************/
/***************************************************************/
void GetAB9EdgeEdgeInteractions(RWGSurface *Oa, int nea, RWGSurface *Ob, int neb,
                                cdouble k, Interp3D *Interpolator, cdouble *GC);

} // namespace scuff
#endif
