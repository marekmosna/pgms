\set ECHO none
BEGIN;

\i test/pgtap-core.sql
\i sql/pgms.sql

SELECT plan(3);

SELECT is(
    precurzor_mz_match(ref, query, tolerance, tolerance_type)::numeric,
    expected,
    'precurzor_mz_match should be ' || expected
) FROM (VALUES
    (100.0::float4, 101.0::float4, 0.1::float4, 'Dalton', 0.0),
    (100.0::float4, 101.0::float4, 2.0::float4, 'Dalton', 1.0),
    (600.0::float4, 600.001::float4, 2.0::float4, 'ppm', 1.0)
) v(ref, query, tolerance, tolerance_type, expected);

SELECT * FROM finish();
ROLLBACK;
