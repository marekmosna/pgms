-- more similarity functions

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
---    "peak_json" pgms.spectrum
---);
CREATE FUNCTION load_from_json(jsonb) RETURNS SETOF record AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT;

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
CREATE FUNCTION cosine_modified(spectrum, spectrum, float4, float4=0.1, float4=0.0, float4=1.0) RETURNS float4 AS 'MODULE_PATHNAME', 'modified_cosine' LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT COST 1000;

--- Compute intersection of masses as similarity score
--- @param spectrum reference spectrum
--- @param spectrum query spectrum
--- @param float4 tolerance
--- @return intersection similarity score
CREATE FUNCTION intersect_mz(spectrum, spectrum, float4=0.1) RETURNS float4 AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT COST 1000;

--- Compute  similarity score based on precursor
--- @param float4 reference precursor
--- @param float4 query precursor
--- @param float4 tolerance
--- @param varchar type of tolerance [Dalton, ppm](default 'Dalton')
--- @return precursor similarity score
CREATE FUNCTION precurzor_mz_match(float4, float4, float4=1.0, varchar='Dalton') 
  RETURNS float4 
  AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT COST 1000;

--- Compute  similarity score based on precursor
--- @param float4[] reference precursor array
--- @param float4[] query precursor array
--- @param float4 tolerance
--- @param varchar type of tolerance [Dalton, ppm](default 'Dalton')
--- @param boolean is symetric
--- @return precursor similarity score array
CREATE OR REPLACE FUNCTION precurzor_mz_match(float4[], float4[], float4=1.0, varchar='Dalton', boolean=false)
  RETURNS float4[]
  AS 'MODULE_PATHNAME', 'precurzor_mz_match_array'
  LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT COST 1000;
