/* begin_generated_IBM_copyright_prolog                              */         
/* This is an automatically generated copyright prolog.              */         
/* After initializing,  DO NOT MODIFY OR MOVE                        */         
/* ----------------------------------------------------------------- */         
/*                                                                   */         
/* Product(s):                                                       */         
/*     5722-SS1                                                      */         
/*     5761-SS1                                                       */        
/*     5770-SS1                                                      */         
/*                                                                   */         
/* (C)Copyright IBM Corp.  2007, 2010                                */         
/*                                                                   */         
/* All rights reserved.                                              */         
/* US Government Users Restricted Rights -                           */         
/* Use, duplication or disclosure restricted                         */         
/* by GSA ADP Schedule Contract with IBM Corp.                       */         
/*                                                                   */         
/* Licensed Materials-Property of IBM                                */         
/*                                                                   */         
/*  ---------------------------------------------------------------  */         
/*                                                                   */         
/* end_generated_IBM_copyright_prolog                                */         
                                                                                
/*** START HEADER FILE SPECIFICATIONS *******************************/          
/*                                                                   */         
/* Header File Name: H/QMYSE                                         */         
/*                                                                   */         
/* Description: Provides storage engine APIs.                        */         
/*                                                                   */         
/* Change Activity:                                                  */         
/*                                                                   */         
/* Flg Reason    Version    Date   Origin Comments                   */         
/* --- --------- ---------- ------ ------ -------------------------- */         
/* $-- P9C33438  v5r4m0f    091707 ROCH   Created                    */         
/* $01 P9C34366  v5r4m0f    102308 ROCH   Add object handle to       */         
/*                                        execute immediate          */         
/* $02 P9C34994  v5r4m0f    110408 ROCH   Add table not found        */         
/* $03 P9C38251  v5r4m0f    020509 ROCH   Add tracing                */         
/* $04 FW491716  v5r4m0f    011610 ROCH   Full NLS catalog support   */         
/*                                                                   */         
/*** END HEADER FILE SPECIFICATIONS *********************************/          
                                                                                
#ifndef QMYSE_h                                                                 
#define QMYSE_h                                                                 
                                                                                
#ifdef __OS400__                    /* If ILE                        */         
 #ifndef __pointer_h                /* If pointers not declared      */         
  #include <pointer.h>              /* Define pointer types          */         
#endif                              /* End ILE                       */         
#else                               /* Else i5/OS PASE               */         
 #include <as400_types.h>           /* Include ILE types             */         
 typedef ILEpointer _SYSPTR;        /* Define pointer types          */         
#endif                              /* End i5/OS PASE                */         
                                                                                
