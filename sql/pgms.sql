SET client_min_messages TO warning;
SET log_min_messages    TO warning;

CREATE TYPE spectrum;

CREATE TYPE spectrumrange AS RANGE (
    SUBTYPE = float4
);

CREATE OR REPLACE FUNCTION spectrum_input(cstring)
    RETURNS spectrum
    AS 'spectrum'
    LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT;

CREATE OR REPLACE FUNCTION spectrum_output(spectrum)
    RETURNS cstring
    AS 'spectrum'
    LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT;

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
CREATE OR REPLACE FUNCTION load_from_mgf(Oid)
    RETURNS SETOF record
    AS 'mgf', 'load_mgf_lo'
    LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT;

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
CREATE OR REPLACE FUNCTION load_from_mgf(varchar)
    RETURNS SETOF record
    AS 'mgf', 'load_mgf_varchar'
    LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT;

--- Compute cosine greedy similarity score
--- @param spectrum reference spectrum
--- @param spectrum query spectrum
--- @param float4 tolerance
--- @param float4 mass power
--- @param float4 intenzity power
--- @return cosine greedy similarity score
CREATE OR REPLACE FUNCTION cosine_greedy(spectrum, spectrum, float4=0.1, float4=0.0, float4=1.0)
    RETURNS float4
    AS 'cosine_greedy'
    LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT COST 1000;

--- Compute cosine hungarian similarity score
--- @param spectrum reference spectrum
--- @param spectrum query spectrum
--- @param float4 tolerance
--- @param float4 mass power
--- @param float4 intenzity power
--- @return cosine hungarian similarity score
CREATE FUNCTION cosine_hungarian(spectrum, spectrum, float4=0.1, float4=0.0, float4=1.0)
    RETURNS float4
    AS 'cosine_hungarian'
    LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT COST 1000;

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
            select t.peak/COALESCE(NULLIF( MAX(t.peak) over (),0), 1) as peak 
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

--- Read given text literal in JSON format and returns the set of records
--- @param jsonb JSON formated text
--- @return Set of untyped records with selected columns
--- select * from pgms.load_from_json('
--- {
---  "peaks_json": [
---      [
---           289.286377,
---          8068.0
---      ],
---      [
---          295.545288,
---          22507.0
---      ],
---      [
---          298.489624,
---          3925.0
---      ],
---      [
---          317.3249511231221321465465,
---          18742.0
---      ]
---  ]
--- }'::jsonb) as (
---    spectrum pgms.spectrum
---);
CREATE OR REPLACE FUNCTION load_from_json(jsonb)
    RETURNS SETOF record
    AS 'json'
    LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT;

--- In case of float4 mass precursor function just returns its value. In case of array of values the function returns the 1st value of array
--- @param float4 mass precursor
--- @return valid mass precursor
CREATE OR REPLACE FUNCTION precursor_mz_correction(f float4) 
  RETURNS float4 AS
$BODY$
BEGIN
    return f;
END;
$BODY$
  LANGUAGE plpgsql IMMUTABLE;

--- In case of float4 mass precursor function just returns its value. In case of array of values the function returns the 1st value of array
--- @param float4[] mass precursor
--- @return valid mass precursor
CREATE OR REPLACE FUNCTION precursor_mz_correction(a float4[]) 
  RETURNS float4 AS
$BODY$
BEGIN
    return a[1];
END;
$BODY$
  LANGUAGE plpgsql IMMUTABLE;

--- Compute modified cosine similarity score
--- @param spectrum reference spectrum
--- @param spectrum query spectrum
--- @param float4 pepmass shift (reference_pepmass - query_pepmass)
--- @param float4 tolerance (default value 1.0)
--- @param float4 mass power (default value 0.0)
--- @param float4 intenzity power (default value 1.0)
--- @return modified cosine similarity score
CREATE OR REPLACE FUNCTION cosine_modified(spectrum, spectrum, float4, float4=0.1, float4=0.0, float4=1.0)
    RETURNS float4
    AS 'modified_cosine', 'modified_cosine'
    LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT COST 1000;

--- Compute intersection of masses as similarity score
--- @param spectrum reference spectrum
--- @param spectrum query spectrum
--- @param float4 tolerance
--- @return intersection similarity score
CREATE OR REPLACE FUNCTION intersect_mz(spectrum, spectrum, float4=0.1)
    RETURNS float4
    AS 'intersect_mz_match' LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT COST 1000;

--- Compute  similarity score based on precursor
--- @param float4 reference precursor
--- @param float4 query precursor
--- @param float4 tolerance
--- @param varchar type of tolerance [Dalton, ppm](default 'Dalton')
--- @return precursor similarity score
CREATE OR REPLACE FUNCTION precurzor_mz_match(float4, float4, float4=1.0, varchar='Dalton')
    RETURNS float4
    AS 'precurzor_mz_match'
    LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT COST 1000;

