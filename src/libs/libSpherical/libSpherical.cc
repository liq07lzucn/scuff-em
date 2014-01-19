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
 * libSpherical.cc -- a collection of various utilities useful for 
 *                    working with spherical coordinates 
 *
 * homer reid      -- 4/2005 -- 2/2010
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <libhrutil.h>

#include "libSpherical.h"
#include "AmosBessel.h"

#define II cdouble(0.0,1.0)
#define ROOT2 1.41421356237309504880

/***************************************************************/
/* convert cartesian coordinates to spherical coordinates      */
/***************************************************************/
void CoordinateC2S(double X, double Y, double Z, double *r, double *Theta, double *Phi)
{ 
  *r=sqrt(X*X + Y*Y + Z*Z);
  *Theta=atan2( sqrt(X*X+Y*Y), Z );
  *Phi=atan2(Y,X);
}

void CoordinateC2S(double X[3], double *r, double *Theta, double *Phi)
 { CoordinateC2S(X[0], X[1], X[2], r, Theta, Phi); }

void CoordinateC2S(double X[3], double R[3])
 { CoordinateC2S(X[0], X[1], X[2], R+0, R+1, R+2); }

/***************************************************************/
/* convert spherical coordinates to cartesian coordinates      */
/***************************************************************/
void CoordinateS2C(double r, double Theta, double Phi, double *X, double *Y, double *Z)
{ 
  double CT=cos(Theta), ST=sin(Theta);
  double CP=cos(Phi), SP=sin(Phi);

  *X=r*ST*CP;
  *Y=r*ST*SP;
  *Z=r*CT;
}

void CoordinateS2C(double r, double Theta, double Phi, double X[3])
{ CoordinateS2C(r, Theta, Phi, X+0, X+1, X+2); }

void CoordinateS2C(double R[3], double X[3])
 { CoordinateS2C(R[0], R[1], R[2], X+0, X+1, X+2); }

/***************************************************************/
/* given the cartesian components of a vector, return its      */
/* spherical components.                                       */
/***************************************************************/
void VectorC2S(double Theta, double Phi, cdouble VC[3], cdouble VS[3])
{ 
  double CT=cos(Theta), ST=sin(Theta);
  double CP=cos(Phi), SP=sin(Phi);
  double M[3][3];
  int mu, nu;

  M[0][0]=ST*CP;    M[0][1]=ST*SP;    M[0][2]=CT;
  M[1][0]=CT*CP;    M[1][1]=CT*SP;    M[1][2]=-ST;
  M[2][0]=-SP;      M[2][1]=CP;       M[2][2]=0.0;

  memset(VS,0,3*sizeof(cdouble));
  for(mu=0; mu<3; mu++)
   for(nu=0; nu<3; nu++)
    VS[mu] += M[mu][nu]*VC[nu];
}

void VectorC2S(double Theta, double Phi, double VC[3], double VS[3])
{ 
  double CT=cos(Theta), ST=sin(Theta);
  double CP=cos(Phi), SP=sin(Phi);
  double M[3][3];
  int mu, nu;

  M[0][0]=ST*CP;    M[0][1]=ST*SP;    M[0][2]=CT;
  M[1][0]=CT*CP;    M[1][1]=CT*SP;    M[1][2]=-ST;
  M[2][0]=-SP;      M[2][1]=CP;       M[2][2]=0.0;

  memset(VS,0,3*sizeof(double));
  for(mu=0; mu<3; mu++)
   for(nu=0; nu<3; nu++)
    VS[mu] += M[mu][nu]*VC[nu];
}

/***************************************************************/
/* given the spherical components of a vector, return its      */
/* cartesian components.                                       */
/***************************************************************/
void VectorS2C(double Theta, double Phi, cdouble VS[3], cdouble VC[3])
{ 
  double CT=cos(Theta), ST=sin(Theta);
  double CP=cos(Phi), SP=sin(Phi);
  double M[3][3];
  int mu, nu;

  M[0][0]=ST*CP;    M[0][1]=CT*CP;    M[0][2]=-SP;
  M[1][0]=ST*SP;    M[1][1]=CT*SP;    M[1][2]=CP;
  M[2][0]=CT;       M[2][1]=-ST;      M[2][2]=0.0;

  memset(VC,0,3*sizeof(cdouble));
  for(mu=0; mu<3; mu++)
   for(nu=0; nu<3; nu++)
    VC[mu] += M[mu][nu]*VS[nu];

}

void VectorS2C(double Theta, double Phi, double VS[3], double VC[3])
{ 
  double CT=cos(Theta), ST=sin(Theta);
  double CP=cos(Phi), SP=sin(Phi);
  double M[3][3];
  int mu, nu;

  M[0][0]=ST*CP;    M[0][1]=CT*CP;    M[0][2]=-SP;
  M[1][0]=ST*SP;    M[1][1]=CT*SP;    M[1][2]=CP;
  M[2][0]=CT;       M[2][1]=-ST;      M[2][2]=0.0;

  memset(VC,0,3*sizeof(double));
  for(mu=0; mu<3; mu++)
   for(nu=0; nu<3; nu++)
    VC[mu] += M[mu][nu]*VS[nu];
}

/***************************************************************/
/* spherical harmonics *****************************************/
/***************************************************************/

