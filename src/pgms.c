#include "pgms.h"

#include <postgres.h>
#include <fmgr.h>
#include <funcapi.h>
#include <catalog/namespace.h>
#include <catalog/pg_type.h>
#include <utils/syscache.h>


PG_MODULE_MAGIC;


Oid spectrumOid;


void _PG_init()
{
    Oid spaceid = LookupExplicitNamespace("pgms", false);
    spectrumOid = GetSysCacheOid2(TYPENAMENSP, Anum_pg_type_oid, PointerGetDatum("spectrum"), ObjectIdGetDatum(spaceid));
}

char* reverse_postfix_sign(char* value)
{
    size_t length = 0;
    char *psign = NULL;
    char* pbegin = value;
    
    if(!value || *value == '\0')
        goto end;

    length = strlen(value);
    psign = value + length - 1;

    while(likely(pbegin != psign) && isspace((unsigned char) *pbegin))
        pbegin++;

    while(likely(pbegin != psign) && isspace((unsigned char) *psign))
        psign--;

    if(unlikely(psign == pbegin))
        goto end;
    else if(*psign == POSITIVE_SIGN)
        *psign = '\0';
    else if(*psign == NEGATIVE_SIGN)
    {
        while(psign != pbegin)
        {
            char c = *(psign - 1);
            *psign = c;
            *(--psign) = NEGATIVE_SIGN;
        }
    }

end:
    elog(DEBUG1, "rotated: %s", value);
    return value;
}