#ifdef __cplusplus                                                              
  extern "C" {                                                                  
#endif                                                                          
                                                                                
#ifdef __OS400__                                                                
  #pragma pack(1)                                                               
#else                                                                           
  #pragma options align=packed                                                  
#endif /* __OS400__ */                                                          
                                                                                
/*********************************************************************/         
/* APIs                                                              */         
/*********************************************************************/         
#ifdef __OS400__                                                                
#ifdef __ILEC400__                                                              
#pragma linkage(QmyRegisterParameterSpaces,OS,nowiden)                          
#else                                                                           
extern "OS"                                                                     
#endif                                                                          
int32_t  QmyRegisterParameterSpaces(void     *,                                 
                                    /* I-Request parameter space     */         
                                    void     *);                                
                                    /* I-Reply parameter space       */         
                                                                                
#ifdef __ILEC400__                                                              
#pragma linkage(QmyRegisterSpace,OS,nowiden)                                    
#else                                                                           
extern "OS"                                                                     
#endif                                                                          
uint64_t QmyRegisterSpace          (void     *);                                
                                    /* I-Space                       */         
                                                                                
#ifdef __ILEC400__                                                              
#pragma linkage(QmyUnregisterSpace,OS,nowiden)                                  
#else                                                                           
extern "OS"                                                                     
#endif                                                                          
void     QmyUnregisterSpace        (uint64_t  );                                
                                    /* I-Space handle                */         
                                                                                
#ifdef __ILEC400__                                                              
#pragma linkage(QmyProcessRequest,OS,nowiden)                                   
#else                                                                           
extern "OS"                                                                     
#endif                                                                          
int32_t  QmyProcessRequest         (void      );                                
                                    /*                               */         
#endif /* __OS400__ */                                                          
                                                                                
                                                                                
/*********************************************************************/         
/* Parameters                                                        */         
/*********************************************************************/         
/*-------------------------------------------------------------------*/         
/* Common parameters                                                 */         
/*-------------------------------------------------------------------*/         
/* QmyProcessRequest request structures                              */         
#define QMY_ALLOCATE_INSTANCE           2                                       
#define QMY_ALLOCATE_SHARE              1                                       
#define QMY_CLEANUP                     8                                       
#define QMY_CLOSE_CONNECTION            3                                       
#define QMY_DEALLOCATE_OBJECT           6                                       
#define QMY_DELETE_ROW                  7                                       
#define QMY_DESCRIBE_CONSTRAINTS        5                                       
#define QMY_DESCRIBE_OBJECT             9                                       
#define QMY_DESCRIBE_RANGE             17                                       
#define QMY_EXECUTE_IMMEDIATE          21                                       
#define QMY_INITIALIZATION             10                                       
#define QMY_INTERRUPT                  27                                       
#define QMY_LOCK_OBJECT                11                                       
#define QMY_OBJECT_INITIALIZATION      12                                       
#define QMY_OBJECT_OVERRIDE            13                                       
#define QMY_PROCESS_COMMITMENT_CONTROL  4                                       
#define QMY_PROCESS_SAVEPOINT          20                                       
#define QMY_PROCESS_XA                 26                                       
#define QMY_PREPARE_OPEN_CURSOR        22                                       
#define QMY_QUIESCE_OBJECT             28                                       
#define QMY_READ_ROWS                  15                                       
#define QMY_REORGANIZE_TABLE           14                                       
#define QMY_RELEASE_ROW                19                                       
#define QMY_UPDATE_ROW                 24                                       
#define QMY_WRITE_ROWS                 25                                       
                                                                                
/* True/False                                                        */         
#define QMY_NO                         0x00                                     
#define QMY_YES                        0x01                                     
                                                                                
/* Commit values                                                     */         
#define QMY_NONE                       0xD5                                     
#define QMY_READ_UNCOMMITTED           0xC3                                     
#define QMY_READ_COMMITTED             0xE2                                     
#define QMY_REPEATABLE_READ            0xC1                                     
#define QMY_SERIALIZABLE               0xD3                                     
                                                                                
/* Return codes                                                      */         
#define QMY_SUCCESS                    0x0000                                   
#define QMY_FAILURE                    0x0001                                   
                                                                                
#define QMY_ERR_MIN                    2016                                     
#define QMY_ERR_SPACE_CRTMOBJ          2017                                     
#define QMY_ERR_FILE_CRTMOBJ           2018                                     
#define QMY_ERR_SPACE_CRTSYNCTOK       2019                                     
#define QMY_ERR_FILE_CRTSYNCTOK        2020                                     
#define	QMY_ERR_MSGID                  2021                                     
#define QMY_ERR_SQUNSYNC               2022                                     
#define QMY_ERR_OBJ_LOCK               2023                                     
#define QMY_ERR_SAVEPOINT              2024                                     
#define QMY_ERR_PARTIAL_ICU            2025                                     
#define QMY_ERR_RTVSORTKEY             2026                                     
#define QMY_ERR_CONVERT_NLSS           2027                                     
#define QMY_ERR_INVALID_NLSS           2028                                     
#define QMY_ERR_INVALID_PARENT         2029                                     
#define QMY_ERR_EXTRACT                2030                                     
#define QMY_ERR_EXTRACT_PARENT         2031                                     
#define QMY_ERR_CMTCTL                 2032                                     
#define QMY_ERR_OPNCMTLVL              2033                                     
#define QMY_ERR_BAD_FILHND             2034                                     
#define QMY_ERR_NEED_MORE_SPACE        2035                                     
#define QMY_ERR_READ_RTNDATA           2036                                     
#define QMY_ERR_READ_ORIENT            2037                                     
#define QMY_ERR_READ_OPTTYP            2038                                     
#define QMY_ERR_CMTLVL                 2039                                     
#define QMY_ERR_SQUNSYNC_ALC           2040	                                    
#define QMY_ERR_RTNFMT                 2041                                     
#define QMY_ERR_UNSUPPORTED_XA         2042                                     
#define QMY_ERR_SRVR_KILLED            2043                                     
#define QMY_ERR_SQUNSYNC_SEI           2044                                     
#define QMY_ERR_SQUNSYNC_SPO           2045                                     
#define	QMY_ERR_CVTCCSTL_IN            2046                                     
#define	QMY_ERR_OPEN_CURSOR            2047                                     
#define QMY_ERR_GETASSCC               2048                                     
#define QMY_ERR_XLT_STRING             2049                                     
#define QMY_ERR_UNLOCK_SYNCTOK         2050                                     
#define QMY_ERR_DESTROY_SYNCTOK        2051                                     
#define QMY_ERR_LOCK_SYNCTOK           2052                                     
#define	QMY_ERR_RECREATE_SYNCTOK       2053                                     
#define	QMY_ERR_CONSTRAINT_SPCHND      2054                                     
#define	QMY_ERR_DELETE_CURSOR          2055                                     
#define	QMY_ERR_INTERFACE              2056                                     
#define	QMY_ERR_DELETE_UFCB            2057                                     
#define	QMY_ERR_OBJINIT_CURSOR         2058                                     
#define	QMY_ERR_OVERRIDE_CURSOR        2059                                     
#define	QMY_ERR_OVERRIDE_DSHND         2060                                     
#define	QMY_ERR_INFO_CURSOR            2061                                     
#define	QMY_ERR_OBJLCK_CURSOR          2062                                     
#define	QMY_ERR_REORGANIZE_CURSOR      2063                                     
#define	QMY_ERR_READ_DATAHND           2064                                     
#define	QMY_ERR_READ_RRNHND            2065                                     
#define	QMY_ERR_READ_KEYHND            2066                                     
#define	QMY_ERR_RANGE_CURSOR           2067                                     
#define	QMY_ERR_RANGE_SPCHND           2068                                     
#define	QMY_ERR_RLSRCD_CURSOR          2069                                     
#define	QMY_ERR_EXECSQL_STTHND         2070                                     
#define	QMY_ERR_PREPOPEN_STTHND        2071                                     
#define	QMY_ERR_UPDATE_CURSOR          2072                                     
#define	QMY_ERR_NO_UFCB                2073                                     
#define	QMY_ERR_UPDATE_UFCB            2074                                     
#define	QMY_ERR_UPDATE_DATAHND         2075                                     
#define	QMY_ERR_WRITE_CURSOR           2076                                     
#define	QMY_ERR_WRITE_DATAHND          2077                                     
#define	QMY_ERR_UNKNOWN_REQUEST        2078                                     
#define	QMY_ERR_ALCSHR_SHRDEF          2079                                     
#define	QMY_ERR_ALCSHR_SHRHND          2080                                     
#define QMY_ERR_ALCSHR_USEHND          2081                                     
#define QMY_ERR_INFO_AVGHND            2082                                     
#define QMY_ERR_RSLLOB                 2083                                     
#define QMY_ERR_LOB_SPACE_TOO_SMALL    2084                                     
#define	QMY_ERR_UNKNOWN_SRVREQ         2085                                     
#define	QMY_ERR_NOT_AUTH               2086                                     
#define QMY_ERR_LOCKMTX                2088                                     
#define QMY_ERR_UNLOCKMTX              2089                                     
#define QMY_ERR_RWCHKRDB               2090                                     
#define QMY_ERR_CREATEMTX              2091                                     
#define QMY_ERR_TABLE_EXISTS           2092/*                    @02A*/         
#define QMY_ERR_UNLKMTX                2094                                     
#define QMY_ERR_CONNECT_SRVJOB         2095                                     
#define QMY_ERR_ADD_SERVER_ENTRY       2096                                     
#define QMY_ERR_FUNCK_RPS              2098                                     
#define QMY_ERR_END_OF_BLOCK           2101                                     
#define QMY_ERR_LVLID_MISMATCH         2102                                     
#define QMY_ERR_GIVE_DESCRIPTOR        2103                                     
#define QMY_ERR_PEND_LOCKS             2104                                     
#define QMY_ERR_NO_LOCK                2105                                     
#define QMY_ERR_SETCND                 2106                                     
#define QMY_ERR_WAITHND                2107                                     
#define QMY_ERR_MAXVALUE               2108                                     
#define QMY_ERR_CLOSE_PIPE             2109                                     
#define QMY_ERR_TAKE_DESC              2110                                     
#define QMY_ERR_WRITE_PIPE             2111                                     
#define QMY_ERR_INTERRUPTED            2112                                     
#define QMY_ERR_NO_PIPE                2113                                     
#define QMY_ERR_SQ_PREPARE             2114                                     
#define QMY_ERR_SQ_OPEN                2115                                     
#define QMY_ERR_KEY_NOT_FOUND          2116                                     
#define QMY_ERR_DUP_KEY                2117                                     
#define QMY_ERR_END_OF_FILE            2118                                     
#define QMY_ERR_LOCK_TIMEOUT           2119                                     
#define QMY_ERR_CST_VIOLATION          2120                                     
#define QMY_ERR_TABLE_NOT_FOUND        2121                                     
#define QMY_ERR_MISC                   2122                                     
#define QMY_ERR_NON_UNIQUE_KEY         2123                                     
#define QMY_ERR_MAX                    2123                                     
                                                                                
/* Max spaces                                                        */         
#define QMY_MAX_SPACES                 10                                       
                                                                                
/* Data type values                                                  */         
#define QMY_BINARY                     0x0000                                   
#define QMY_FLOAT                      0x0001                                   
#define QMY_ZONED                      0x0002                                   
#define QMY_PACKED                     0x0003                                   
#define QMY_CHAR                       0x0004                                   
#define QMY_BLOBCLOB                   0x4004                                   
#define QMY_VARCHAR                    0x8004                                   
#define QMY_GRAPHIC                    0x0005                                   
#define QMY_DBCLOB                     0x4005                                   
#define QMY_VARGRAPHIC                 0x8005                                   
#define QMY_DATE                       0x000B                                   
#define QMY_TIME                       0x000C                                   
#define QMY_TIMESTAMP                  0x000D                                   
                                                                                
/*-------------------------------------------------------------------*/         
/* QMY_ALLOCATE_SHARE parameter structures                           */         
/*-------------------------------------------------------------------*/         
/* Share definition                                                  */         
typedef struct ShrDef {                                                         
   char            Reserved1[10];   /* Reserved for future use       */         
   char            ArrSeq[1];       /* Arrival sequence              */         
                                    /*  Valid values:                */         
                                    /*   QMY_NO                      */         
                                    /*   QMY_YES                     */         
   char            Reserved2[5];    /* Reserved for future use       */         
   uint32_t        ObjNamLen;       /* Object name length            */         
   char            ObjNam[258];     /* Object name                   */         
   char            Reserved3[10];   /* Reserved for future use       */         
 } ShrDef_t;                        /*                               */         
                                                                                
/* Table definition                                                  */         
typedef struct format_hdr {                                                     
   uint64_t        StartIdVal;      /* Starting value for identity   */         
                                    /* column                        */         
   uint32_t        ColDefOff;       /* Offset to the column          */         
                                    /* definitions                   */         
                                    /* from the start of this header */         
   int16_t         ColCnt;          /* Number of column definitions  */         
   char            FilLvlId[13];    /* File level ID                 */         
   char            Reserved1[5];    /* Reserved for future use       */         
 } format_hdr_t;                    /*   Format header               */         
                                                                                
/* Column definition.  There is one per column in the table.         */         
typedef struct col_def {                                                        
   char            ColType[2];      /* Data type                     */         
   uint16_t        ColLen;          /* Number of bytes this column   */         
                                    /* occupies in the row           */         
   int16_t         ColBufOff;       /* Column offset                 */         
   char            ColCCSID[2];     /* CCSID                         */         
   char            Reserved1[8];    /* Reserved for future use       */         
 } col_def_t;                       /*   Format header               */         
                                                                                
/*-------------------------------------------------------------------*/         
/* QMY_DESCRIBE_CONSTRAINTS parameter structures                     */         
/*-------------------------------------------------------------------*/         
/* Constraint name                                                   */         
typedef struct cst_name {                                                       
   uint16_t        Len;             /* Variable length of name       */         
   char            Name[128];       /* Delimited name                */         
   char            Reserved1[14];   /* Reserved for future use       */         
} cst_name_t;                                                                   
                                                                                
/* Constraint definition                                             */         
typedef struct constraint_hdr {                                                 
   uint32_t        CstDefOff;       /* Offset to the constraint      */         
                                    /* definition/description from   */         
                                    /* the start of this header.     */         
   uint32_t        CstLen;          /* Length of the constraint. For */         
                                    /* FK constraints, this includes */         
                                    /* the header, description, and  */         
                                    /* column lists. Add this length */         
                                    /* to the start of the header to */         
                                    /* address the next constraint.  */         
   char            CstType[1];      /* Type of constraint.           */         
                                    /*  Valid values:                */         
                                    /*   QMY_CST_FK                  */         
   char            Reserved1[7];    /* Reserved for future use       */         
 } constraint_hdr_t;                /*   Constraint header           */         
                                                                                
#define QMY_CST_FK                     0x00                                     
                                                                                
/* Foreign key definition.  There is one per foreign key constraint. */         
typedef struct FK_constraint {                                                  
   cst_name_t      CstName;         /* Constraint name               */         
   cst_name_t      RefSchema;       /* Referenced tables's schema    */         
   cst_name_t      RefTable;        /* Referenced table's name       */         
   uint32_t        KeyCnt;          /* Foreign key count             */         
   uint32_t        RefCnt;          /* Referenced key count          */         
   uint32_t        KeyColOff;       /* Offset to list of foreign     */         
                                    /*   key columns                 */         
   uint32_t        RefColOff;       /* Offset to list of referenced  */         
                                    /*   key columns                 */         
   uint16_t        UpdMethod;       /* Update method                 */         
                                    /*  Valid values:                */         
                                    /*   QMY_NOACTION                */         
                                    /*   QMY_RESTRICT                */         
   uint16_t        DltMethod;       /* Delete method                 */         
                                    /*  Valid values:                */         
                                    /*   QMY_CASCADE                 */         
                                    /*   QMY_SETDFT                  */         
                                    /*   QMY_SETNULL                 */         
                                    /*   QMY_NOACTION                */         
                                    /*   QMY_RESTRICT                */         
   char            Reserved1[6];    /* Reserved for future use       */         
} FK_constraint_t;                                                              
                                                                                
#define QMY_NOACTION                   0x0000                                   
#define QMY_CASCADE                    0x0001                                   
#define QMY_SETDFT                     0x0002                                   
#define QMY_SETNULL                    0x0003                                   
#define QMY_RESTRICT                   0x0004                                   
                                                                                
/*-------------------------------------------------------------------*/         
/* QMY_DESCRIBE_OBJECT parameter structures                          */         
/*-------------------------------------------------------------------*/         
/* ILE time definition                                               */         
typedef struct {                                                                
  uint16_t Year;                    /* Year                          */         
  uint16_t Month;                   /* Month                         */         
  uint16_t Day;                     /* Day                           */         
  uint16_t Hour;                    /* Hour                          */         
  uint16_t Minute;                  /* Minute                        */         
  uint16_t Second;                  /* Second                        */         
} ILE_time_t;                                                                   
                                                                                
typedef struct RowKey {                                                         
  uint64 RowKeyArray[120];          /* Average number of rows for    */         
                                    /* for this keypart              */         
} RowKey_t;                                                                     
                                                                                
/*-------------------------------------------------------------------*/         
/* QMY_DESCRIBE_RANGE parameter structures                           */         
/*-------------------------------------------------------------------*/         
/* Key bound                                                         */         
typedef struct Bound {                                                          
  char Reserved1[2];                /* Reserved for future use       */         
  uint16_t Position;                /* Bound value position          */         
  char IsNull[1];                   /* Bound value is null           */         
                                    /*  Valid values:                */         
                                    /*   QMY_NO                      */         
                                    /*   QMY_YES                     */         
  char Infinity[1];                 /* Bound infinity value          */         
                                    /*  Valid values:                */         
                                    /*   QMY_NO_INFINITY             */         
                                    /*   QMY_POS_INFINITY            */         
                                    /*   QMY_NEG_INFINITY            */         
  char Embodiment[1];               /* Bound inclusion/exclusion     */         
                                    /*  Valid values:                */         
                                    /*   QMY_INCLUSION               */         
                                    /*   QMY_EXCLUSION               */         
  char Reserved2[9];                /* Reserved for future use       */         
} Bound_t;                                                                      
                                                                                
/* Key bounds range definition.  There is one per key in a composite.*/         
typedef struct Bounds {                                                         
  Bound_t LoBound;                  /* Low key bound                 */         
  Bound_t HiBound;                  /* High key bound                */         
} Bounds_t;                                                                     
                                                                                
/* Bound infinity values                                             */         
#define QMY_NO_INFINITY                0x00                                     
#define QMY_POS_INFINITY               0x01                                     
#define QMY_NEG_INFINITY               0x02                                     
                                                                                
/* Bound inclusion values                                            */         
#define QMY_INCLUSION                  0x00                                     
#define QMY_EXCLUSION                  0x01                                     
                                                                                
/* Literal definition.  Describes a literal value in the literal     */         
/* storage area.  The same definition may be used to describe more   */         
/* than one literal.                                                 */         
 typedef struct LitDef {                                                        
   uint32_t Offset;                 /* Offset to literal value       */         
   uint16_t Length;                 /* Length of literal value       */         
                                    /* NOTE: The GRAPHIC data type   */         
                                    /*  has two bytes per char; for  */         
                                    /*  GRAPHIC literals, specify    */         
                                    /*  the number of characters.    */         
   int16_t FieldNbr;                /* Field number associated with  */         
                                    /* this literal                  */         
   uint16_t DataType;               /* Literal's data type           */         
   char Reserved2[6];               /* Reserved for future use       */         
} LitDef_t;                                                                     
                                                                                
                                                                                
/*-------------------------------------------------------------------*/         
/* QMY_OBJECT_OVERRIDE, QMY_READ_ROWS, QMY_WRITE_ROWS and            */         
/*  QMY_UPDATE_ROW parameter structures                              */         
/*-------------------------------------------------------------------*/         
/* Buffer definition                                                 */         
typedef struct {                                                                
  uint32_t MaxRowCnt;               /* Max row count in buffer       */         
  uint32_t UsedRowCnt;              /* Used row count in buffer      */         
  char     Reserved1[8];            /* Reserved for future use       */         
} BufferHdr_t;                                                                  
                                                                                
                                                                                
/*-------------------------------------------------------------------*/         
/* QMY_READ_ROWS parameter structures                                */         
/*-------------------------------------------------------------------*/         
/* Pipe reply definition                                             */         
typedef struct {                                                                
  int32_t  RtnCod;                  /* Return code                   */         
  uint32_t CumRowCnt;               /* Cumulative row count in buffer*/         
} PipeRpy_t;                                                                    
                                                                                
                                                                                
/*-------------------------------------------------------------------*/         
/* QMY_EXECUTE_IMMEDIATE and QMY_PREPARE_OPEN_CURSOR                 */         
/*  parameter structures                                             */         
/*-------------------------------------------------------------------*/         
/* Statement header definition                                       */         
typedef struct {                                                                
  char     SrtSeqNam[10];           /* Sort sequence name            */         
  char     SrtSeqSch[10];           /* Sort sequence schema          */         
  char     Reserved1[12];           /* Reserved for future use       */         
  uint32_t Length;                  /* Length of statement           */         
} StmtHdr_t;                                                                    
                                                                                
                                                                                
/*********************************************************************/         
/* Request parameter space structures                                */         
/*********************************************************************/         
/*-------------------------------------------------------------------*/         
/* Allocate object instance                                          */         
/*  -Reply format: Qmy_MAOI0100_output                               */         
/*  -Table share scope                                               */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MAOI0100 {                                                   
   uint16_t        Format;          /* QMY_ALLOCATE_INSTANCE         */         
   char            Reserved1[2];    /* Reserved for future use       */         
   int32_t         CnnHnd;          /* Connection handle             */         
   uint64_t        ShrHnd;          /* Share handle                  */         
   uint64_t        UseSpcHnd;       /* In-use indicator space handle */         
   char            Reserved2[8];    /* Reserved for future use       */         
} Qmy_MAOI0100_t;                                                               
                                                                                
/*-------------------------------------------------------------------*/         
/* Allocate object share                                             */         
/*  -Reply format: Qmy_MAOS0100_output                               */         
/*  -Storage engine scope                                            */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MAOS0100 {                                                   
   uint16_t        Format;          /* QMY_ALLOCATE_SHARE            */         
   char            Reserved1[2];    /* Reserved for future use       */         
   int32_t         CnnHnd;          /* Connection handle             */         
   char            Reserved2[8];    /* Reserved for future use       */         
   uint64_t        ShrDefSpcHnd;    /* Share definitions space handle*/         
   uint64_t        ShrHndSpcHnd;    /* Share handles space handle    */         
                                    /*  One share handle will be     */         
                                    /*  returned for each share      */         
                                    /*  definition                   */         
   uint64_t        FmtSpcHnd;       /* Format space handle           */         
   uint32_t        FmtSpcLen;       /* Length of format space        */         
   uint16_t        ShrDefCnt;       /* Count of shares definitions   */         
                                    /* 1st definition-Table          */         
                                    /* 2nd definition on-Indexes     */         
   char            NamCCSID[2];     /* Table/schema name CCSID   @04A*/         
   uint32_t        SchNamLen;       /* Schema name length            */         
   char            SchNam[258];     /* Schema name                   */         
   char            Reserved4[10];   /* Reserved for future use       */         
} Qmy_MAOS0100_t;                                                               
                                                                                
/*-------------------------------------------------------------------*/         
/* Cleanup storage engine                                            */         
/*  -Reply format: Qmy_MCLN0100_output                               */         
/*  -Storage engine scope                                            */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MCLN0100 {                                                   
   uint16_t        Format;          /* QMY_CLEANUP                   */         
   char            Reserved1[14];   /* Reserved for future use       */         
} Qmy_MCLN0100_t;                                                               
                                                                                
/*-------------------------------------------------------------------*/         
/* Close application connection                                      */         
/*  -Reply format: Qmy_MCCN0100_output                               */         
/*  -Application connection scope                                    */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MCCN0100 {                                                   
   uint16_t        Format;          /* QMY_CLOSE_CONNECTION          */         
   char            Reserved1[2];    /* Reserved for future use       */         
   int32_t         CnnHnd;          /* Connection handle             */         
   char            Reserved2[8];    /* Reserved for future use       */         
} Qmy_MCCN0100_t;                                                               
                                                                                
/*-------------------------------------------------------------------*/         
/* Deallocate object share or object instance                        */         
/*  -Reply format: Qmy_MDLC0100_output                               */         
/*  -Object share or object instance scope                           */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MDLC0100 {                                                   
   uint16_t        Format;          /* QMY_DEALLOCATE_OBJECT         */         
   char            Reserved1[6];    /* Reserved for future use       */         
   uint64_t        ObjHnd;          /* Object handle                 */         
   char            ObjDrp[1];       /* Object was dropped            */         
                                    /*  Valid values:                */         
                                    /*   QMY_NO                      */         
                                    /*   QMY_YES                     */         
   char            Reserved2[15];   /* Reserved for future use       */         
} Qmy_MDLC0100_t;                                                               
                                                                                
/*-------------------------------------------------------------------*/         
/* Delete row                                                        */         
/*  -Reply format: Qmy_MDLT0100_output                               */         
/*  -Object instance scope                                           */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MDLT0100 {                                                   
   uint16_t        Format;          /* QMY_DELETE_ROW                */         
   char            Reserved1[2];    /* Reserved for future use       */         
   int32_t         CnnHnd;          /* Connection handle             */         
   uint64_t        ObjHnd;          /* Object handle                 */         
   uint32_t        RelRowNbr;       /* Relative row number           */         
   char            Reserved2[12];   /* Reserved for future use       */         
} Qmy_MDLT0100_t;                                                               
                                                                                
/*-------------------------------------------------------------------*/         
/* Describe constraint information                                   */         
/*  -Reply format:  Qmy_MDCT0100_output                              */         
/*  -Table share scope                                               */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MDCT0100 {                                                   
   uint16_t        Format;          /* QMY_DESCRIBE_CONSTRAINTS      */         
   char            Reserved1[2];    /* Reserved for future use       */         
   int32_t         CnnHnd;          /* Connection handle             */         
   uint64_t        ShrHnd;          /* Share handle                  */         
   uint64_t        CstSpcHnd;       /* Constraint space handle       */         
   uint32_t        CstSpcLen;       /* Constraint space length       */         
   char            Reserved2[4];    /* Reserved for future use       */         
} Qmy_MDCT0100_t;                                                               
                                                                                
/*-------------------------------------------------------------------*/         
/* Describe object                                                   */         
/*  -Reply format: Qmy_MDSO0100_output                               */         
/*  -Table share scope                                               */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MDSO0100 {                                                   
   uint16_t        Format;          /* QMY_DESCRIBE_OBJECT           */         
   char            Reserved1[2];    /* Reserved for future use       */         
   int32_t         CnnHnd;          /* Connection handle             */         
   uint64_t        ShrHnd;          /* Share handle                  */         
   uint64_t        RowKeyHnd;       /* Rows per key handle           */         
   char            RtnObjLen[1];    /* Return object length          */         
                                    /*  Valid values:                */         
                                    /*   QMY_NO                      */         
                                    /*   QMY_YES                     */         
   char            RtnRowCnt[1];    /* Return row count              */         
                                    /*  Valid values:                */         
                                    /*   QMY_NO                      */         
                                    /*   QMY_YES                     */         
   char            RtnDltRowCnt[1]; /* Return deleted row count      */         
                                    /*  Valid values:                */         
                                    /*   QMY_NO                      */         
                                    /*   QMY_YES                     */         
   char            RtnRowKey[1];    /* Return rows per key           */         
                                    /*  Valid values:                */         
                                    /*   QMY_NO                      */         
                                    /*   QMY_YES                     */         
   char            RtnMeanRowLen[1];/* Return mean row length        */         
                                    /*  Valid values:                */         
                                    /*   QMY_NO                      */         
                                    /*   QMY_YES                     */         
   char            RtnModTim[1];    /* Return time of last           */         
                                    /*   modification                */         
                                    /*  Valid values:                */         
                                    /*   QMY_NO                      */         
                                    /*   QMY_YES                     */         
   char            RtnCrtTim[1];    /* Return create time            */         
                                    /*  Valid values:                */         
                                    /*   QMY_NO                      */         
                                    /*   QMY_YES                     */         
   char            RtnPageCnt[1];   /* Return page count             */         
                                    /*  Valid values:                */         
                                    /*   QMY_NO                      */         
                                    /*   QMY_YES                     */         
   char            RtnEstIoCnt[1];  /* Return estimated # of I/O's   */         
                                    /*  Valid values:                */         
                                    /*   QMY_NO                      */         
                                    /*   QMY_YES                     */         
   char            Reserved2[15];   /* Reserved for future use       */         
} Qmy_MDSO0100_t;                                                               
                                                                                
/*-------------------------------------------------------------------*/         
/* Describe range                                                    */         
/*  -Reply format: Qmy_MDRG0100_output                               */         
/*  -Table share scope                                               */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MDRG0100 {                                                   
   uint16_t        Format;          /* QMY_DESCRIBE_RANGE            */         
   char            Reserved1[2];    /* Reserved for future use       */         
   int32_t         CnnHnd;          /* Connection handle             */         
   uint64_t        ShrHnd;          /* Share handle                  */         
   uint64_t        SpcHnd;          /* Space handle                  */         
   uint32_t        KeyCnt;          /* Key count. Specify the        */         
                                    /*   number of keys in the       */         
                                    /*   composite.                  */         
   uint32_t        LiteralCnt;      /* Literal count. Specify the    */         
                                    /*   number of literal           */         
                                    /*   definitions.                */         
   uint32_t        BoundsOff;       /* Offset to the range bounds    */         
                                    /*   from the beginning of the   */         
                                    /*   range space.                */         
   uint32_t        LitDefOff;       /* Offset to the literal         */         
                                    /*   definitions from the        */         
                                    /*   beginning of the range      */         
                                    /*   space.                      */         
   uint32_t        LiteralsOff;     /* Offset to the literal values  */         
                                    /*   from the beginning of the   */         
                                    /*   range space.                */         
   uint32_t        Cutoff;          /* Early exit cutoff value.      */         
                                    /*  Valid values:                */         
                                    /*   0 - System decides cutoff   */         
                                    /*   n - Number between 0 and 100*/         
                                    /*       which indicates         */         
                                    /*       percentage of rows      */         
   uint16_t        EndByte;         /* I-Last byte of partial key    */         
   char            Reserved2[10];   /*   Reserved for future use     */         
   uint32_t        SpcLen;          /* Space length                  */         
} Qmy_MDRG0100_t;                                                               
                                                                                
/*-------------------------------------------------------------------*/         
/* Execute immediate                                                 */         
/*  -Reply format: Qmy_MSEI0100_output                               */         
/*  -Storage engine scope                                            */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MSEI0100 {                                                   
   uint16_t        Format;          /* QMY_EXECUTE_IMMEDIATE         */         
   char            Reserved1[2];    /* Reserved for future use       */         
   int32_t         CnnHnd;          /* Connection handle             */         
   uint64_t        ObjHnd;          /* Object handle             @01C*/         
   uint64_t        StmtsSpcHnd;     /* Statements space handle       */         
                                    /*  (Each statement should start */         
                                    /*   on quad word boundary)      */         
   uint32_t        NbrStmts;        /* Number of statements          */         
   char            StmtCCSID[2];    /* Statements CCSID              */         
   char            AutoCrtSchema[1];/* Auto create schema            */         
                                    /*  Valid values:                */         
                                    /*   QMY_NO                      */         
                                    /*   QMY_YES                     */         
   char            CmtLvl[1];       /* Commit level                  */         
                                    /*  Valid values:                */         
                                    /*   QMY_NONE                    */         
                                    /*   QMY_READ_UNCOMMITTED        */         
                                    /*   QMY_READ_COMMITTED          */         
                                    /*   QMY_REPEATABLE_READ         */         
                                    /*   QMY_SERIALIZABLE            */         
   char            CmtBefore[1];    /* Commit before statements      */         
                                    /*  Valid values:                */         
                                    /*   QMY_NO                      */         
                                    /*   QMY_YES                     */         
   char            CmtAfter[1];     /* Commit after statements if    */         
                                    /*  successful, rollback after   */         
                                    /*  statements if failure        */         
                                    /*  Valid values:                */         
                                    /*   QMY_NO                      */         
                                    /*   QMY_YES                     */         
   char            DropSchema[1];   /* Ignore 'schema not found' err */         
                                    /*  Valid values:                */         
                                    /*   QMY_NO                      */         
                                    /*   QMY_YES                     */         
   char            Reserved3[13];   /* Reserved for future use       */         
} Qmy_MSEI0100_t;                                                               
                                                                                
/*-------------------------------------------------------------------*/         
/* Initialize storage engine                                         */         
/*  -Reply format: Qmy_MINI0100_output                               */         
/*  -Storage engine scope                                            */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MINI0100 {                                                   
   uint16_t        Format;          /* QMY_INITIALIZATION            */         
   char            Reserved1[14];   /* Reserved for future use       */         
   uint16_t        RDBNamLen;       /* RDB name length               */         
   uint64_t        TrcSpcHnd;       /* Trace space handle        @03A*/         
                                    /*  Points to uint16_t           */         
                                    /*  Valid combinations:          */         
                                    /*   0x0020 - PRTSQLINF          */         
                                    /*   0x0010 - STRTRC             */         
                                    /*   0x0008 - DSPJOBLOG          */         
                                    /*   0x0004 - STRDBG             */         
                                    /*   0x0002 - STRDBMON           */         
   char            Reserved2[6];    /* Reserved for future use   @03C*/         
   char            RDBName[256];    /* RDB name                      */         
                                    /*   Blanks indicate default     */         
} Qmy_MINI0100_t;                                                               
                                                                                
/*-------------------------------------------------------------------*/         
/* Interrupt asyncronous request                                     */         
/*  -Reply format: Qmy_MINT0100_output                               */         
/*  -Table or index instance scope                                   */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MINT0100 {                                                   
   uint16_t        Format;          /* QMY_INTERRUPT                 */         
   char            Reserved1[2];    /* Reserved for future use       */         
   int32_t         CnnHnd;          /* Connection handle             */         
   uint64_t        ObjHnd;          /* Object handle                 */         
} Qmy_MINT0100_t;                                                               
                                                                                
/*-------------------------------------------------------------------*/         
/* Lock object                                                       */         
/*  -Reply format: Qmy_MOLX0100_output                               */         
/*  -Table share scope                                               */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MOLX0100 {                                                   
   uint16_t        Format;          /* QMY_LOCK_OBJECT               */         
   char            Reserved1[2];    /* Reserved for future use       */         
   int32_t         CnnHnd;          /* Connection handle             */         
   uint64_t        ShrHnd;          /* Share handle                  */         
   uint64_t        LckTimeoutVal;   /* Lock timeout                  */         
   char            Action[1];       /* Lock/unlock action            */         
                                    /*  Valid values:                */         
                                    /*   QMY_LOCK                    */         
                                    /*   QMY_UNLOCK                  */         
   char            LckTyp[1];       /* Lock type                     */         
                                    /*  Valid values:                */         
                                    /*   QMY_LSRD                    */         
                                    /*   QMY_LSRO                    */         
                                    /*   QMY_LSUP                    */         
                                    /*   QMY_LEAR                    */         
                                    /*   QMY_LENR                    */         
   char            LckTimeout[1];   /* Lock timeout specified        */         
                                    /*  Valid values:                */         
                                    /*   QMY_NO (Wait indefinitely)  */         
                                    /*   QMY_YES                     */         
   char            Reserved2[5];    /* Reserved for future use       */         
} Qmy_MOLX0100_t;                                                               
                                                                                
/* Lock action                                                       */         
#define QMY_LOCK                       0x01                                     
#define QMY_UNLOCK                     0x02                                     
                                                                                
/* Lock types                                                        */         
#define QMY_LSRD                       0x81                                     
#define QMY_LSRO                       0x41                                     
#define QMY_LSUP                       0x21                                     
#define QMY_LEAR                       0x11                                     
#define QMY_LENR                       0x09                                     
                                                                                
/*-------------------------------------------------------------------*/         
/* Initialize object instance                                        */         
/*  -Reply format: Qmy_MOIX0100_output                               */         
/*  -Table or index instance scope                                   */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MOIX0100 {                                                   
   uint16_t        Format;          /* QMY_OBJECT_INITIALIZATION     */         
   char            Reserved1[2];    /* Reserved for future use       */         
   int32_t         CnnHnd;          /* Connection handle             */         
   uint64_t        ObjHnd;          /* Object handle                 */         
   char            Intent[1];       /* Object access intent          */         
                                    /*  Valid values:                */         
                                    /*   QMY_READ_ONLY               */         
                                    /*   QMY_UPDATABLE               */         
   char            CmtLvl[1];       /* Commit level                  */         
                                    /*  Valid values:                */         
                                    /*   QMY_NONE                    */         
                                    /*   QMY_READ_UNCOMMITTED        */         
                                    /*   QMY_READ_COMMITTED          */         
                                    /*   QMY_REPEATABLE_READ         */         
                                    /*   QMY_SERIALIZABLE            */         
   char            Reserved2[14];   /* Reserved for future use       */         
} Qmy_MOIX0100_t;                                                               
                                                                                
/*-------------------------------------------------------------------*/         
/* Override object instance                                          */         
/*  -Reply format: Qmy_MOOX0100_none                                 */         
/*  -Table or index instance scope                                   */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MOOX0100 {                                                   
   uint16_t        Format;          /* QMY_OBJECT_OVERRIDE           */         
   char            Reserved1[2];    /* Reserved for future use       */         
   int32_t         CnnHnd;          /* Connection handle             */         
   uint64_t        ObjHnd;          /* Object handle                 */         
   uint64_t        OutSpcHnd;       /* Output buffer handle          */         
   char            Reserved2[8];    /* Reserved for future use       */         
   uint16_t        NxtRowOff;       /* Offset from beginning of      */         
                                    /*   each row in space to        */         
                                    /*   beginning of next row       */         
   char            Reserved3[14];   /* Reserved for future use       */         
} Qmy_MOOX0100_t;                                                               
                                                                                
/*-------------------------------------------------------------------*/         
/* Process commitment control                                        */         
/*  -Reply format: Qmy_MCCX0100_output                               */         
/*  -Application connection scope                                    */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MCCX0100 {                                                   
   uint16_t        Format;          /* QMY_PROCESS_COMMITMENT_CONTROL*/         
   char            Reserved1[2];    /* Reserved for future use       */         
   int32_t         CnnHnd;          /* Connection handle             */         
   char            Reserved2[8];    /* Reserved for future use       */         
   char            Function[1];     /* Function                      */         
                                    /*  Valid values:                */         
                                    /*   QMY_COMMIT                  */         
                                    /*   QMY_ROLLBACK                */         
   char            Reserved3[15];   /* Reserved for future use       */         
} Qmy_MCCX0100_t;                                                               
                                                                                
/* Commitment control function values                                */         
#define QMY_COMMIT                     0x01                                     
#define QMY_ROLLBACK                   0x02                                     
                                                                                
/*-------------------------------------------------------------------*/         
/* Process savepoint                                                 */         
/*  -Reply format: Qmy_MSPX0100_output                               */         
/*  -Application connection scope                                    */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MSPX0100 {                                                   
   uint16_t        Format;          /* QMY_PROCESS_SAVEPOINT         */         
   char            Reserved1[2];    /* Reserved for future use       */         
   int32_t         CnnHnd;          /* Connection handle             */         
   char            Reserved2[8];    /* Reserved for future use       */         
   uint32_t        SavPtNamOff;     /* Offset to savepoint name      */         
   uint32_t        SavPtNamLen;     /* Length of savepoint name      */         
   char            Function[1];     /* Function                      */         
                                    /*  Valid values:                */         
                                    /*   QMY_SET_SAVEPOINT           */         
                                    /*   QMY_RELEASE_SAVEPOINT       */         
                                    /*   QMY_ROLLBACK_SAVEPOINT      */         
   char            Reserved3[7];    /* Reserved for future use       */         
} Qmy_MSPX0100_t;                                                               
                                                                                
/* Savepoint function values                                         */         
#define QMY_SET_SAVEPOINT              0x01                                     
#define QMY_RELEASE_SAVEPOINT          0x02                                     
#define QMY_ROLLBACK_SAVEPOINT         0x03                                     
                                                                                
/*-------------------------------------------------------------------*/         
/* Process XA                                                        */         
/*  -Reply format: Qmy_MXAX0100_output                               */         
/*  -Application connection scope                                    */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MXAX0100 {                                                   
   uint16_t        Format;          /* QMY_PROCESS_XA                */         
   char            Reserved1[2];    /* Reserved for future use       */         
   int32_t         CnnHnd;          /* Connection handle             */         
   char            Reserved2[8];    /* Reserved for future use       */         
   uint32_t        XIDOff;          /* Offset to XID                 */         
   uint32_t        XIDLen;          /* Length of XID                 */         
   char            Function[1];     /* Function                      */         
                                    /*  Valid values:                */         
                                    /*   QMY_XA_START                */         
                                    /*   QMY_XA_END                  */         
                                    /*   QMY_XA_PREPARE              */         
                                    /*   QMY_XA_ROLLBACK             */         
                                    /*   QMY_XA_COMMIT               */         
                                    /*   QMY_XA_RECOVER              */         
   char            Reserved3[7];    /* Reserved for future use       */         
} Qmy_MXAX0100_t;                                                               
                                                                                
/* XA function values                                                */         
#define QMY_XA_START                   0x01                                     
#define QMY_XA_END                     0x02                                     
#define QMY_XA_PREPARE                 0x03                                     
#define QMY_XA_ROLLBACK                0x04                                     
#define QMY_XA_COMMIT                  0x05                                     
#define QMY_XA_RECOVER                 0x06                                     
                                                                                
/*-------------------------------------------------------------------*/         
/* Prepare/open cursor                                               */         
/*  -Reply format: Qmy_MSPO0100_output                               */         
/*  -Storage engine scope                                            */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MSPO0100 {                                                   
   uint16_t        Format;          /* QMY_PREPARE_OPEN_CURSOR       */         
   char            Reserved1[2];    /* Reserved for future use       */         
   int32_t         CnnHnd;          /* Connection handle             */         
   char            Reserved2[8];    /* Reserved for future use       */         
   uint64_t        StmtsSpcHnd;     /* Statement space handle        */         
                                    /*  Space format:                */         
                                    /*   - char     SrtSeqNam[10]    */         
                                    /*   - char     SrtSeqSch[10]    */         
                                    /*   - char     Reserved[12]     */         
                                    /*   - uint32_t Length           */         
                                    /*   - char     Text[*]          */         
   char            StmtCCSID[2];    /* Statement CCSID               */         
   char            Catalog[1];      /* Catalog cursor                */         
                                    /*  Valid values:                */         
                                    /*   QMY_NO                      */         
                                    /*   QMY_YES                 @04A*/         
   char            Reserved3[1];    /* Reserved for future use   @04C*/         
   uint32_t        FmtSpcLen;       /* Length of format space    @04A*/         
   uint64_t        FmtSpcHnd;       /* Format space handle       @04A*/         
   char            Reserved4[8];    /* Reserved for future use   @04A*/         
} Qmy_MSPO0100_t;                                                               
                                                                                
/*-------------------------------------------------------------------*/         
/* Quiesce table instance                                            */         
/*  -Reply format: Qmy_MQSC0100_output                               */         
/*  -Table share or table or index instance scope                    */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MQSC0100 {                                                   
   uint16_t        Format;          /* QMY_QUIESCE_OBJECT            */         
   char            Reserved1[6];    /* Reserved for future use       */         
   uint64_t        ObjHnd;          /* Object handle                 */         
} Qmy_MQSC0100_t;                                                               
                                                                                
/*-------------------------------------------------------------------*/         
/* Read rows                                                         */         
/*  -Reply format: Qmy_MRDX0100_output                               */         
/*  -Table or index instance scope                                   */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MRDX0100 {                                                   
   uint16_t        Format;          /* QMY_READ_ROWS                 */         
   char            Reserved1[2];    /* Reserved for future use       */         
   int32_t         CnnHnd;          /* Connection handle             */         
   uint64_t        ObjHnd;          /* Object handle                 */         
   char            Reserved2[8];    /* Reserved for future use       */         
   uint64_t        OutSpcHnd;       /* Output buffer handle          */         
   uint64_t        OutRRNSpcHnd;    /* Output RRN data space handle  */         
   uint64_t        KeySpcHnd;       /* Key space handle              */         
   char            Reserved3[4];    /* Reserved for future use       */         
                                    /*   from beginning of key space */         
   uint32_t        RelRowNbr;       /* Relative row number           */         
   uint32_t        KeyColsLen;      /* Length of key columns         */         
   uint32_t        KeyColsNbr;      /* Number of key columns         */         
   char            Intent[1];       /* Object access intent          */         
                                    /*  Valid values:                */         
                                    /*   QMY_READ_ONLY               */         
                                    /*   QMY_UPDATABLE               */         
   char            CmtLvl[1];       /* Commit level                  */         
                                    /*  Valid values:                */         
                                    /*   QMY_NONE                    */         
                                    /*   QMY_READ_UNCOMMITTED        */         
                                    /*   QMY_READ_COMMITTED          */         
                                    /*   QMY_REPEATABLE_READ         */         
                                    /*   QMY_SERIALIZABLE            */         
   char            RtnData[1];      /* Return data                   */         
                                    /*  Valid values:                */         
                                    /*   QMY_RETURN_DATA             */         
                                    /*   QMY_RETURN_NO_DATA          */         
   char            Async[1];        /* Async read                    */         
                                    /*  Valid values:                */         
                                    /*   QMY_NO                      */         
                                    /*   QMY_YES                     */         
   char            Orientation[1];  /* Orientation                   */         
                                    /*  Valid values:                */         
                                    /*   QMY_EQUAL                   */         
                                    /*   QMY_BEFORE_EQUAL            */         
                                    /*   QMY_BEFORE_OR_EQUAL         */         
                                    /*   QMY_AFTER_EQUAL             */         
                                    /*   QMY_AFTER_OR_EQUAL          */         
                                    /*   QMY_NEXT_EQUAL              */         
                                    /*   QMY_PREVIOUS_EQUAL          */         
                                    /*   QMY_LAST_PREVIOUS           */         
                                    /*	 QMY_PREFIX_LAST             */          
                                    /*   QMY_FIRST                   */         
                                    /*   QMY_LAST                    */         
                                    /*   QMY_NEXT                    */         
                                    /*   QMY_PREVIOUS                */         
                                    /*   QMY_SAME                    */         
   char            Reserved4[3];    /* Reserved for future use   @04C*/         
   uint32_t        AsyncLowBnd;     /* Async read low bound          */         
                                    /*  Valid values:                */         
                                    /*   0-4294967295            @04A*/         
   int32_t         PipeDesc;        /* Pipe descriptor               */         
                                    /*  Valid values:                */         
                                    /*   QMY_REUSE                   */         
                                    /*   0-2147483647                */         
} Qmy_MRDX0100_t;                                                               
                                                                                
/* Read return data types                                            */         
#define QMY_RETURN_DATA                0x00                                     
#define QMY_RETURN_NO_DATA             0x01                                     
                                                                                
/* Read keyed orientation types                                      */         
#define QMY_EQUAL                      0x0B                                     
#define QMY_BEFORE_EQUAL               0x09                                     
#define QMY_BEFORE_OR_EQUAL            0x0A                                     
#define QMY_AFTER_EQUAL                0x0D                                     
#define QMY_AFTER_OR_EQUAL             0x0C                                     
#define QMY_PREFIX_LAST                0x10                                     
#define QMY_LAST_PREVIOUS              0x11                                     
#define QMY_NEXT_EQUAL                 0x0E                                     
#define QMY_PREVIOUS_EQUAL             0x0F                                     
                                                                                
                                                                                
/* Generic orientation types                                         */         
#define QMY_FIRST                      0x01                                     
#define QMY_LAST                       0x02                                     
#define QMY_NEXT                       0x03                                     
#define QMY_PREVIOUS                   0x04                                     
#define QMY_SAME                       0x21                                     
                                                                                
/* Object access intent values                                       */         
#define QMY_READ_ONLY                  0x80                                     
#define QMY_UPDATABLE                  0xF0                                     
                                                                                
/* Object access intent values                                       */         
#define QMY_REUSE                      -1                                       
                                                                                
/*-------------------------------------------------------------------*/         
/* Reorganize table                                                  */         
/*  -Reply format: Qmy_MRGX0100_output                               */         
/*  -Table share scope                                               */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MRGX0100 {                                                   
   uint16_t        Format;          /* QMY_REORGANIZE_TABLE          */         
   char            Reserved1[2];    /* Reserved for future use       */         
   int32_t         CnnHnd;          /* Connection handle             */         
   uint64_t        ShrHnd;          /* Share handle                  */         
} Qmy_MRGX0100_t;                                                               
                                                                                
/*-------------------------------------------------------------------*/         
/* Release row lock                                                  */         
/*  -Reply format: Qmy_MRRX0100_output                               */         
/*  -Table or index instance scope                                   */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MRRX0100 {                                                   
   uint16_t        Format;          /* QMY_RELEASE_ROW               */         
   char            Reserved1[2];    /* Reserved for future use       */         
   int32_t         CnnHnd;          /* Connection handle             */         
   uint64_t        ObjHnd;          /* Object handle                 */         
   char            Intent[1];       /* Object access intent          */         
                                    /*  Valid values:                */         
                                    /*   QMY_READ_ONLY               */         
                                    /*   QMY_UPDATABLE               */         
   char            Reserved2[15];   /* Reserved for future use       */         
} Qmy_MRRX0100_t;                                                               
                                                                                
/*-------------------------------------------------------------------*/         
/* Update row                                                        */         
/*  -Reply format: Qmy_MUPD0100_output                               */         
/*  -Object instance scope                                           */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MUPD0100 {                                                   
   uint16_t        Format;          /* QMY_UPDATE_ROW                */         
   char            Reserved1[2];    /* Reserved for future use       */         
   int32_t         CnnHnd;          /* Connection handle             */         
   uint64_t        ObjHnd;          /* Object handle                 */         
   uint64_t        InSpcHnd;        /* Input buffer handle           */         
   uint32_t        RelRowNbr;       /* Relative row number           */         
   char            Reserved2[4];    /* Reserved for future use       */         
} Qmy_MUPD0100_t;                                                               
                                                                                
/*-------------------------------------------------------------------*/         
/* Write row                                                         */         
/*  -Reply format: Qmy_MWRT0100_output                               */         
/*  -Object instance scope                                           */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MWRT0100 {                                                   
   uint16_t        Format;          /* QMY_WRITE_ROWS                */         
   char            Reserved1[2];    /* Reserved for future use       */         
   int32_t         CnnHnd;          /* Connection handle             */         
   uint64_t        ObjHnd;          /* Object handle                 */         
   uint64_t        InSpcHnd;        /* Input buffer handle           */         
   char            Reserved2[8];    /* Reserved for future use       */         
   char            CmtLvl[1];       /* Commit level                  */         
                                    /*  Valid values:                */         
                                    /*   QMY_NONE                    */         
                                    /*   QMY_READ_UNCOMMITTED        */         
                                    /*   QMY_READ_COMMITTED          */         
                                    /*   QMY_REPEATABLE_READ         */         
                                    /*   QMY_SERIALIZABLE            */         
   char            Reserved3[15];   /* Reserved for future use       */         
} Qmy_MWRT0100_t;                                                               
                                                                                
                                                                                
                                                                                
/*********************************************************************/         
/* Reply parameter space structures                                  */         
/*********************************************************************/         
/* Common error                                                      */         
/*********************************************************************/         
typedef struct Qmy_Error_output {                                               
   char            JobName[10];     /* Name of job detecting error   */         
   char            JobUser[10];     /* User of job detecting error   */         
   char            JobNbr[6];       /* Number of job detecting error */         
   char            MsgId[7];        /* Message id for error          */         
   char            Reserved1[15];   /* Reserved for future use       */         
} Qmy_Error_output_t;                                                           
                                                                                
/*-------------------------------------------------------------------*/         
/* Allocate object instance                                          */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MAOI0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
   uint64_t        ObjHnd;          /* Object handle                 */         
   char            Reserved1[8];    /* Reserved for future use       */         
} Qmy_MAOI0100_output_t;                                                        
                                                                                