/*--------------------------------------------------------------*/
/*- legendre functions and derivatives normalized appropriately-*/
/*- for use in spherical harmonics                             -*/
/*-                                                            -*/
/*- Plm[0]      = P_m^m       ----------------------------------*/
/*- Plm[1]      = P_{m+1}^m   ----------------------------------*/
/*- ...                                                        -*/
/*- Plm[lMax-m] = P_{lMax}^m  ----------------------------------*/
/*-                                                            -*/
/*- and PlmPrime is similar for d/dx Plm                       -*/
/*-                                                            -*/
/*- code inspired by that of numerical recipes, 3rd edition.   -*/
/*-                                                            -*/
/*- note: the recurrence relation for d/dx Plm is:             -*/
/*-                                                            -*/
/*- (1-x^2) d/dx P_lm = A(l,m)*P_{l-1}^m - l*x*P_l^m           -*/
/*- where A(l,m) = sqrt( (2l+1)*(l^2-m^2) / (2l-1) )           -*/
/*--------------------------------------------------------------*/
void GetPlm(int lMax, int m, double x, double *Plm, double *PlmPrime)
{
  double omx2=(1.0-x)*(1.0+x);

  /***************************************************************/
  /* compute P_m^m ***********************************************/
  /***************************************************************/
  double pmm=1.0;
  double fact=1.0;
  int n;
  for(n=1; n<=m; n++)
   { pmm*=omx2*fact/(fact+1.0);
     fact+=2.0;
   };
  pmm=sqrt( (2.0*m+1.0)*pmm/(4.0*M_PI) );
  if (m&1) 
   pmm=-pmm;

  /***************************************************************/
  /* assemble the first two slots in the output arrays ***********/
  /***************************************************************/
  double oldfact=sqrt(2.0*m+3.0);
  double l, Alm;

  Plm[0] = pmm;
  Plm[1] = x*oldfact*pmm;
  PlmPrime[0] = -m*x*Plm[0]/omx2;
  l=m+1;
  Alm=sqrt( (2*l+1)*(l*l-m*m) / (2*l-1) );
  PlmPrime[1] = (Alm*Plm[0] - l*x*Plm[1])/omx2;

  /***************************************************************/
  /* use recurrence to fill in the remaining slots               */
  /***************************************************************/
  int np;
  for(np=2, l=m+2; l<=lMax; l++, np++)
   { fact=sqrt( (4.0*l*l-1.0) / (l*l-m*m) );
     Plm[np]=fact*(x*Plm[np-1] - Plm[np-2]/oldfact);
     Alm=sqrt( (2*l+1)*(l*l-m*m) / (2*l-1) );
     PlmPrime[np] = (Alm*Plm[np-1] - l*x*Plm[np])/omx2;
     oldfact=fact;
   };

}

/*--------------------------------------------------------------*/
/*- return the value of a single spherical harmonic ------------*/
/*--------------------------------------------------------------*/
cdouble GetYlm(int l, int m, double Theta, double Phi)
{  
  if ( abs(m)==1 && (fabs(Theta)<1.0e-6 || fabs(Theta-M_PI)<1.0e-6) )
   return 0.0; 

  if (abs(m)>l) 
   ErrExit("%s:%i: abs(m)>l in GetYlm",__FILE__,__LINE__);

  double *Plm      = new double[l]; 
  double *PlmPrime = new double[l];
  double mPhi=m*Phi;
  cdouble RetVal;

  GetPlm(l, abs(m), cos(Theta), Plm, PlmPrime);
  if (m<0)
   RetVal = ((m%1) ? -1.0 : 1.0) * Plm[l-abs(m)] * exp(-1.0*II*mPhi);
  else
   RetVal = Plm[l-m]*exp(II*mPhi);

  delete[] Plm;
  delete[] PlmPrime;

  return RetVal;

}  

/*--------------------------------------------------------------*/
/*- return the value of all spherical harmonics from            */
/*-  (l,m)=(0,0) to (l,m) = (lMax, +lMax)                       */
/*-                                                             */
/*- Ylm must point to a buffer with enough room for at least    */
/*-  (lMax+1)*(lMax+1) cdoubles.                                */
/*-                                                             */
/*  on return Ylm is filled in as follows:                      */
/*   Ylm[0]   = Y_{l=0,m=0}                                     */
/*   Ylm[1]   = Y_{l=1,m=-1}                                    */
/*   Ylm[2]   = Y_{l=1,m=0}                                     */
/*   Ylm[3]   = Y_{l=1,m=+1}                                    */
/*   Ylm[4]   = Y_{l=2,m=-2}                                    */
/*    ...                                                       */
/*   Ylm[N-1] = Y_{l=lMax, m=+lMax}                             */
/*                                                              */
/* where N=(lMax+1)*(lMax+1).                                   */
/*                                                              */
/*  (the entry in the array corresponding to (l,m) is stored in */
/*   slot l*l + l + m )                                         */
/*--------------------------------------------------------------*/
void GetYlmArray(int lMax, double Theta, double Phi, cdouble *Ylm)
{
  GetYlmDerivArray(lMax, Theta, Phi, Ylm, 0);
}

