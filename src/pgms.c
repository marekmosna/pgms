#include <postgres.h>
#include <fmgr.h>
#include <funcapi.h>
#include <catalog/namespace.h>
#include <utils/syscache.h>


PG_MODULE_MAGIC;


Oid spectrumOid;


void _PG_init()
{
    Oid spaceid = LookupExplicitNamespace("pgms", false);
    spectrumOid = GetSysCacheOid2(TYPENAMENSP, Anum_pg_type_oid, PointerGetDatum("spectrum"), ObjectIdGetDatum(spaceid));
}