/*-------------------------------------------------------------------*/         
/* Allocate object share                                             */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MAOS0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
} Qmy_MAOS0100_output_t;                                                        
                                                                                
/*-------------------------------------------------------------------*/         
/* Cleanup storage engine                                            */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MCLN0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
} Qmy_MCLN0100_output_t;                                                        
                                                                                
/*-------------------------------------------------------------------*/         
/* Close application connection                                      */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MCCN0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
} Qmy_Qmy_MCCN0100_output_t;                                                    
                                                                                
/*-------------------------------------------------------------------*/         
/* Deallocate object share or object instance                        */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MDLC0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
} Qmy_MDLC0100_output_t;                                                        
                                                                                
/*-------------------------------------------------------------------*/         
/* Delete row                                                        */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MDLT0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
} Qmy_MDLT0100_output_t;                                                        
                                                                                
/*-------------------------------------------------------------------*/         
/* Describe constraint information                                   */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MDCT0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
   uint32_t        NeededLen;       /* Size of space needed to       */         
                                    /*   return all constraint info  */         
                                    /*   in bytes.                   */         
   uint32_t        CstCnt;          /* Constraint count              */         
   char            Reserved1[8];    /* Reserved for future use       */         
} Qmy_MDCT0100_output_t;                                                        
                                                                                