/*--------------------------------------------------------------*/
/*- same as GetYlmArray except that we use real-valued Ylms    -*/
/*-  which are defined like the usual Ylms but with complex    -*/
/*-  exponentials replaced by real-valued sinusoids, i.e.      -*/
/*-   exp(imPhi) -> cos(imPhi) (m>0)                           -*/
/*-   exp(imPhi) -> sin(imPhi) (m<0)                           -*/
/*--------------------------------------------------------------*/
void GetRealYlmArray(int lMax, double Theta, double Phi, double *RealYlm)
{
  int NAlpha=(lMax+1)*(lMax+1);
  cdouble *Ylm = new cdouble[NAlpha];
  GetYlmDerivArray(lMax, Theta, Phi, Ylm, 0);
  for(int l=0; l<=lMax; l++)
   { 
     int Alpha = LM2ALPHA(l,0); 
     RealYlm[ Alpha ] = real( Ylm[Alpha] );

     for(int m=1; m<=l; m++)
      { 
        int AlphaP = LM2ALPHA(l,m); 

        int AlphaM = LM2ALPHA(l,-m); 
        double Sign = (m%2) ? -1.0 : 1.0;
        RealYlm[AlphaP] = real( Ylm[AlphaP] + Sign*Ylm[AlphaM]) / ROOT2;
        RealYlm[AlphaM] = imag( Ylm[AlphaP] - Sign*Ylm[AlphaM]) / ROOT2; 
      };

   };

  delete[] Ylm;
}

double GetRealYlm(int l, int m, double Theta, double Phi)
{
  int Alpha = LM2ALPHA(l,m);
  int NAlpha = (l+1)*(l+1);
  double *RealYlm = new double[NAlpha];
  GetRealYlmArray(l, Theta, Phi, RealYlm);
  double RetVal = RealYlm[Alpha];
  delete[] RealYlm;
  return RetVal;
}

/*--------------------------------------------------------------*/
/*- same as GetYlmArray except that the theta derivatives of   -*/
/*- the Ylms are also computed and returned in dYlmdTheta (if  -*/
/*- dYlmdTheta is non-null)                                    -*/
/*--------------------------------------------------------------*/
void GetYlmDerivArray(int lMax, double Theta, double Phi, 
                      cdouble *Ylm, cdouble *dYlmdTheta)
{
  int l, m, mm, Alpha;

  /* whoops! this doesn't work in C++. for now i will  */
  /* hard-code the value of LMAX...FIXME               */
  // double **Plm      = new double[lMax+1][lMax+1];
  // double **PlmPrime = new double[lMax+1][lMax+1];

  #define LMAXMAX 30
  if (lMax>LMAXMAX) ErrExit("%s:%i: internal error",__FILE__,__LINE__);
  double Plm[LMAXMAX+1][LMAXMAX+1];
  double PlmPrime[LMAXMAX+1][LMAXMAX+1];

  double *CosMP     = new double[lMax+1];
  double *SinMP     = new double[lMax+1];
  double CT, ST;
  cdouble PhiFac;

  /*--------------------------------------------------------------*/
  /* get sines and cosines                                        */ 
  /*--------------------------------------------------------------*/
  for(m=0; m<=lMax; m++)
   { SinMP[m]=sin( ((double)m) * Phi );
     CosMP[m]=cos( ((double)m) * Phi );
   };

  /*--------------------------------------------------------------*/
  /* get associated legendre functions and derivatives.           */
  /* after this code snippet, i have                              */
  /*   Plm[m][l] = P_l^m (\cos\theta)                             */ 
  /*    and                                                       */ 
  /*   PlmPrime[m][l] = (dP_l^m(x) / dx) |_{x=\cos\theta}         */
  /* where P_l^m is the associated legendre function with the     */
  /* correct prefactor for use in spherical harmonics.            */
  /*--------------------------------------------------------------*/
  if ( Theta < 1.0e-6 )
   Theta=1.0e-6;
  if ( fabs(M_PI-Theta) < 1.0e-6 ) 
   Theta=M_PI-1.0e-6;
  ST=sin(Theta);
  CT=cos(Theta);
  for(m=0; m<=lMax; m++)
   GetPlm(lMax, m, CT, Plm[m] + m, PlmPrime[m] + m);

  /*--------------------------------------------------------------*/
  /* assemble output quantities                                   */
  /*--------------------------------------------------------------*/
  for(Alpha=l=0; l<=lMax; l++)
   for(m=-l; m<=l; m++, Alpha++)
    { 
      mm=abs(m);

      if (m>=0)
       PhiFac=cdouble( CosMP[mm],  SinMP[mm] );
      else
       PhiFac=( (mm%2)==1 ? -1.0 : 1.0) * cdouble( CosMP[mm], - SinMP[mm] );

      Ylm[Alpha]         =          Plm[mm][l]      * PhiFac;
      if (dYlmdTheta)
       dYlmdTheta[Alpha] =  -1.0*ST*PlmPrime[mm][l] * PhiFac;
    };

  //delete[] Plm;
  //delete[] PlmPrime;
  delete[] CosMP;
  delete[] SinMP;
}

