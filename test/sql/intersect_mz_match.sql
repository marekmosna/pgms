\set ECHO none
BEGIN;

\i test/pgtap-core.sql
\i sql/pgms.sql

SELECT plan(7);

SELECT is(
    intersect_mz(ref, query),
    expected,
    'intersect_mz should be' || to_char(expected, '0D99')
) FROM (VALUES
    (
        '{}'::spectrum, 
        '{}'::spectrum,
        0.0::float4
    ),
    (
        '{}'::spectrum, 
        '{{290}, {1.0}}'::spectrum,
        0.0::float4
    )
) v(ref, query, expected);

SELECT is(
    intersect_mz(ref, query),
    expected,
    'intersect_mz should be' || to_char(expected, '0D99')
) FROM (VALUES
    (
        '{{100, 200, 300,   500}, {1.0, 1.0, 1.0, 1.0}}'::spectrum, 
        '{{100, 200, 290, 499.9}, {1.0, 1.0, 1.0, 1.0}}'::spectrum,
        (3.00/4.00)::float4
    ),
    (
        '{{100, 200, 300,   500}, {1.0, 1.0, 1.0, 1.0}}'::spectrum, 
        '{{290}, {1.0}}'::spectrum,
        (0.00)::float4
    ),
    (
        '{{100, 200, 300,   500}, {1.0, 1.0, 1.0, 1.0}}'::spectrum, 
        '{{100, 200, 200.1, 499.9}, {1.0, 1.0, 1.0, 1.0}}'::spectrum,
        (3.00/4.00)::float4
    ),
    (
        '{{10 , 200, 300,   500}, {1.0, 1.0, 1.0, 1.0}}'::spectrum, 
        '{{100, 200, 200.1, 499.9}, {1.0, 1.0, 1.0, 1.0}}'::spectrum,
        (2.00/4.00)::float4
    ),
    (
        '{{100, 200, 200.1, 499.9}, {1.0, 1.0, 1.0, 1.0}}'::spectrum,
        '{{10 , 200, 300,   500}, {1.0, 1.0, 1.0, 1.0}}'::spectrum,
        (2.00/4.00)::float4
    )
) v(ref, query, expected);

SELECT * FROM finish();
ROLLBACK;