/*-------------------------------------------------------------------*/         
/* Describe object                                                   */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MDSO0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
   uint64_t        ObjLen;          /* Object length                 */         
   uint64_t        RowCnt;          /* Row count                     */         
   uint64_t        DltRowCnt;       /* Deleted row count             */         
   uint64_t        PageCnt;         /* Page count                    */         
   uint64_t        EstIoCnt;        /* Estimated # of I/O's          */         
   uint32_t        MeanRowLen;      /* Mean row length               */         
   char            Reserved1[12];   /* Reserved for future use       */         
   ILE_time_t      ModTim;          /* Time of last modification     */         
   char            Reserved2[3];    /* Reserved for future use       */         
   ILE_time_t      CrtTim;          /* Create time                   */         
   char            Reserved3[11];   /* Reserved for future use       */         
} Qmy_MDSO0100_output_t;                                                        
                                                                                
/*-------------------------------------------------------------------*/         
/* Describe range                                                    */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MDRG0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
   uint64_t        RecCnt;          /* Number of rows                */         
   uint16_t        RtnCode;         /* Return code                   */         
                                    /*  Valid values:                */         
                                    /*   QMY_SUCCESS                 */         
                                    /*   QMY_FAILURE                 */         
                                    /*   QMY_EARLY_EXIT              */         
   char            Reserved1[6];    /* Reserved for future use       */         
} Qmy_MDRG0100_output_t;                                                        
                                                                                
