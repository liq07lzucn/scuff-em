/*
 * RWGComposite.cc -- class constructor and other misc functions
 *                 -- for the RWG composite class 
 *                 
 * Homer Reid      -- 6/2012
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <libhrutil.h>

#include "libscuff.h"
#include "RWGComposite.h"

#define MAXSTR 1000
#define MAXTOK 50

namespace scuff{

/*--------------------------------------------------------------*/
/*-  RWGComposite constructor that begins reading an open      -*/
/*-  file immediately after a line like COMPOSITE MyLabel.     -*/
/*-                                                            -*/
/*-  On return, if an object was successfully created, its     -*/
/*-   ErrMsg field is NULL, and the file read pointer points   -*/
/*-   the line immediately following the ENDCOMPOSITE line.    -*/
/*-                                                            -*/
/*-  If there was an error in parsing the COMPOSITE section,   -*/
/*-   ErrMsg field points to an error message string.          -*/
/*-                                                            -*/
/*- syntax:                                                    -*/
/*- COMPOSITE MyCompositeLabel                                 -*/
/*-                                                            -*/
/*-           MESHFILE MyMesh.msh                              -*/
/*-                                                            -*/
/*-           SUBREGION 1 SiO2                                 -*/
/*-           SUBREGION 2 Gold                                 -*/
/*-                                                            -*/
/*-           PARTIALSURFACE 1 SUBREGIONS 0 1                  -*/
/*-           PARTIALSURFACE 2 SUBREGIONS 0 2                  -*/
/*-           PARTIALSURFACE 3 SUBREGIONS 1 2                  -*/
/*-                                                            -*/
/*- ENDCOMPOSITE                                               -*/
/*--------------------------------------------------------------*/
RWGComposite::RWGComposite(FILE *f, const char *pLabel, int *LineNum)
{ 
  /***************************************************************/
  /* initialize class fields *************************************/
  /***************************************************************/
  Label = strdup(pLabel);
  ErrMsg=0;

  PartialSurfaces=0 ;
  PartialSurfaceLabels=0 ;
  PSSubRegions=0;
  NumPartialSurfaces=0;

  // 
  // explain me  
  // 
  NumSubRegions = 0; 
  SubRegionMPs = (MatProp **) mallocEC( 1*sizeof(MatProp *) );

  Log("Processing COMPOSITE %s",pLabel);

  /***************************************************************/
  /* read lines from the file one at a time **********************/
  /***************************************************************/
  char Line[MAXSTR], LineCopy[MAXSTR];
  int NumTokens, TokensConsumed;
  char *Tokens[MAXTOK];
  int ReachedTheEnd=0;
  char *pMeshFileName=0;
  GTransformation *OTGT=0; // 'one-time geometrical transformation'
  int SubRegionID, SubRegionID1, SubRegionID2;
  MatProp *MP;
  while ( ReachedTheEnd==0 && fgets(Line, MAXSTR, f) )
   { 
     (*LineNum)++;
     strcpy(LineCopy,Line);
     NumTokens=Tokenize(LineCopy, Tokens, MAXTOK);
     if ( NumTokens==0 || Tokens[0][0]=='#' )
      continue; 

     /*--------------------------------------------------------------*/
     /*- switch off based on first token on the line ----------------*/
     /*--------------------------------------------------------------*/
     if ( !strcasecmp(Tokens[0],"MESHFILE") )
      { if (NumTokens!=2)
         { ErrMsg=strdup("MESHFILE keyword requires one argument");
           return;
         };
        pMeshFileName=strdup(Tokens[1]);
      }
     else if ( !strcasecmp(Tokens[0],"SUBREGION") )
      { 
        if (NumTokens!=3)
         { ErrMsg=strdup("SUBREGION keyword requires two arguments ");
           return;
         };

        if ( 1!=sscanf(Tokens[1],"%i", &SubRegionID) || SubRegionID != (NumSubRegions+1) )
         { ErrMsg=vstrdup("SUBREGIONs must be numbered sequentially starting from 1");
           return; 
         };

        MP=new MatProp(Tokens[2]);
        if (MP->ErrMsg)
         { ErrMsg=vstrdup("material %s: %s",Tokens[2],MP->ErrMsg);
           return; 
         };

        // add a new region to the list of regions for this composite
        NumSubRegions++;
        SubRegionMPs=(MatProp **)realloc(SubRegionMPs, (NumSubRegions+1)*sizeof(MatProp *));
        SubRegionMPs[NumSubRegions]=MP;
        Log(" Adding new subregion (%i) for material %s",NumSubRegions,MP->Name);

      }
     else if ( !strcasecmp(Tokens[0],"PARTIALSURFACE") )
      { 
        if (NumTokens!=5 || strcasecmp(Tokens[2],"SUBREGIONS") )
         ErrMsg=strdup("invalid syntax for PARTIALSURFACE keyword");
        if ( !ErrMsg && 1!=sscanf(Tokens[3],"%i",&SubRegionID1) )
         ErrMsg=vstrdup("syntax error");
        if ( !ErrMsg && 1!=sscanf(Tokens[4],"%i",&SubRegionID2) )
         ErrMsg=vstrdup("syntax error");

        if ( SubRegionID1<0 || SubRegionID1>NumSubRegions ) 
         ErrMsg=vstrdup("invalid subregion ID (%i)",SubRegionID1);
        if ( SubRegionID2<0 || SubRegionID2>NumSubRegions ) 
         ErrMsg=vstrdup("invalid subregion ID (%i)",SubRegionID1);

        if (ErrMsg)
         return;

        // add a new partial surface to this composite.
        //  (actually for now we just make a note of its existence; the
        //   actual PartialSurface data structure is not created until later)
        NumPartialSurfaces++;
        PartialSurfaceLabels=(char **)realloc(PartialSurfaceLabels, NumPartialSurfaces * sizeof(char *) );
        PartialSurfaceLabels[NumPartialSurfaces-1]=vstrdup(Tokens[1]);
        PSSubRegions = (int *)realloc(PSSubRegions, 2*NumPartialSurfaces*sizeof(int) );
        PSSubRegions[ 2*(NumPartialSurfaces-1) + 0 ] = SubRegionID1;
        PSSubRegions[ 2*(NumPartialSurfaces-1) + 1 ] = SubRegionID2;
        Log(" Adding new partial surface (%s) bounding %s and %s", 
              PartialSurfaceLabels[NumPartialSurfaces-1],
              SubRegionID1 ? SubRegionMPs[SubRegionID1]->Name : "exterior",
              SubRegionID2 ? SubRegionMPs[SubRegionID2]->Name : "exterior");

      }
     else if ( !strcasecmp(Tokens[0],"DISPLACED") || !strcasecmp(Tokens[0],"ROTATED") )
      { 
        // try to parse the line as a geometrical transformation.
        // note that OTGT is used as a running GTransformation that may
        // be augmented by multiple DISPLACED ... and/or ROTATED ...
        // lines within the OBJECT...ENDOBJECT section, and which is 
        // applied to the object at its birth and subsequently discarded.
        // in particular, OTGT is NOT stored as the 'GT' field inside 
        // the Object class, which is intended to be used for 
        // transformations that are applied and later un-applied 
        // during the life of the object. 
	OTGT = new GTransformation(Tokens, NumTokens, &ErrMsg, &TokensConsumed);
        if (ErrMsg)
         return;
        if (TokensConsumed!=NumTokens) 
         { ErrMsg=strdup("junk at end of line");
           return;
         };
      }
     else if ( !strcasecmp(Tokens[0],"ENDCOMPOSITE") )
      { 
        ReachedTheEnd=1;
      }
     else
      { ErrMsg=vstrdup("unknown keyword %s in COMPOSITE section",Tokens[0]);
        return;
      };
   }; 

  if (pMeshFileName==0)
   ErrMsg=vstrdup("COMPOSITE section must include a MESHFILE specification");
  if (NumSubRegions==0)
   ErrMsg=vstrdup("COMPOSITE section must include one or more SUBREGION designations");
  if (NumPartialSurfaces==0)
   ErrMsg=vstrdup("COMPOSITE section must include one or more PARTIALSURFACE designations");

  InitRWGComposite(pMeshFileName, OTGT);
  
  free(pMeshFileName);

}

