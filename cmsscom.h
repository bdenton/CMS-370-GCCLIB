/*
 * cmsscom.h
 *
 *  Defines the Types, structures, and functions that are part
 *  of the GCCLIB support for the CMS SUBCOM facility.
 *
 *  The GCCLIB code in support of SUBCOM is contained in:
 *  -- cmsscomc c        : Public C functions
 *  -- cmsscom  assemble : Low-level Assembler functions
 *
 *  Created on: Feb 27, 2026
 *      Author: WilliamDenton
 */

#if !defined(CMSSCom_included)
#define CMSSCom_included

#include <cmssys.h>

typedef struct SUBCOMREG SUBCOMREG;
typedef struct SCBLOCK SCBLOCK;

typedef enum {
/*                         VM/CE  EPLIST                             */
/*    Flag    Value        Used?  Avail?  Meaning                    */
    EPLFPROG = 0x00,    /*   Y       N    Program-to-Program         */
    EPLFCMND = 0x01,    /*   Y       Y    Address COMMAND            */
    EPLFSBCM = 0x02,    /*   Y       Y    SUBCOM call                */
    EPLFNNUE = 0x03,    /*           Y    No NUCEXT, extended plist  */
    EPLFNNUT = 0x04,    /*           N    No NUCEXT, tokenized plist */
    EPLFRXFN = 0x05,    /*   Y       Y    Rexx external function     */
                        /*                6-word EPLIST is present   */
    EPLFIMMD = 0x06,    /*           Y    IMMCMD call                */
    EPLFSRCH = 0x0B,    /*   Y       Y    Command search             */
    EPLFEXEC = 0x10,    /*           N    Invoked by BPX1EXC         */
    EPLFENDC = 0xFE,    /*           N    End-of-command call        */
    EPLFABEN = 0xFF     /*           N    Abend or NUCXDROP          */
} CallType;

/* Synonyms for the only calltypes used in VM/CE */
#define EPLFNCFL EPLFCMND     /* also called CMS_COMMAND in cmssys.h */
#define EPLCMDFL EPLFSRCH     /* also called CMS_CONSOLE in cmssys.h */
#define EPFUNSUB EPLFRXFN

typedef int (SUBCOM_HANDLER) (
    SCBLOCK        *scblock,
    CallType        callType,
    PLIST          *plist,
    EPLIST         *eplist
  );

struct SCBLOCK {
    struct SCBLOCK     *scbfwptr;      /* -> next SCBLOCK            */
    void               *scbwkwrd;      /* User word: for GCCLIB, this*/
                                       /* will point to a SUBCOMREG  */
    char                scbname[8];    /* Name of SUBCOM processor   */
    union {
        char            scbpsw[8];     /* Starting PSW for subcommand*/
      struct {                         /* mapped as:                 */
        char            scbint;        /*   Interrupt mask           */
        char            scbkey;        /*   Storage protect key      */
        char            scbflag;       /*   System flags             */
        char            scbuflag;      /*   User flags               */
        SUBCOM_HANDLER *scbentr;       /*   -> SUBCOM processor entry*/
      };
    };
    void               *scbxorg;       /* Where NUCEXT was loaded    */
    int                 scbxlen;       /* Loaded NUCEXT length or 0  */
    char                scbflg2;       /* Second flag byte           */
    char                scbavl1[3];    /* reserved                   */
    char                scbsegid[8];   /* Logical segment identifier */
    void               *scbtestk;      /* Thread EXECCOM stack (ESA+)*/
};

struct SUBCOMREG {
    SUBCOMREG          *nextReg;       /* -> next in list (LIFO)     */
    SUBCOM_HANDLER     *handler;       /* -> SUBCOM processor fcn    */
    struct CMSCRAB     *cmscrab;       /* -> CMSCRAB when SET issued */
    struct GCCCRAB     *gcccrab;       /* -> GCCCRAB GCCLIB anchor   */
    char                subcom_name[8];/* SUBCOM name (no \0 ending) */
};

/* Public SUBCOM functions */

extern int __scmset (char *name, SUBCOM_HANDLER *handler);
#define CMSSubcomSET(name, handler) (__scmset(name, handler))

extern int __scmclr (char *name);
#define CMSSubcomCLR(name) (__scmclr(name))

extern SCBLOCK *__scmqry (char *name);
#define CMSSubcomQRY(name) (__scmqry(name))

/* Low-level SUBCOM related assembler function */

typedef enum {
    SCMSET = 1,
    SCMCLR = 2,
    SCMQRY = 3
} SubcomOp;

extern void *__subcom (SubcomOp op, SUBCOMREG *subcomreg);
#define CMScallSubcom(op, reg) (__subcom(op, reg))

#endif /* CMSSCom_included */