/* ESTDSIKR specific return codes                                    */         
#define QMY_EARLY_EXIT                 0x0002                                   
                                                                                
/*-------------------------------------------------------------------*/         
/* Execute immediate                                                 */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MSEI0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
} Qmy_MSEI0100_output_t;                                                        
                                                                                
/*-------------------------------------------------------------------*/         
/* Initialize storage engine                                         */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MINI0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
   uint16_t        Version;         /* Storage engine API version@04A*/         
   char            Reserved1[14];   /* Reserved for future use   @04A*/         
} Qmy_MINI0100_output_t;                                                        
                                                                                
/*-------------------------------------------------------------------*/         
/* Initialize object instance                                        */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MOIX0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
   uint16_t        InNullMapOff;    /* Offset from beginning of      */         
                                    /*  each row in input buffer     */         
                                    /*  to it's null byte map        */         
   uint16_t        InNxtRowOff;     /* Offset from beginning of      */         
                                    /*  each row in input buffer     */         
                                    /*  to beginning of next row     */         
   uint16_t        OutNullMapOff;   /* Offset from beginning of      */         
                                    /*  each row in output buffer    */         
                                    /*  to it's null byte map        */         
   uint16_t        OutNxtRowOff;    /* Offset from beginning of      */         
                                    /*  each row in output buffer    */         
                                    /*  to beginning of next row     */         
   char            Reserved1[8];    /* Reserved for future use       */         
} Qmy_MOIX0100_output_t;                                                        
                                                                                
