#include <postgres.h>
#include <plpgsql.h>

#include <catalog/pg_type.h>
#include <catalog/namespace.h>
#include <utils/syscache.h>

#include "spectrum.h"

PG_MODULE_MAGIC;

void _PG_init()
{
    Oid spaceid = LookupExplicitNamespace("pgms", true);
    if(OidIsValid(spaceid))
    {
#if PG_VERSION_NUM >= 120000
        spectrumOid = GetSysCacheOid2(TYPENAMENSP, Anum_pg_type_oid, PointerGetDatum("spectrum"), ObjectIdGetDatum(spaceid));
#else
        spectrumOid = GetSysCacheOid2(TYPENAMENSP, PointerGetDatum("spectrum"), ObjectIdGetDatum(spaceid));
#endif
    }
}