/**********************************************************************/
/* evaluate the radial functions that enter into the M_{lm} and N_{lm}*/
/* functions defined below.                                           */
/*                                                                    */
/* the functions are evaluated for l=0,1,\cdots, lMax. thus, a total  */
/* of lMax+1 functions are computed.                                  */
/*                                                                    */
/* on entry, R and dRdr must point to arrays with enough space for    */
/* lMax+2 cdoubles. (dRdr may be NULL, in which case that part of the */
/* computation is skipped).                                           */
/*                                                                    */
/* note: there is no typo here! R and dRdr must have space for lMax+2 */
/* slots, not lMax+1 slots, even though the user is only asking for   */
/* lMax+1 return quantities. the reason is that we need the extra slot*/
/* because we need to compute one extra R function to get derivatives */
/* using recurrence relations.                                        */
/*                                                                    */
/* Workspace points to an optional user-allocated workspace buffer.   */
/* Workspace may be NULL, in which case work space will be allocated  */
/* dynamically; if it is non-NULL it must point to a buffer of length */
/* at least 4*(lMax+2).                                               */
/**********************************************************************/
void GetRadialFunctions(int lMax, cdouble k, double r, int WaveType, 
                        cdouble *R, cdouble *dRdr, double *Workspace)
{
  int l;
  cdouble kr;
  double Sign;

  /*--------------------------------------------------------------*/
  /*- 20111112 separate handling for the r==0 case ---------------*/
  /*--------------------------------------------------------------*/
  if (r==0.0)
   { memset(R, 0, lMax*sizeof(cdouble));
     if (dRdr) memset(dRdr, 0, lMax*sizeof(cdouble));
     if (WaveType == LS_REGULAR ) 
      { R[0]=1.0/3.0; dRdr[1]=k/3.0; };
     return;
   };
  
  if ( real(k)==0.0 )
   { 
     kr=imag(k)*r;
     if ( WaveType == LS_REGULAR )
      AmosBessel('i',kr,0.0,lMax+2,0,R,Workspace);
     else // no distinction between incoming/outgoing in this case, right?
      AmosBessel('k',kr,0.0,lMax+2,0,R,Workspace);
   }
  else
   { kr=k*r;
     if ( WaveType == LS_REGULAR )
      AmosBessel('j',kr,0.0,lMax+2,0,R,Workspace);
     else if ( WaveType == LS_OUTGOING ) 
      AmosBessel('o',kr,0.0,lMax+2,0,R,Workspace);
     else  // ( WaveType == LS_INCOMING ) 
      AmosBessel('t',kr,0.0,lMax+2,0,R,Workspace);
   };

  if (dRdr==0) 
   return;

  /* get derivatives using d/dx f_L(x) = (L/x)f_L(x) - f_{L+1}(x)) */
  /* 20100325 hmmm... it seems that, for j_L, y_L, and k_L, i do   */
  /* indeed have                                                   */
  /*  d/dx f_L(x) = (L/x) f_L(x) - f_{L+1}(x)                      */
  /* but for i_l i instead have                                    */
  /*  d/dx f_L(x) = (L/x) f_L(x) + f_{L+1}(x)                      */
  /* ?? is this correct? somebody other than me please check and   */
  /* verify.                                                       */
  /* 20100519 yes, this is correct by eq 10.2.21 of abramowitz+stegun */ 
  /*  (page 444)                                                   */
  if ( real(k)==0.0 && WaveType==LS_REGULAR )
   Sign=1.0;
  else
   Sign=-1.0;

  for(l=0; l<=lMax; l++)
   { 
     if (r==0.0) 
      dRdr[l]=0.0;
     else
      dRdr[l] = k*( ((double) l)*R[l]/kr + Sign*R[l+1] );
   };

}

/***************************************************************/
/***************************************************************/
/***************************************************************/
void GetRadialFunction(int l, cdouble k, double r, int WaveType, 
                       cdouble *Rl, cdouble *dRldr, cdouble *RlSlash)
{ 
  cdouble *R    = new cdouble[l+2];
  cdouble *dRdr = new cdouble[l+2];
  
  GetRadialFunctions(l, k, r, WaveType, R, dRdr); 

  *Rl = R[l];

  if (dRldr) 
   *dRldr = dRdr[l];

  if (RlSlash) 
   *RlSlash = R[l]/(k*r) + dRdr[l]/k;

  delete[] R;
  delete[] dRdr;

}

/***************************************************************/
/* scalar helmholtz solutions.                                 */
/*                                                             */ 
/* Phi_{lm} = f_l(kr) Y_{lm}(\theta,\phi)                      */ 
/*                                                             */ 
/* where the radial function f_l(kr) is                        */
/*  a. j_l(kr)            for interior, real-frequency         */
/*  b. j_l(kr) + iy_l(kr) for exterior, real-frequency         */
/*  c. i_l(kappa*r)       for interior, imag-frequency         */
/*  d. k_l(kappa*r)       for exterior, imag-frequency         */
/*      (where kappa=imag(k))                                  */
/*                                                             */
/* set WaveType=LS_REGULAR, LS_OUTGOING, or LS_INCOMING.       */
/*                                                             */ 
/* on entry, Psi must point to a cdouble array with enough     */ 
/* space to store NAlpha cdoubles, where NAlpha = (lMax+1)^2.  */
/*                                                             */ 
/* on return, Psi[Alpha] is the value of the Alphath function  */
/* evaluated at the specified coordinates, where               */
/* Alpha= l*(l+1) + m.                                         */
/***************************************************************/
void GetScalarHelmholtzSolutions(int lMax, cdouble k, 
                                 double r, double Theta, double Phi,
                                 int WaveType, cdouble *Psi)
{  
  int NAlpha=(lMax+1)*(lMax+1);
  int Alpha, l, m;

  cdouble *R   = new cdouble[lMax+2];
  cdouble *Ylm = new cdouble[NAlpha];

  /***************************************************************/
  /* get radial functions ****************************************/
  /***************************************************************/
  GetRadialFunctions(lMax, k, r, WaveType, R, 0);
  
  /***************************************************************/
  /* get angular functions ***************************************/
  /***************************************************************/
  if ( fabs(Theta) < 1.0e-6 )           /* this is a useful hack */
   Theta=1.0e-6;
  if ( fabs(M_PI-Theta) < 1.0e-6) 
   Theta=M_PI-1.0e-6;
  GetYlmArray(lMax, Theta, Phi, Ylm);

  /***************************************************************/
  /* assemble the psi functions                                  */
  /***************************************************************/
  for (Alpha=l=0; l<=lMax; l++)
   for (m=-l; m<=l; m++, Alpha++)
    Psi[Alpha]=R[l]*Ylm[Alpha];

  delete[] R;
  delete[] Ylm;

}

