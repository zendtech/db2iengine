/*** START HEADER FILE SPECIFICATIONS ********************************/         
/*                                                                   */         
/* Header File Name: H/QLGUSR                                        */         
/*                                                                   */         
/* Descriptive Name: This header contains all of the prototypes,     */         
/*                   constants, etc. needed to interface to any API  */         
/*                   functions contained in the QLGUSR *SRVPGM.      */         
/*                   This service program contains those functions   */         
/*                   which need to run as *INHERIT state and *USER   */         
/*                   domain.                                         */         
/*                                                                   */         
/* 5769-SS1  (C) Copyright IBM Corp. 1997,1998                       */         
/* All rights reserved.                                              */         
/* US Government Users Restricted Rights -                           */         
/* Use, duplication or disclosure restricted                         */         
/* by GSA ADP Schedule Contract with IBM Corp.                       */         
/*                                                                   */         
/* Licensed Materials-Property of IBM                                */         
/*                                                                   */         
/*                                                                   */         
/* Description:      This file supports the following functions:     */         
/*                                                                   */         
/*               --> QlgCvtTextDescToDesc                            */         
/*                                                                   */         
/*                      ( Convert Text Descriptor To Descriptor )    */         
/*                      function accepts a description of a CCSID of */         
/*                      the given type or operating system platform  */         
/*                      and returns a description of its equivalent  */         
/*                      in the requested type or operating system    */         
/*                      platform.                                    */         
/*                                                                   */         
/*               --> QlgTransformUCSData                             */         
/*                                                                   */         
/*                      ( Transform UCS Data                    )    */         
/*                      function transforms, via a formula as        */         
/*                      compared to a mapping, data to or from one   */         
/*                      form or encoding scheme from or to UCS.  A   */         
/*                      transformation type identification is used   */         
/*                      to specify the type of tranformation.        */         
/*                                                                   */         
/*                                                                   */         
/* Header Files Included:       None.                                */         
/*                                                                   */         
/* Macros List:                 None.                                */         
/*                                                                   */         
/* Function Prototype List:     QlgCvtTextDescToDesc                 */         
/*                              QlgTransformUCSData                  */         
/*                                                                   */         
/*                                                                   */         
/* Change Activity:                                                  */         
/*                                                                   */         
/* CFD List:                                                         */         
/*                                                                   */         
/* FLAG  REASON  RLSE   DATE   PGMR      CHANGE DESCRIPTION          */         
/* ---- -------- ---- -------- --------- --------------------------- */         
/* $A0= D95753   4D30 12/17/97 V2CIB100: New Include                 */         
/* $A1= P3665962 4D30 02/19/98 V2CIB100: Add QlgTransformUCSData     */         
/* $A2= D98583   5D20 07/05/01 michmyer: Add defines for UTF endian  */         
/*                                       conversions                 */         
/*                                                                   */         
/*  Additional notes about the Change Activity                       */         
/* End Change Activity.                                              */         
/*** END HEADER FILE SPECIFICATIONS **********************************/         
                                                                                
#ifndef _H_QLGUSR                                                               
#define _H_QLGUSR                                                               
#if (__OS400_TGTVRM__>=510)  /* @B1A*/                                          
#pragma datamodel(P128)      /* @B1A*/                                          
#endif                       /* @B1A*/                                          
                                                                                
/*--**--**--**--**--**--**--**--**---**--**--**--**--**--**--**--**--*/         
/* Start of QlgCvtTextDescToDesc API function information...         */         
/*--**--**--**--**--**--**--**--**---**--**--**--**--**--**--**--**--*/         
                                                                                
/*********************************************************************/         
/* Max values...                                                     */         
/*********************************************************************/         
                                                                                
#define Qlg_MaxDescSize       64                                                
                                                                                
/*********************************************************************/         
/* External constants for operating system/platform types to convert */         
/* from/to...                                                        */         
/*********************************************************************/         
                                                                                
#define Qlg_TypeCCSID          0                                                
#define Qlg_TypeAS400CCSID     1                                                
#define Qlg_TypeDesc           2                                                
#define Qlg_TypeAix31          3                                                
#define Qlg_TypeAix41          4                                                
#define Qlg_TypeWin31          5                                                
#define Qlg_TypeWin95          6                                                
#define Qlg_TypeWinNT          7                                                
#define Qlg_TypeOS2V3          8                                                
#define Qlg_TypeOS2V4          9                                                
#define Qlg_TypeMVS           10                                                
#define Qlg_TypeIANA          11                                                
                                                                                
/*********************************************************************/         
/* Constants for system errors...                                    */         
/*********************************************************************/         
                                                                                
#define Qlg_InternalError1         -1                                           
#define Qlg_InternalError2         -2                                           
                                                                                
/*********************************************************************/         
/* Constants for caller errors...                                    */         
/*********************************************************************/         
                                                                                
#define Qlg_BadInType              -3                                           
#define Qlg_BadOutType             -4                                           
#define Qlg_InTypeEqualsOutType    -5                                           
#define Qlg_CCSIDMapError          -6                                           
#define Qlg_BadCCSIDInDesc         -7                                           
#define Qlg_JobCCSIDNotFound       -8                                           
#define Qlg_BadJobCCSID            -9                                           
#define Qlg_InDescConvertFail     -10                                           
#define Qlg_InDescriptorNotFound  -11                                           
#define Qlg_OutTypeDescNotFound   -12                                           
#define Qlg_OutDescSizeTooSmall   -13                                           
#define Qlg_OutDescSizeExceedsMax -14                                           
                                                                                
