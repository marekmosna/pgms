\set ECHO none
BEGIN;

\i test/pgtap-core.sql
\i sql/pgms.sql

SELECT plan(1);

SELECT is(
    intersect_mz(
        '{{100, 200, 300,   500}, {1.0, 1.0, 1.0, 1.0}}'::spectrum, 
        '{{100, 200, 290, 499.9}, {1.0, 1.0, 1.0, 1.0}}'::spectrum
        ),
    (1.00/3.00)::float4,
    'intersect_mz should be' || to_char((1.00/3.00)::float4, '0D99')
);

SELECT * FROM finish();
ROLLBACK;