/***************************************************************/  
/* vector spherical harmonics (note that the return values are */
/* the _spherical_ components of the vector, not the cartesian */ 
/* components.)                                                */ 
/* this is the function that jackson calls 'X_{lm}.'           */
/***************************************************************/
void GetXlm(int l, int m, double Theta, double Phi, cdouble X[3])
{  
  if (l==0)
   { memset(X,0,3*sizeof(cdouble));
     return;
   };
  
  if ( fabs(Theta)<1.0e-6 )
   Theta=1.0e-6;
  if ( fabs(Theta-M_PI) < 1.0e-6 )
   Theta=M_PI-1.0e-6;
  double SinTheta=sin(Theta);

  int NAlpha=(l+1)*(l+1);
  cdouble *Ylm        = new cdouble[NAlpha]; 
  cdouble *dYlmdTheta = new cdouble[NAlpha];
  GetYlmDerivArray(l, Theta, Phi, Ylm, dYlmdTheta);

  int Alpha = l*(l+1) + m;
  double NormFac=1.0/sqrt( (double)(l*(l+1)) ) ;

  X[0]=0.0;
  X[1]= -m*NormFac*Ylm[Alpha]/SinTheta;
  X[2]= -II*NormFac*dYlmdTheta[Alpha];

  delete[] Ylm;
  delete[] dYlmdTheta;

}

/***************************************************************/
/* get an array of vector spherical harmonics.                 */
/*                                                             */
/* return values:                                              */
/*  X[0..2]: r, theta, phi component of X_0 (even though it=0) */
/*  X[3..5]: r, theta, phi component of X_1                    */
/*  ...                                                        */
/*  X[3*(NAlpha-1)...3*(NAlpha-1)+2] : r, theta, phi compoents */
/*                                     of X_{NAlpha-1}         */
/*                                                             */
/* so the X buffer must have room to store 3*NAlpha doubles.   */ 
/***************************************************************/
void GetXlmArray(int lMax, double Theta, double Phi, cdouble *X)
{  
  int l, m, Alpha, NAlpha=(lMax+1)*(lMax+1);
  cdouble *Ylm = new cdouble[NAlpha];
  cdouble *dYlmdTheta = new cdouble[NAlpha];
  double NormFac, SinTheta;
  
  if ( fabs(Theta)<1.0e-6 )
   Theta=1.0e-6;
  if ( fabs(Theta-M_PI) < 1.0e-6 )
   Theta=M_PI-1.0e-6;
  SinTheta=sin(Theta);

  GetYlmDerivArray(lMax, Theta, Phi, Ylm, dYlmdTheta);

  X[0]=X[1]=X[2]=0.0;

  for(Alpha=l=1; l<=lMax; l++)
   { 
     NormFac=1.0/sqrt( (double)(l*(l+1)) ) ;

     for(m=-l; m<=l; m++, Alpha++)
      { X[3*Alpha+0]=0.0;
        X[3*Alpha+1]= -m*NormFac*Ylm[Alpha]/SinTheta;
        X[3*Alpha+2]= -II*NormFac*dYlmdTheta[Alpha];
      };

   };

  delete[] Ylm;
  delete[] dYlmdTheta;

}