/*-------------------------------------------------------------------*/         
/* Interrupt asyncronous read rows                                   */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MINT0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
} Qmy_MINT0100_output_t;                                                        
                                                                                
/*-------------------------------------------------------------------*/         
/* Lock object                                                       */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MOLX0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
} Qmy_MOLX0100_output_t;                                                        
                                                                                
/*-------------------------------------------------------------------*/         
/* Reorganize table                                                  */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MRGX0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
} Qmy_MRGX0100_output_t;                                                        
                                                                                
/*-------------------------------------------------------------------*/         
/* Override object instance                                          */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MOOX0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
} Qmy_MOOX0100_output_t;                                                        
                                                                                
/*-------------------------------------------------------------------*/         
/* Prepare/open cursor                                               */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MSPO0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
   uint64_t        ObjHnd;          /* Object handle                 */         
   char            Reserved1[8];    /* Reserved for future use       */         
   uint16_t        InNullMapOff;    /* Offset from beginning of      */         
                                    /*  each row in input buffer     */         
                                    /*  to it's null byte map        */         
   uint16_t        InNxtRowOff;     /* Offset from beginning of      */         
                                    /*  each row in input buffer     */         
                                    /*  to beginning of next row     */         
   uint16_t        OutNullMapOff;   /* Offset from beginning of      */         
                                    /*  each row in output buffer    */         
                                    /*  to it's null byte map        */         
   uint16_t        OutNxtRowOff;    /* Offset from beginning of      */         
                                    /*  each row in output buffer    */         
                                    /*  to beginning of next row     */         
   char            Reserved2[8];    /* Reserved for future use       */         
} Qmy_MSPO0100_output_t;                                                        
                                                                                
