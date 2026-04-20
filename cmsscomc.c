/*
 * cmsscomc.c
 *
 * C function part of GCCLIB support for the CMS SUBCOM facility.
 * Contains the following public methods:
 *
 * -- CMSSubcomSET (__scmset entry name) define a SUBCOM processor
 * -- CMSSubcomCLR (__scmclr entry name) delete a SUBCOM processor
 * -- CMSSubcomQRY (__scmqry entry name) locates a SCBLOCK
 *
 * Includes the following non-published functions:
 * -- SubcomCleanup performs SUBCOM cleanup at process exit
 * -- normalizeName normalized a C string to a CMS token
 * -- findSubcomReg searches the list of active SUBCOMs
 *
 *  Created on: Mar 1, 2026
 *      Author: WilliamDenton
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <cmsruntm.h>
#include <gcccrab.h>
#include <cmssys.h>
#include <cmsscom.h>

/********************************************************************/
/*  Normalize a string to a tokenized CMS value                     */
/*  -- exactly eight bytes long                                     */
/*  -- left-aligned, blank padded                                   */
/*  -- upper-case                                                   */
/*  -- NO terminating null character                                */
/*                                                                  */
/*  void normalizeName(name, result)                                */
/*                                                                  */
/*  name   : the C string containing the value to be normalized.    */
/*                                                                  */
/*  result : pointer to eight bytes of storage that will be set     */
/*           set to the results of the normalizing action.          */
/*                                                                  */
/*  If 'result" is null, this function does nothing.                */
/*  If 'name' is null, the result is eight spaces.                  */
/*  If 'name' is longer than eight bytes, only the first            */
/*  eight are used.                                                 */
/*                                                                  */
/*  NOTE: the logic here depends on the EBCDIC collating sequence   */
/*        where: uppercase = lowercase | blank                      */
/********************************************************************/
static void normalizeName (char *name, char *result) {
	int i;
    if (result != NULL) {
        memcpy(result, "        ", 8);
        if (name != NULL) {
            int lim = strlen(name);
            if (lim > 8) {
                lim = 8;
            }
            for (i = 0; i < lim; i++) {
                if (isalpha(name[i])) {
                    result[i] |= name[i];
                } else {
                    result[i] = name[i];
                }
            }
        }
    }
}

/********************************************************************/
/*  Locate a previously registered SUBCOM processor                 */
/*                                                                  */
/*  SUBCOMREG findSubcomReg(name, back)                             */
/*                                                                  */
/*  name : points to the normalized eight byte name of the SUBCOM   */
/*         service registration information to locate               */
/*                                                                  */
/*  back : (optional) is a pointer to a field that will be set to   */
/*         the address of the SUBCOMREG list element that is linked */
/*         to the element with the matching name.                   */
/*                                                                  */
/*  Returns the previously created SUBCOMREG element with a name    */
/*  that matches the 'name' parameter.                              */
/********************************************************************/
static SUBCOMREG *findSubcomReg (void *name, SUBCOMREG **back) {
    GCCCRAB *gcccrab = GETGCCCRAB();
    SUBCOMREG *result = gcccrab->subcomlist;

    if (back != NULL) {
        *back = &(gcccrab->subcomlist);
    }
    while (result != NULL) {
        if (memcmp(result->subcom_name, name, 8) == 0) {
            return result;
        }
        if (back != NULL) {
            *back = result;
        }
        result = result->nextReg;
    }
    return NULL;
}

/********************************************************************/
/*  Register (or replace) a SUBCOM processor                        */
/*                                                                  */
/*  int CMSSubcomSET(name, handler)                                 */
/*                                                                  */
/*  name    : the C string name for  the new (or replaces SUBCOM    */
/*            processor function                                    */
/*                                                                  */
/*  handler : is a pointer to the SUBCOM processor function         */
/*            with the following signature:                         */
/*                                                                  */
/*            int (handler) (scblock, calltype, plist, eplist       */
/*                                                                  */
/*            scblock  : pointer to the SCBLOCK for the SUBCOM      */
/*                                                                  */
/*            calltype : is the type of CMS call that invoked       */
/*                       this SUBCOM call. (enum CallType)          */
/*                                                                  */
/*            plist    : points to the PLIST passed by the caller   */
/*                                                                  */
/*            eplist   : points to the EPLIST passed by the caller  */
/*                       or is NULL if none was provided.           */
/*                                                                  */
/*  Returns the integer return code from the CMS SUBCOM SET call    */
/********************************************************************/
void SubcomCleanup (int rc);           /* forward reference         */