/***************************************************************/
/* vector helmholtz functions. these are the functions known   */
/* as M_{lm} and N_{lm} in some books. M_{lm} is the quantity  */ 
/* that multiplies a_E(l,m) in the first line of jackson's     */
/* equation 9.122, while N_{lm} is the quantity that multiplies*/
/* a_E(l,m) in the second line of that equation.               */ 
/*                                                             */ 
/* these functions are defined by                              */ 
/*  M_{lm} = f_l(kr) X_{lm}(\theta,\phi)                       */ 
/*  N_{lm} = (1/-ik) \nabla \times M_{lm}                      */
/*                                                             */ 
/* where X_{lm} = L \times Y_{lm}(\theta,\phi)                 */ 
/*  (as computed by the previous function)                     */ 
/*                                                             */ 
/* (note that my definition of N is i*the convention used in   */ 
/* some other books, i.e. tai)                                 */ 
/*                                                             */ 
/* and where the radial function f_l(kr) is                    */
/*  a. j_l(kr)            for interior, real-frequency         */
/*  b. j_l(kr) + iy_l(kr) for exterior, real-frequency         */
/*  c. i_l(kappa*r)       for interior, imag-frequency         */
/*  d. k_l(kappa*r)       for exterior, imag-frequency         */
/*      (where kappa=imag(k))                                  */ 
/*                                                             */ 
/* set WaveType=LS_REGULAR, LS_OUTGOING, or LS_INCOMING.       */
/*                                                             */ 
/* on entry, M and N must point to cdouble arrays with enough  */ 
/* space to store 3*NAlpha cdoubles,                           */ 
/* where NAlpha = (lMax+1)^2.                                  */
/*                                                             */ 
/* on return,                                                  */
/*  M[3*Alpha,3*Alpha+1,3*Alpha+2]                             */
/* and                                                         */ 
/*  N[3*Alpha,3*Alpha+1,3*Alpha+2]                             */
/* are the                                                     */ 
/* (spherical) coordinates of M_{lm} and N_{lm}, where         */
/* Alpha= l*(l+1) + m.                                         */
/*                                                             */
/* Workspace points to an optional user-allocated workspace    */
/* buffer. Workspace may be NULL, in which case work space will*/
/* be allocated dynamically; if it is non-NULL it must point   */
/* to a buffer of length at least 4*(lMax+2).                  */
/***************************************************************/
void GetMNlmArray(int lMax, cdouble k,
                  double r, double Theta, double Phi,
                  int WaveType, cdouble *M, cdouble *N,
                  double *Workspace)
{  
  int NAlpha=(lMax+1)*(lMax+1);
  int Alpha, l, m;
  double SinTheta, dl, dm;
  cdouble nik, PreFac;
  cdouble ROverR;
  cdouble *R          = new cdouble[lMax+2]; 
  cdouble *dRdr       = new cdouble[lMax+2];
  cdouble *Ylm        = new cdouble[NAlpha];
  cdouble *dYlmdTheta = new cdouble[NAlpha];

  /***************************************************************/
  /* get radial functions ****************************************/
  /***************************************************************/
  GetRadialFunctions(lMax, k, r, WaveType, R, dRdr, Workspace);
  
  /***************************************************************/
  /* get angular functions ***************************************/
  /***************************************************************/
  if ( fabs(Theta) < 1.0e-6 )           /* this is a useful hack */
   Theta=1.0e-6;
  if ( fabs(M_PI-Theta) < 1.0e-6)
   Theta=M_PI-1.0e-6;
  SinTheta=sin(Theta);
  GetYlmDerivArray(lMax, Theta, Phi, Ylm, dYlmdTheta);

  /***************************************************************/
  /* assemble the components of the M and N functions ************/
  /***************************************************************/
  memset(M,0,3*sizeof(cdouble));  /* zero out the l==0 functions */
  memset(N,0,3*sizeof(cdouble));
  nik=-II*k;
  for (Alpha=l=1; l<=lMax; l++)
   for (m=-l; m<=l; m++, Alpha++)
    { 
       dl=((double)l);
       dm=((double)m);

       PreFac=1.0/sqrt( dl*(dl+1.0) );

       M[3*Alpha + 0]= 0.0;
       M[3*Alpha + 1]= -dm*PreFac*R[l]*Ylm[Alpha]/SinTheta;
       M[3*Alpha + 2]= -II*PreFac*R[l]*dYlmdTheta[Alpha];

       PreFac/=nik;

       if (r==0.0)
        ROverR = (l==1) ? k/3.0 : 0.0;
       else
        ROverR=R[l]/r;
       N[3*Alpha + 0]= -sqrt(dl*(dl+1.0))*ROverR*Ylm[Alpha]/k;
       N[3*Alpha + 1]= II*PreFac*(ROverR + dRdr[l])*dYlmdTheta[Alpha];
       N[3*Alpha + 2]= -dm*PreFac*(ROverR + dRdr[l])*Ylm[Alpha]/SinTheta;

    };

  delete[] R;
  delete[] dRdr;
  delete[] Ylm;
  delete[] dYlmdTheta;
  
}