/*-------------------------------------------------------------------*/         
/* Process commitment control                                        */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MCCX0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
} Qmy_MCCX0100_output_t;                                                        
                                                                                
/*-------------------------------------------------------------------*/         
/* Process savepoint                                                 */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MSPX0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
} Qmy_MSPX0100_output_t;                                                        
                                                                                
/*-------------------------------------------------------------------*/         
/* Process XA                                                        */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MXAX0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
} Qmy_MXAX0100_output_t;                                                        
                                                                                
/*-------------------------------------------------------------------*/         
/* Quiesce table instance                                            */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MQSC0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
} Qmy_MQSC0100_output_t;                                                        
                                                                                
/*-------------------------------------------------------------------*/         
/* Read rows                                                         */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MRDX0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
} Qmy_MRDX0100_output_t;                                                        
                                                                                
/*-------------------------------------------------------------------*/         
/* Release row lock                                                  */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MRRX0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
} Qmy_MRRX0100_output_t;                                                        
                                                                                
/*-------------------------------------------------------------------*/         
/* Update row                                                        */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MUPD0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
   uint32_t        DupRRN;          /* Duplicate RRN                 */         
   uint32_t        DupObjNamOff;    /* Duplicate object offset       */         
   uint32_t        DupObjNamLen;    /* Duplicate object length       */         
   char            Reserved1[4];    /* Reserved for future use       */         
} Qmy_MUPD0100_output_t;                                                        
                                                                                