/*********************************************************************/         
/*                                                                   */         
/*                      Prototype for Calling                        */         
/*             Convert Text Descriptor To Descriptor API.            */         
/*                                                                   */         
/*                      QlgCvtTextDescToDesc                         */         
/*                                                                   */         
/*  Inputs:                                                          */         
/*          int      -> Type of descriptor provided to match on.     */         
/*          int      -> Type of descriptor requested in result.      */         
/*          char   * -> Pointer to descriptor provided to match on.  */         
/*          int      -> Length of provided descriptor.               */         
/*          char   * -> Pointer to buffer to place descriptor result.*/         
/*          int      -> Size of buffer for result.                   */         
/*          int      -> CCSID of provided descriptor.                */         
/*                                                                   */         
/*  Returns:                                                         */         
/*     Success:      -> Length of resulting descriptor.              */         
/*                                                                   */         
/*     Error:        -> One of the system or caller errors as listed */         
/*                      above.                                       */         
/*                                                                   */         
/*********************************************************************/         
                                                                                
#ifndef __ILEC400__                                                             
   extern "C"                                                                   
#endif                                                                          
                                                                                
int QlgCvtTextDescToDesc( int    ,                                              
                          int    ,                                              
                          char * ,                                              
                          int    ,                                              
                          char * ,                                              
                          int    ,                                              
                          int    );                                             
                                                                                
/*--**--**--**--**--**--**--**--**---**--**--**--**--**--**--**--**--*/         
/* Start of QlgTransformUCSData API function information...          */         
/*--**--**--**--**--**--**--**--**---**--**--**--**--**--**--**--**--*/         
                                                                                
/*********************************************************************/         
/* Transformation type identifiers...                                */         
/*********************************************************************/         
                                                                                
#define Qlg_XFormUCS2toUTF8    1                                                
#define Qlg_XFormUTF8toUCS2    2                                                
                                                                                
/*********************************************************************/         
/* from, to and autodetect type identifiers...                   @A2A*/         
/*********************************************************************/         
                                                                                
#define Qlg_Autodetect	1                                                        
#define Qlg_UTF32BE	2                                                           
#define Qlg_UTF32LE	3                                                           
#define Qlg_UTF16BE	4                                                           
#define Qlg_UTF16LE	5                                                           
#define Qlg_UTF8	6                                                              
#define Qlg_BOM		1                                                              
#define Qlg_noBOM	2                                                             
                                                                                
/*********************************************************************/         
/* Macro to give the user the transformation type for non UCS2       */         
/* transformations. DO NOT USE this macro if doing a UCS2            */         
/* transformation.                                              @A2A */         
/*********************************************************************/         
#define Qlg_GETxformtype(fromtype, totype, BOMreq) \                            
        ((fromtype * 10000) + (totype * 10) + BOMreq)                           
/*********************************************************************/         
/* sample usage of macro                                             */         
/* Qlg_GETxformtype(Qlg_UTF16BE, Qlg_UTF8, Qlg_BOM) results in a     */         
/* transformation of 030061 which is a UTF-16 Big Endian to UTF-8    */         
/* with a BOM							@A2A */                                                     
/*********************************************************************/         
                                                                                
                                                                                
/*********************************************************************/         
/*                                                                   */         
/*                      Prototype for Calling                        */         
/*                     Transform UCS Data API.                       */         
/*                                                                   */         
/*                       QlgTransformUCSData                         */         
/*                                                                   */         
/*  Inputs:                                                          */         
/*          int      -> Type of transformation requested.            */         
/*          char  ** -> Address of pointer to buffer to transform.   */         
/*          size_t * -> Address of number of bytes to transform.     */         
/*          char  ** -> Address of pointer to resulting transform.   */         
/*          size_t * -> Address of size of buffer to hold result.    */         
/*          size_t * -> Address of space needed to complete request. */         
/*                                                                   */         
/*  Returns:                                                         */         
/*     Success:      -> 0                                            */         
/*                                                                   */         
/*     Error:        -> EFAULT   - Bad or invalid data pointer.      */         
/*                      EINVAL   - Invalid input data length.        */         
/*                      ENOMEM   - Not enough memory for malloc.     */         
/*                      EILSEQ   - Data out of sequence.             */         
/*                      E2BIG    - Insufficient space in out buffer. */         
/*                      EBADFUNC - Invalid transform type request.   */         
/*                      ENOTSUP  - Transformation not supported.     */         
/*                                                                   */         
/*********************************************************************/         
                                                                                
#ifndef __ILEC400__                                                             
   extern "C"                                                                   
#endif                                                                          
                                                                                
int QlgTransformUCSData( int      ,                                             
                         char  ** ,                                             
                         size_t * ,                                             
                         char  ** ,                                             
                         size_t * ,                                             
                         size_t * );                                            
                                                                                
/*--**--**--**--**--**--**--**--**---**--**--**--**--**--**--**--**--*/         
/* Start of ... API function information...                          */         
/*--**--**--**--**--**--**--**--**---**--**--**--**--**--**--**--**--*/         
                                                                                
#if (__OS400_TGTVRM__>=510)  /* @B1A*/                                          
#pragma datamodel(pop)       /* @B1A*/                                          
#endif                       /* @B1A*/                                          
#endif /* #ifndef _H_QLGUSR */                                                  