/***************************************************************/
/***************************************************************/
/***************************************************************/
void GetMNlmHardCoded(int l, int m, cdouble k, double r, double Theta, double Phi, 
                      int WaveType, cdouble M[3], cdouble N[3])
{ 
  if (l==0)
   { M[0]=M[1]=M[2]=N[0]=N[1]=N[2]=0.0;
     return;
   };

  cdouble kr=k*r, kr2=kr*kr, kr3=kr2*kr, kr4=kr2*kr2;
  cdouble ikr=II*k*r, ikr2=ikr*ikr, ikr3=ikr2*ikr;
  cdouble ExpFac = (WaveType == LS_OUTGOING) ? exp( ikr) :
                   (WaveType == LS_INCOMING) ? exp(-ikr) : cos(kr);
  ExpFac *= exp(II*((double) m)*Phi);
  cdouble Q1=(1.0-ikr);
  cdouble Q2A=(1.0 - ikr + ikr2);
  cdouble Q2B=(3.0 - 3.0*ikr + ikr2);
  cdouble Q3=(6.0 - 6.0*ikr + 3.0*ikr2 - ikr3);
  M[0]=M[1]=M[2]=N[0]=N[1]=N[2]=0.0;
  double Sign = m > 0 ? 1.0 : -1.0;
  double CT = cos(Theta), ST = sin(Theta);
  double C2T = cos(2.0*Theta), S2T = sin(2.0*Theta);
  if (l==1 && abs(m)==1 )
   {  cdouble MPreFac = sqrt(3.0/(16.0*M_PI)) * ExpFac / kr2;
      cdouble NPreFac = MPreFac / kr;
      M[1] = MPreFac * -II * Q1;
      M[2] = MPreFac * Sign * Q1 * CT;
      N[0] = NPreFac * Sign * -2.0 * II * Q1 * ST;
      N[1] = NPreFac * Sign * II * Q2A * CT;
      N[2] = NPreFac * -1.0 * Q2A;
   }
  else if (l==1 && m==0 )
   {  cdouble MPreFac = sqrt(3.0/(8.0*M_PI)) * ExpFac / kr2;
      cdouble NPreFac = MPreFac / kr;
      M[2] = MPreFac * Q1 * ST;
      N[0] = NPreFac * 2.0 * II * Q1 * CT;
      N[1] = NPreFac * II * Q2A * ST;
   }
  else if (l==2 && abs(m)==2 )
   {  cdouble MPreFac = sqrt(5.0/(16.0*M_PI)) * ExpFac / kr3;
      cdouble NPreFac = MPreFac / kr;
      M[1] = MPreFac * Sign * II * Q2B * ST;
      M[2] = MPreFac * -1.0 * Q2B * CT * ST;
      N[0] = NPreFac * 3.0 * II * Q2B * ST * ST;
      N[1] = NPreFac * -1.0 * II * Q3 * CT * ST;
      N[2] = NPreFac * Sign * Q3 * ST;
   }
  else if (l==2 && abs(m)==1 )
   {  cdouble MPreFac = sqrt(5.0/(16.0*M_PI)) * ExpFac / kr3;
      cdouble NPreFac = MPreFac / kr;
      M[1] = MPreFac * -1.0 * II * Q2B * CT;
      M[2] = MPreFac * Sign * Q2B * C2T;
      N[0] = NPreFac * Sign * -3.0 * II * Q2B * S2T;
      N[1] = NPreFac * Sign * II * Q3 * C2T;
      N[2] = NPreFac * -1.0 * Q3 * CT;
   }
  else if (l==2 && m==0 )
   {  cdouble MPreFac = sqrt(15.0/(8.0*M_PI)) * ExpFac / kr3;
      cdouble NPreFac = MPreFac / kr;
      M[2] = MPreFac * Q2B * CT * ST;
      N[0] = NPreFac * II * Q2B * (3.0*CT*CT-1.0);
      N[1] = NPreFac * II * Q3 * CT * ST;
   };
}

/***************************************************************/
/***************************************************************/
/***************************************************************/
void GetMNlm(int l, int m, cdouble k, double r, double Theta, double Phi, 
             int WaveType, cdouble M[3], cdouble N[3])
{
  if ( l<=2 )
   { GetMNlmHardCoded(l, m, k, r, Theta, Phi, WaveType, M, N);
     return;
   };

  int NAlpha = (l+1)*(l+1);
  cdouble *MArray = new cdouble[3*NAlpha]; 
  cdouble *NArray = new cdouble[3*NAlpha];

  GetMNlmArray(l, k, r, Theta, Phi, WaveType, MArray, NArray);

  int Alpha = l*(l+1) + m;
  M[0] = MArray[ 3*Alpha + 0 ];
  M[1] = MArray[ 3*Alpha + 1 ];
  M[2] = MArray[ 3*Alpha + 2 ];
  N[0] = NArray[ 3*Alpha + 0 ];
  N[1] = NArray[ 3*Alpha + 1 ];
  N[2] = NArray[ 3*Alpha + 2 ];

  delete[] MArray;
  delete[] NArray;

}
 
