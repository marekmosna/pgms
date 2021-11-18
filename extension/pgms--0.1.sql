-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pgms" to load this file. \quit


CREATE TYPE spectrum;

CREATE FUNCTION spectrum_input(cstring) RETURNS spectrum  AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT;
CREATE FUNCTION spectrum_output(spectrum) RETURNS cstring AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT;

CREATE TYPE spectrum
(
    internallength = VARIABLE,
    input = spectrum_input,
    output = spectrum_output,
    alignment = float
);


CREATE FUNCTION cosine_greedy(spectrum, spectrum, float4=0.1) RETURNS float4 AS 'MODULE_PATHNAME','cosine_greedy_simple' LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT COST 100;
CREATE FUNCTION cosine_greedy(spectrum, spectrum, float4, float4, float4) RETURNS float4 AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT COST 1000;

CREATE FUNCTION cosine_hungarian(spectrum, spectrum, float4=0.1, float4=0.0, float4=1.0) RETURNS float4 AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT COST 1000;