int __scmset (char *name, SUBCOM_HANDLER *handler) {
    int rc = 0;
    char workname[8];
    GCCCRAB *gcccrab = GETGCCCRAB();

    normalizeName(name, &workname);

    /* overload __exit if this is the first
     * SUBCOM processor to be registered.
     * This is done to insure all active
     * SUBCOMs will be deleted before the
     * program returns to CMS.
     */
    if (gcccrab->subcomlist == NULL) {
    	gcccrab->saveexit = gcccrab->exitfunc;
        gcccrab->exitfunc = SubcomCleanup;
    }

    SUBCOMREG *scomreg = findSubcomReg(workname, NULL);

    /* if the SUBCOM provider is already registered,
     * just update its processing function. Otherwise,
     * construct a new SUBCOMREG structure and call
     * __subcom to define the new processor to CMS.
     */
    if (scomreg != NULL) {
        scomreg->handler = handler;
    } else {
        scomreg = malloc(sizeof(SUBCOMREG));
        /* add to the list of SUBCOMs (LIFO) */
        scomreg->nextReg = gcccrab->subcomlist;
        gcccrab->subcomlist = scomreg;
        /* fill in the rest */
        scomreg->handler = handler;
        scomreg->cmscrab = GETCMSCRAB();
        scomreg->gcccrab = gcccrab;
        memcpy(scomreg->subcom_name, workname, 8);

        /* call the CMS SUBCOM SET function */
        rc = (int)__subcom(SCMSET, scomreg);
    }
    return rc;
}

/********************************************************************/
/*  Delete a SUBCOM processor's registration with CMS               */
/*                                                                  */
/*  int CMSSubcomCLR(name)                                          */
/*                                                                  */
/*  name    : the C string name for the currently active SUBCOM     */
/*            processor function                                    */
/*                                                                  */
/*  Returns the integer return code from the CMS SUBCOM CLR call    */
/********************************************************************/
int __scmclr (char *name) {
    int rc = 0;
    char workname[8];
    GCCCRAB *gcccrab = GETGCCCRAB();

    normalizeName(name, &workname);

    SUBCOMREG *scomback = NULL;
    SUBCOMREG *scomreg = findSubcomReg(workname, &scomback);
    if (scomreg != NULL) {
        scomback = scomreg->nextReg; /* remove from list */
        if (gcccrab->subcomlist == NULL) {
            /* fixup exitfunc if we just removed the last SUBCOM */
        	gcccrab->exitfunc = gcccrab->saveexit;
        }
        rc = (int)__subcom(SCMCLR, scomreg);
    }
    return rc;
}

/********************************************************************/
/*  Locate the SCBLOCK for an active SUBCOM processor               */
/*                                                                  */
/*  int CMSSubcomQRY(name)                                          */
/*                                                                  */
/*  name    : the C string name for the SUBCOM processor whose      */
/*            SCBLOCK is to be located                              */
/*                                                                  */
/*  Returns the address of the SCBLOCK describing the SUBCOM        */
/*  processor or NULL if no processor matches the 'name'            */
/*  (The SCBLOCK structure is defined in cmsscom.h and by the       */
/*  SCBLOCK MACRO in CMS.)                                          */
/********************************************************************/
SCBLOCK *__scmqry (char *name) {
    SCBLOCK *result = NULL;
    char workname[8];

    normalizeName(name, &workname);

    SUBCOMREG *scomreg = findSubcomReg(workname, NULL);
    if (scomreg != NULL) {
        result = (SCBLOCK*)__subcom(SCMQRY, scomreg);
    }
    return result;
}

/********************************************************************/
/*  Cleanup after any SUBCOM processors that were defined by        */
/*  insuring that their definitions have been cleared in CMS.       */
/*                                                                  */
/*  Called as part of the GCCLIB process termination (both normal   */
/*  and abnormal endings.)                                          */
/*                                                                  */
/*  Will transfer to the normal exit function (__exit) once the     */
/*  SUBCOM cleanup is complete.                                     */
/********************************************************************/
void SubcomCleanup (int rc) {
    GCCCRAB *gcccrab = GETGCCCRAB();
    SUBCOMREG *next = gcccrab->subcomlist;
    while (next != NULL) {
        SUBCOMREG *this = next;
        next = this->nextReg;
        __subcom(SCMCLR, this);
        free(this);
    }
    if (gcccrab->saveexit != NULL) {
    	(gcccrab->saveexit)(rc);
    }
}