/*-------------------------------------------------------------------*/         
/* Write row                                                         */         
/*-------------------------------------------------------------------*/         
typedef struct Qmy_MWRT0100_output {                                            
   Qmy_Error_output_t ErrInfo;      /* Error information             */         
   int64_t         NewIdVal;        /* New identity value            */         
   uint32_t        DupRRN;          /* Duplicate RRN                 */         
   uint32_t        DupObjNamOff;    /* Duplicate object offset       */         
   uint32_t        DupObjNamLen;    /* Duplicate object length       */         
   char            NewIdGen[1];     /* New identity was generated    */         
                                    /*  Valid values:                */         
                                    /*   QMY_NO                      */         
                                    /*   QMY_YES                     */         
   char            IdCycles[1];     /* Identity cycles               */         
                                    /*  Valid values:                */         
                                    /*   QMY_NO                      */         
                                    /*   QMY_YES                     */         
   char            Reserved1[6];    /* Reserved for future use       */         
   int32_t         IdIncrement;     /* Identity increment            */         
} Qmy_MWRT0100_output_t;                                                        
                                                                                
                                                                                
#ifdef __OS400__                                                                
  #pragma pack(pop)                                                             
#else                                                                           
  #pragma options align=reset                                                   
#endif /* __OS400__ */                                                          
                                                                                
#ifdef __cplusplus                                                              
  }                                                                             
#endif                                                                          
                                                                                
#endif                                                                          