/***************************************************************/
/* compute the curl of a vector-valued function ****************/
/***************************************************************/
void GetCurl(double r, double Theta, double Phi,
             void (*F)(double r, double Theta, double Phi, cdouble *V),
             cdouble *CurlF, cdouble *CurlF2)
{
 
  /***************************************************************/
  /* METHOD 1: use the expression for the curl in spherical      */
  /*           coordinates                                       */
  /***************************************************************/
#define METHOD1
#ifdef METHOD1
{
  cdouble F0[3], FP[3], FM[3], dFdr[3], dFdTheta[3], dFdPhi[3];
  double Delta;
  double CT, ST;
  int Mu;

  CT=cos(Theta);
  ST=sin(Theta);

  F(r, Theta, Phi, F0);
 
  Delta = (r==0.0) ? 1.0e-4 : 1.0e-4*fabs(r);
  F(r+Delta, Theta, Phi, FP);
  F(r-Delta, Theta, Phi, FM); 
  for(Mu=0; Mu<3; Mu++)
   dFdr[Mu] = (FP[Mu]-FM[Mu]) / (2.0*Delta);
 
  Delta = (Theta==0.0) ? 1.0e-4 : 1.0e-4*fabs(Theta);
  F(r, Theta+Delta, Phi, FP);
  F(r, Theta-Delta, Phi, FM);
  for(Mu=0; Mu<3; Mu++)
   dFdTheta[Mu] = (FP[Mu]-FM[Mu]) / (2.0*Delta);
 
  Delta = (Phi==0.0) ? 1.0e-4 : 1.0e-4*fabs(Phi);
  F(r, Theta, Phi+Delta, FP);
  F(r, Theta, Phi-Delta, FM);
  for(Mu=0; Mu<3; Mu++)
   dFdPhi[Mu] = (FP[Mu]-FM[Mu]) / (2.0*Delta);

  /*
  F(r+DeltaR, Theta, Phi, dFdTheta); 
  F(r, Theta+DeltaT, Phi, dFdTheta); 
  F(r, Theta, Phi+DeltaP, dFdPhi); 
  for(Mu=0; Mu<3; Mu++)
   { dFdr[Mu]     = (dFdr[Mu]     - F0[Mu]) / DeltaR;
     dFdTheta[Mu] = (dFdTheta[Mu] - F0[Mu]) / DeltaT;
     dFdPhi[Mu]   = (dFdPhi[Mu]   - F0[Mu]) / DeltaP;
   };
  */

  CurlF[0]=(ST*dFdTheta[2] + CT*F0[2] - dFdPhi[1]) / (r*ST);
  CurlF[1]=dFdPhi[0]/(r*ST) - F0[2]/r - dFdr[2];
  CurlF[2]=F0[1]/r + dFdr[1] - dFdTheta[0]/r;

 };
#endif 

  /***************************************************************/
  /* METHOD 2: convert back and forth to cartesian components    */
  /***************************************************************/
#define METHOD2
#ifdef METHOD2
{
  double X[3], rr, tt, pp;
  cdouble F0[3], F0C[3], dFdX[3], dFdY[3], dFdZ[3], CurlFC[3];
  double Delta;
  int mu;

  CoordinateS2C(r, Theta, Phi, X);

  F(r,Theta,Phi,F0);
  VectorS2C(Theta, Phi, F0, F0C);

  Delta = (X[0]==0.0) ? 1.0e-4 : 1.0e-4*fabs(X[0]);
  X[0]+=Delta;
  CoordinateC2S(X, &rr, &tt, &pp);
  F(rr, tt, pp, F0);
  VectorS2C(tt, pp, F0, dFdX);
  for(mu=0; mu<3; mu++)
   dFdX[mu] = (dFdX[mu] - F0C[mu]) / Delta;
  X[0]-=Delta;

  Delta = (X[1]==0.0) ? 1.0e-4 : 1.0e-4*fabs(X[1]);
  X[1]+=Delta;
  CoordinateC2S(X, &rr, &tt, &pp);
  F(rr, tt, pp, F0);
  VectorS2C(tt, pp, F0, dFdY);
  for(mu=0; mu<3; mu++)
   dFdY[mu] = (dFdY[mu] - F0C[mu]) / Delta;
  X[1]-=Delta;

  Delta = (X[2]==0.0) ? 1.0e-4 : 1.0e-4*fabs(X[2]);
  X[2]+=Delta;
  CoordinateC2S(X, &rr, &tt, &pp);
  F(rr, tt, pp, F0);
  VectorS2C(tt, pp, F0, dFdZ);
  for(mu=0; mu<3; mu++)
   dFdZ[mu] = (dFdZ[mu] - F0C[mu]) / Delta;
  X[2]-=Delta;

  CurlFC[0]=dFdY[2] - dFdZ[1];
  CurlFC[1]=dFdZ[0] - dFdX[2];
  CurlFC[2]=dFdX[1] - dFdY[0];
  VectorC2S(Theta, Phi, CurlFC, CurlF2);
}
#endif

}
 
/***************************************************************/
/* compute the divergence of a vector-valued function.         */
/* Terms[0..2] are the three separate terms in the expression  */
/* for the divergence in spherical coordinates. Use the second */
/* of the two calling conventions if you don't need these.     */
/***************************************************************/
void GetDiv(double r, double Theta, double Phi,
            void (*F)(double r, double Theta, double Phi, cdouble *V),
            cdouble Terms[3], cdouble *DivF)
{
  cdouble F0[3], dFdr[3], dFdTheta[3], dFdPhi[3];
  double CT, ST;
  int mu;

  CT=cos(Theta);
  ST=sin(Theta);
  F(r,Theta,Phi,F0);
  F(r+1.0e-6, Theta, Phi, dFdr); 
  F(r, Theta+1.0e-6, Phi, dFdTheta); 
  F(r, Theta, Phi+1.0e-6, dFdPhi); 
  for(mu=0; mu<3; mu++)
   { dFdr[mu]     = (dFdr[mu]     - F0[mu]) / 1.0e-6;
     dFdTheta[mu] = (dFdTheta[mu] - F0[mu]) / 1.0e-6;
     dFdPhi[mu]   = (dFdPhi[mu]   - F0[mu]) / 1.0e-6;
   };
  Terms[0]=dFdr[0] + 2.0*F0[0]/r;
  Terms[1]=(CT*F0[1] + ST*dFdTheta[1])/(r*ST);
  Terms[2]=dFdPhi[2]/(r*ST);
  DivF[0]=Terms[0]+Terms[1]+Terms[2];
}



cdouble GetDiv(double r, double Theta, double Phi,
               void (*F)(double r, double Theta, double Phi, cdouble *V))
{ 
  cdouble Terms[3], DivF;
  GetDiv(r, Theta, Phi, F, Terms, &DivF);
  return DivF;
}
