-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pgms" to load this file. \quit


CREATE TYPE spectrum;
CREATE TYPE spectrumrange AS RANGE (
    SUBTYPE = float4
);

CREATE FUNCTION spectrum_input(cstring) RETURNS spectrum  AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT;
CREATE FUNCTION spectrum_output(spectrum) RETURNS cstring AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT;

CREATE TYPE spectrum
(
    internallength = VARIABLE,
    input = spectrum_input,
    output = spectrum_output,
    alignment = float,
    storage = extended
);

CREATE CAST (spectrum AS float[][])
    WITH INOUT
    AS IMPLICIT;

CREATE CAST (float[][] AS spectrum)
    WITH INOUT
    AS IMPLICIT;

--- Read given Large Object Oid in Mascote Generic Format and returns the set of records
--- @param Oid Large Object identificator
--- @return Set of untyped records with selected columns
--- select * from pgms.load_from_mgf(:LASTOID) as (
---    "SCANS" integer,
---    "INCHIKEY" varchar,
---    "IONMODE" varchar,
---    "PEPMASS" float,
---    spectrum pgms.spectrum
---    ...
---);
CREATE FUNCTION load_from_mgf(Oid) RETURNS SETOF record         AS 'MODULE_PATHNAME', 'load_mgf_lo' LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT;

--- Read given text literal in Mascote Generic Format and returns the set of records
--- @param varchar MGF formated text
--- @return Set of untyped records with selected columns
--- select * from pgms.load_from_mgf(
--- 'BEGIN IONS
--- 81.0334912 0.72019481254
--- 83.04914126 1.4943088127053332
--- 93.0334912 1.06672542884
--- ...
--- END IONS') as (
---    spectrum pgms.spectrum
---);
CREATE FUNCTION load_from_mgf(varchar) RETURNS SETOF record     AS 'MODULE_PATHNAME', 'load_mgf_varchar' LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT;

--- Compute cosine greedy similarity score
--- @param spectrum reference spectrum
--- @param spectrum query spectrum
--- @param float4 tolerance
--- @return cosine greedy similarity score
CREATE FUNCTION cosine_greedy(spectrum, spectrum, float4=0.1) RETURNS float4 AS 'MODULE_PATHNAME','cosine_greedy_simple' LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT COST 100;

--- Compute cosine greedy similarity score
--- @param spectrum reference spectrum
--- @param spectrum query spectrum
--- @param float4 tolerance
--- @param float4 mass power
--- @param float4 intenzity power
--- @return cosine greedy similarity score
CREATE FUNCTION cosine_greedy(spectrum, spectrum, float4, float4, float4) RETURNS float4 AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT COST 1000;

--- Compute cosine hungarian similarity score
--- @param spectrum reference spectrum
--- @param spectrum query spectrum
--- @param float4 tolerance
--- @param float4 mass power
--- @param float4 intenzity power
--- @return cosine hungarian similarity score
CREATE FUNCTION cosine_hungarian(spectrum, spectrum, float4=0.1, float4=0.0, float4=1.0) RETURNS float4 AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT COST 1000;

--- Normalize mass spectrum (provides peaks in interval <0, 1>)
--- @param spectrum ion spectrum
--- @return normalized spectrum
CREATE OR REPLACE FUNCTION spectrum_normalize( s spectrum )
  RETURNS spectrum AS
$BODY$
BEGIN
    return array[array(
            select mz FROM unnest((s::float[])[1:1]) AS t(mz) order by t.mz
        )] || array[array(
            select t.peak/max(t.peak) over () as peak 
                FROM unnest((s::float[])[1:1], (s::float[])[2:2]) AS t(mz, peak)
                order by t.mz
        )];
END;
$BODY$
  LANGUAGE plpgsql IMMUTABLE;

-----------------------------------------------------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION spectrum_range( s spectrum, bounds varchar )
  RETURNS spectrumrange AS
$BODY$
declare
    mz_l float4;
    mz_r float4;
BEGIN
    select into mz_l, mz_r min(t.mz) over (), max(t.mz) over () as mz FROM unnest((s::float[])[1:1]) AS t(mz);
    return (left(bounds, 1) || mz_l || ','|| mz_r || right(bounds, 1))::pgms.spectrumrange;
END;
$BODY$
  LANGUAGE plpgsql IMMUTABLE;
