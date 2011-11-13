/*
 * libscuffInternals.h -- some definitions and prototypes that are used
 *                     -- internally within libscuff code files, but which
 *                     -- probably don't need to be exported as part of the API
 *
 * homer reid          -- 10/2005 -- 10/2011
 */

#ifndef LIBSCUFFINTERNALS_H
#define LIBSCUFFINTERNALS_H

#include "libscuff.h"

/***************************************************************/
/* 1. argument structures for routines whose input/output      */
/*    interface is so complicated that an ordinary C++         */
/*    function prototype would be too unwieldy.                */
/*                                                             */
/*    note: in each case, we provide an Init() function that   */
/*    fills in default values for the lesser-used fields       */
/*    in the argument structure; however, this leaves many     */
/*    fields uninitialized, and it is up to the caller to      */
/*    fill those in.                                           */
/***************************************************************/

/*--------------------------------------------------------------*/
/*- GetPanelPanelInteractions() --------------------------------*/
/*--------------------------------------------------------------*/
typedef struct GetPPIArgStruct
 { 
   // input fields to be filled in by caller
   RWGObject *Oa, *Ob;
   int npa, npb;
   int iQa, iQb;
   cdouble k;

   int NumGradientComponents;
   int NumTorqueAxes; 
   double *GammaMatrix;

   // output fields filled in by routine
   // note: H[0] = HPlus ( = HDot + (1/(ik)^2) * HNabla )
   // note: H[1] = HTimes
   // note: GradH[3*Mu + 0 ] = dHPlus/dR_\Mu
   // note: GradH[3*Mu + 1 ] = dHTimes/dR_\Mu
   // note: dHdT[3*Mu + 0 ] = dHPlus/dTheta_\Mu
   // note: dHdT[3*Mu + 1 ] = dHTimes/dTheta_\Mu
   cdouble H[2];
   cdouble GradH[6];
   cdouble dHdT[6];

 } GetPPIArgStruct;

void InitGetPPIArgs(GetPPIArgStruct *Args);
void GetPanelPanelInteractions(GetPPIArgStruct *Args);
void GetPanelPanelInteractions(GetPPIArgStruct *Args,
                               cdouble *H, 
                               cdouble *GradH, 
                               cdouble *dHdT);

/*--------------------------------------------------------------*/
/*- GetEdgeEdgeInteractions() ----------------------------------*/
/*--------------------------------------------------------------*/
typedef struct GetEEIArgStruct
 { 
   // input fields to be filled in by caller
   RWGObject *Oa, *Ob;
   int nea, neb;
   cdouble k;

   int NumGradientComponents;
   int NumTorqueAxes; 
   double *GammaMatrix;

   // output fields filled in by routine
   // note: GC[0] = <f_a|G|f_b>
   // note: GC[1] = <f_a|C|f_b>
   // note: GradGC[3*Mu + 0 ] d/dR_\Mu (<f_a|G|f_b>)
   // note: GradGC[3*Mu + 1 ] d/dR_\Mu (<f_a|C|f_b>)
   // note: dGCdT[3*Mu + 0 ] d/dTheta_\Mu (<f_a|G|f_b>)
   // note: dGCdT[3*Mu + 1 ] d/dTheta_\Mu (<f_a|C|f_b>)
   cdouble GC[2];
   cdouble GradGC[6];
   cdouble dGCdT[6];

 } GetEEIArgStruct;

void InitGetEEIArgs(GetEEIArgStruct *Args);
void GetEdgeEdgeInteractions(GetEEIArgStruct *Args);

/*--------------------------------------------------------------*/
/*- AssembleBEMMatrixBlock() -----------------------------------*/
/*--------------------------------------------------------------*/
typedef struct ABMBArgStruct
 {
   // input fields to be filled in by caller
   RWGGeometry *G;
   RWGObject *Oa, *Ob;
   cdouble Frequency;
   int nThread;

   int NumTorqueAxes;
   double *GammaMatrix;
  
   int RowOffset, ColOffset;

   int Symmetric;

   // output fields filled in by routine
   HMatrix *B;
   HMatrix **GradB;
   HMatrix **dBdTheta;

   // additional fields used internally that may be ignored by 
   // the called both before and after the call
   double Sign;
   cdouble EpsA, EpsB; 
   double MuA, MuA;
   int OaIsPEC, ObIsPEC;

 } ABMBArgStruct;

void InitABMBArgs(ABMBArgStruct *Args);
void AssembleBEMMatrixBlock(ABMBArgStruct *Args);

/***************************************************************/
/* 2. definition of data structures and methods for working    */
/*    with frequency-independent panel-panel integrals (FIPPIs)*/
/*                                                             */
/* note:                                                       */
/*  'FIPPI'   = 'frequency-independent panel-panel integral'   */
/*  'FIPPIDR' = 'FIPPI data record'                            */
/*  'FIPPIDT' = 'FIPPI data table'                             */
/***************************************************************/

// a 'FIPPIDataRecord' is a structure containing all the FIPPIs  
// for a single panel-panel pair 
typedef struct FIPPIDataRecord
 { 
   int HaveDerivatives;
   double hDotRM1, hDotR0, hDotR1, hDotR2;
   double hNablaRM1, hNablaR0, hNablaR1, hNablaR2;
   double hTimesRM3, hTimesRM1, hTimesR0, hTimesR1;
   
   // the following fields are needed only if we are computing
   // derivatives of panel-panel integrals
   double hDotRM3, hNablaRm3, hTimesRm5;
   double dhTimesdRMuRm3[3], dhTimesdRMuRm1[3], 
          dhTimesdRMuR0[3], dhTimesdRMuR1[3];
 } FIPPIDataRecord;

/*--------------------------------------------------------------*/
/*- this is the routine that computes the FIPPIs for a given    */
/*- pair of panels                                              */
/*--------------------------------------------------------------*/
FIPPIDataRecord *ComputeFIPPIDataRecord(double **Va, double *Qa,
                                        double **Vb, double *Qb,
                                        int NeedDerivatives,
                                        FIPPIDataRecord *FDR);

// 'FIPPIDataTable' is a class that implements a hash table
// storing FIPPIDataRecords for many pairs of panels
class FIPPIDataTable
 { 
    // constructor 
    FIPPIDataTable();

    // destructor 
    ~FIPPIDataTable();

    // retrieve FIPPIs for a given pair of panels
    FIPPIDataRecord *GetFIPPIDataRecord(double **Va, double *Qa,
                                        double **Vb, double *Qb,
                                        int NeedDerivatives);
   
 };

/***************************************************************/   
/* 3. some additional non-class-method routines used internally*/
/***************************************************************/
cdouble TaylorMaster(int WhichCase, int WhichG, int WhichH, cdouble GParam,
                     double *V1, double *V2, double *V3,
                     double *V2P, double *V3P, double *Q, double *QP);

/*--------------------------------------------------------------*/
/*- AssessPanelPair counts common vertices in a pair of panels  */
/*--------------------------------------------------------------*/
int AssessPanelPair(double **Va, double **Vb);

int AssessPanelPair(RWGObject *Oa, int npa, 
                    RWGObject *Ob, int npb,
                    double *rRel, 
                    double **Va, double **Vb);

int AssessPanelPair(RWGObject *Oa, int npa, 
                    RWGObject *Ob, int npb,
                    double *rRel);

int NumCommonVertices(RWGObject *Oa, int npa, 
                      RWGObject *Ob, int npb);



#endif //LIBSCUFFINTERNALS_H