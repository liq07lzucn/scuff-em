/*
 * GetFIPPIs_BruteForce.cc -- compute frequency-independent panel-panel 
 *                         -- integrals by brute force
 * 
 * homer reid -- 11/2005   -- 12/2011
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <libhrutil.h>
#include <libSGJC.h>
#include <libPolyFit.h>

#include "libscuff.h"
#include "libscuffInternals.h"

#define ABSTOL 1.0e-12    // absolute tolerance
#define RELTOL 1.0e-8    // relative tolerance

/***************************************************************/
/* data structure used to pass data to integrand routines      */
/***************************************************************/
typedef struct FIPPIBFData
 { 
   double *V0, A[3], B[3];
   double V0P[3], AP[3], BP[3];
 } FIPPIBFData;

/***************************************************************/
/***************************************************************/
/***************************************************************/
static void FIPPIBFIntegrand(unsigned ndim, const double *x, void *parms,
                             unsigned nfun, double *fval)
{
  FIPPIBFData *FIPPIBFD=(FIPPIBFData *)parms;
  
  /***************************************************************/
  /***************************************************************/
  /***************************************************************/
  double u=x[0];
  double v=u*x[1];
  double up=x[2];
  double vp=up*x[3];

  /***************************************************************/
  /***************************************************************/
  /***************************************************************/
  double X[3], XmQ[3], XP[3], XPmQP[3], R[3], XxXP[3];
  double r, r2, oor, oor3;

  memcpy(X,FIPPIBFD->V0,3*sizeof(double));
  VecPlusEquals(X,u,FIPPIBFD->A);
  VecPlusEquals(X,v,FIPPIBFD->B);

  memcpy(XP,FIPPIBFD->V0P,3*sizeof(double));
  VecPlusEquals(XP,up,FIPPIBFD->AP);
  VecPlusEquals(XP,vp,FIPPIBFD->BP);

  VecSub(X, XP, R);
  VecCross(X, XP, XxXP);
  r=VecNorm(R);
  oor=u*up/r;
  oor3=u*up/(r*r*r);
  r2=u*up*r*r;
  r*=u*up;

  /***************************************************************/
  /***************************************************************/
  /***************************************************************/
  fval[0] = R[0]*oor3;
  fval[1] = R[1]*oor3;
  fval[2] = R[2]*oor3;
  fval[3] = XxXP[0]*oor3;
  fval[4] = XxXP[1]*oor3;
  fval[5] = XxXP[2]*oor3;

  fval[6]  = oor;
  fval[7]  = up*oor;
  fval[8]  = vp*oor;
  fval[9]  = u*oor;
  fval[10] = u*up*oor;
  fval[11] = u*vp*oor;
  fval[12] = v*oor;
  fval[13] = v*up*oor;
  fval[14] = v*vp*oor;

  fval[15] = r;
  fval[16] = up*r;
  fval[17] = vp*r;
  fval[18] = u*r;
  fval[19] = u*up*r;
  fval[20] = u*vp*r;
  fval[21] = v*r;
  fval[22] = v*up*r;
  fval[23] = v*vp*r;

  fval[24] = r2;
  fval[25] = up*r2;
  fval[26] = vp*r2;
  fval[27] = u*r2;
  fval[28] = u*up*r2;
  fval[29] = u*vp*r2;
  fval[30] = v*r2;
  fval[31] = v*up*r2;
  fval[32] = v*vp*r2;

} 