/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
/*--------------------------------------------------------------*/
void RWGComposite::InitRWGComposite(const char *pMeshFileName, const GTransformation *OTGT)
{
  ErrMsg=0;
  //kdPanels = NULL;

  /*------------------------------------------------------------*/
  /*- try to open the mesh file.                                */
  /*------------------------------------------------------------*/
  FILE *MeshFile=fopen(pMeshFileName,"r");
  if (!MeshFile)
   ErrExit("could not open file %s",pMeshFileName);
  Log(" Processing RWG composite from meshfile %s",pMeshFileName);
   
  /*------------------------------------------------------------*/
  /*- initialize simple fields ---------------------------------*/
  /*------------------------------------------------------------*/
  NumPanels=NumVertices=0;
  MeshFileName=strdup(pMeshFileName);

  NumPanelsPerPartialSurface=(int *)mallocEC(NumPartialSurfaces*sizeof(int));
  EpsTF=(cdouble *)mallocEC( (NumSubRegions+1) * sizeof(cdouble) );
  MuTF=(cdouble *)mallocEC( (NumSubRegions+1) * sizeof(cdouble) );

  /*------------------------------------------------------------*/
  /*- note: the 'OTGT' parameter to this function is distinct   */
  /*- from the 'GT' field inside the class body. the former is  */
  /*- an optional 'One-Time Geometrical Transformation' to be   */
  /*- applied to the object once at its creation. the latter    */
  /*- is designed to store a subsequent transformation that may */
  /*- be applied to the object, and is initialized to zero.     */
  /*------------------------------------------------------------*/
  GT=0;

  /*------------------------------------------------------------*/
  /*- Switch off based on the file type to read the mesh file:  */
  /*-  1. file extension=.msh    --> ReadGMSHFile              -*/
  /*-  2. file extension=.mphtxt --> ReadComsolFile            -*/
  /*------------------------------------------------------------*/
  char *p=GetFileExtension(MeshFileName);
  if (!p)
   ErrExit("file %s: invalid extension",MeshFileName);
  else if (!strcasecmp(p,"msh"))
   ReadGMSHFile(MeshFile,MeshFileName,OTGT);
  //else if (!strcasecmp(p,"mphtxt"))
  //  ReadComsolFile(MeshFile,MeshFileName,OTGT);
  else
   ErrExit("file %s: unknown extension %s",MeshFileName,p);

  /*------------------------------------------------------------*/
  /*- create PartialSurface structures ----------------------------*/
  /*------------------------------------------------------------*/
  PartialSurfaces=(PartialSurface **)mallocEC(NumPartialSurfaces*sizeof(PartialSurface *));
  BFIndexOffset=(int *)mallocEC(NumPartialSurfaces*sizeof(int));
  BFIndexOffset[0]=0;
  PartialSurface *PS;
  int np, npTPS;
  TotalBFs=0;
  for(int nps=0; nps<NumPartialSurfaces; nps++)
   { 
     // create a new PartialSurface structure for this partial surface 
     PS = PartialSurfaces[nps]=(PartialSurface *)mallocEC(sizeof(PartialSurface));

     // initialize the list of panels that reside on this partial
     // surface. Note that the entries of the Panels[] array 
     // within the PartialSurface structure are just pointers to 
     // the same RWGPanel structures that were created by the 
     // meshfile input routine. 
     // note: npTPS = 'num panels, this partial surface'
     PS->NumPanels = NumPanelsPerPartialSurface[nps];
     PS->Panels=(RWGPanel **)mallocEC(PS->NumPanels*sizeof(RWGPanel *));
     for(npTPS=np=0; np<NumPanels; np++)
      if ( Panels[np]->SurfaceIndex==nps )
       { PartialSurfaces[nps]->Panels[npTPS]= Panels[np];
         PartialSurfaces[nps]->Panels[npTPS]->Index=npTPS;
         npTPS++;
       };

     // analyze edge connectivity for this partial surface 
     InitEdgeList(PS);

PS->NumHEdges=0;
PS->NumTotalEdges=PS->NumEdges;

     TotalBFs += 2*(PS->NumEdges + PS->NumHEdges);

     if ( nps+1 < NumPartialSurfaces )
      BFIndexOffset[nps+1] = BFIndexOffset[nps] + 2*(PS->NumEdges+PS->NumHEdges);

     Log("  Partial surface %i (subregions %i,%i): (%i,%i,%i) full/half/total edges",nps,PSSubRegions[2*nps+0],PSSubRegions[2*nps+1],PS->NumEdges,PS->NumHEdges,PS->NumTotalEdges);

   };

}

} // namespace scuff