/***************************************************************/
/* compute FIPPIs using brute-force technique                  */
/* (adaptive cubature over both panels).                       */
/***************************************************************/
void ComputeQIFIPPIData_BruteForce(double **Va, double **Vb, QIFIPPIData *QIFD)
{ 
  double rRel;

  /***************************************************************/
  /* setup for call to cubature routine    ***********************/
  /***************************************************************/
  FIPPIBFData MyFIPPIBFData, *FIPPIBFD=&MyFIPPIBFData;
 
  FIPPIBFD->V0 = Va[0];
  VecSub(Va[1], Va[0], FIPPIBFD->A);
  VecSub(Va[2], Va[1], FIPPIBFD->B);

  // note that for Vb[0] and Qb we make copies of the 
  // entries (not just the pointers) because we may need
  // to displace them, see below.
  memcpy(FIPPIBFD->V0P,Vb[0],3*sizeof(double));
  VecSub(Vb[1], Vb[0], FIPPIBFD->AP);
  VecSub(Vb[2], Vb[1], FIPPIBFD->BP);
   
  double Lower[4]={0.0, 0.0, 0.0, 0.0};
  double Upper[4]={1.0, 1.0, 1.0, 1.0};

  int nf, fDim=33;
  double Result[fDim], Error[fDim];

  /***************************************************************/
  /* switch off based on whether or not there are any common     */
  /* vertices                                                    */
  /***************************************************************/
  if (1) //ncv==0)
   {
     /*--------------------------------------------------------------*/
     /* if there are no common vertices then we can just use naive   */
     /* cubature                                                     */
     /*--------------------------------------------------------------*/
     adapt_integrate_log(fDim, FIPPIBFIntegrand, (void *)FIPPIBFD, 4, Lower, Upper,
                         0, ABSTOL, RELTOL, Result, Error, "SGJC.log", 15);

     QIFD->xMxpRM3[0]   = Result[nf++];
     QIFD->xMxpRM3[1]   = Result[nf++];
     QIFD->xMxpRM3[2]   = Result[nf++];
     QIFD->xXxpRM3[0]   = Result[nf++];
     QIFD->xXxpRM3[1]   = Result[nf++];
     QIFD->xXxpRM3[2]   = Result[nf++];
   
     QIFD->uvupvpRM1[0] = Result[nf++];
     QIFD->uvupvpRM1[1] = Result[nf++];
     QIFD->uvupvpRM1[2] = Result[nf++];
     QIFD->uvupvpRM1[3] = Result[nf++];
     QIFD->uvupvpRM1[4] = Result[nf++];
     QIFD->uvupvpRM1[5] = Result[nf++];
     QIFD->uvupvpRM1[6] = Result[nf++];
     QIFD->uvupvpRM1[7] = Result[nf++];
     QIFD->uvupvpRM1[8] = Result[nf++];
   
     QIFD->uvupvpR1[0] = Result[nf++];
     QIFD->uvupvpR1[1] = Result[nf++];
     QIFD->uvupvpR1[2] = Result[nf++];
     QIFD->uvupvpR1[3] = Result[nf++];
     QIFD->uvupvpR1[4] = Result[nf++];
     QIFD->uvupvpR1[5] = Result[nf++];
     QIFD->uvupvpR1[6] = Result[nf++];
     QIFD->uvupvpR1[7] = Result[nf++];
     QIFD->uvupvpR1[8] = Result[nf++];
   
     QIFD->uvupvpR2[0] = Result[nf++];
     QIFD->uvupvpR2[1] = Result[nf++];
     QIFD->uvupvpR2[2] = Result[nf++];
     QIFD->uvupvpR2[3] = Result[nf++];
     QIFD->uvupvpR2[4] = Result[nf++];
     QIFD->uvupvpR2[5] = Result[nf++];
     QIFD->uvupvpR2[6] = Result[nf++];
     QIFD->uvupvpR2[7] = Result[nf++];
     QIFD->uvupvpR2[8] = Result[nf++];
   }
  else
   {
#if 0
     /*--------------------------------------------------------------*/
     /* if there are common vertices then we estimate the panel-panel*/
     /* integrals using a limiting process in which we displace the  */
     /* second of the two panels through a distance Z in the         */
     /* direction of the panel normal and try to fit to Z==0         */
     /*--------------------------------------------------------------*/
     int nz, NZ=10;
     double Z[NZ], GR[NZ], GI[NZ], CR[NZ], CI[NZ];
     double DeltaZ=Ob->Panels[npb]->Radius/10.0;
     double *ZHat=Ob->Panels[npb]->ZHat;

     for(nz=0; nz<NZ; nz++)
      { 
        Z[nz]=((double)(nz+1))*DeltaZ;
        VecScaleAdd(Vb[0], Z[nz], ZHat, PPIBFD->V0P);
        VecScaleAdd(Qb,    Z[nz], ZHat, PPIBFD->QP);

 //       adapt_integrate(fDim, PPIBFIntegrand, (void *)PPIBFD, 4, Lower, Upper,
 //                       0, ABSTOL, RELTOL, Result, Error);
       adapt_integrate_log(fDim, PPIBFIntegrand, (void *)PPIBFD, 4, Lower, Upper,
                           100000, ABSTOL, RELTOL, Result, Error, "SGJC.log", 15);

        GR[nz]=Result[0];
        GI[nz]=Result[1];
        CR[nz]=Result[2];
        CI[nz]=Result[3];
      };
 
     PolyFit *PF=new PolyFit(Z, GR, NZ, 4);
     PF->PlotFit(Z, GR, NZ, 0.0, Z[NZ-1], "real(<fa|G|fb>)");
     real(Args->H[0])=PF->f(0.0);
     delete PF;

     PF=new PolyFit(Z, GI, NZ, 4);
     PF->PlotFit(Z, GI, NZ, 0.0, Z[NZ-1], "imag(<fa|G|fb>)");
     imag(Args->H[0])=PF->f(0.0);
     delete PF;

     PF=new PolyFit(Z, CR, NZ, 4);
     PF->PlotFit(Z, CR, NZ, 0.0, Z[NZ-1], "imag(<fa|C|fb>)");
     real(Args->H[1])=PF->f(0.0);
     delete PF;

     PF=new PolyFit(Z, CI, NZ, 4);
     PF->PlotFit(Z, CI, NZ, 0.0, Z[NZ-1], "imag(<fa|C|fb>)");
     real(Args->H[1])=PF->f(0.0);
     delete PF;
#endif
     
   }; // if (ncv==0 ... else)

  /***************************************************************/
  /***************************************************************/
  /***************************************************************/

}
