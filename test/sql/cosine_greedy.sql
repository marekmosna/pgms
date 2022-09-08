\set ECHO none
BEGIN;

\i test/pgtap-core.sql
\i sql/pgms.sql

SELECT plan(4);

SELECT is(
    ROUND(
        (cosine_greedy(spectrum_normalize(ref), spectrum_normalize(query), tolerance, mz_power, intensity_power))::numeric,
        6),
    expected_score,
    'cosine_greedy should be' || to_char(expected_score, '0D000000')
) FROM (VALUES
    (
        '{{100, 200, 300, 500, 510}, {0.1, 0.2, 1.0, 0.3, 0.4}}'::spectrum,
        '{{100, 200, 290, 490, 510}, {0.1, 0.2, 1.0, 0.3, 0.4}}'::spectrum,
        0.1::float4, 0.0::float4, 1.0::float4, 0.161538
    ),
    (
        '{{100, 299, 300, 301, 510}, {0.1, 1.0, 0.2, 0.3, 0.4}}'::spectrum,
        '{{100, 300, 301, 511}, {0.1, 1.0, 0.3, 0.4}}'::spectrum,
        0.2::float4, 0.0::float4, 1.0::float4, 0.234404
    ),
    (
        '{{100, 299, 300, 301, 510}, {0.1, 1.0, 0.2, 0.3, 0.4}}'::spectrum,
        '{{100, 300, 301, 511}, {0.1, 1.0, 0.3, 0.4}}'::spectrum,
        2.0::float4, 0.0::float4, 1.0::float4, 0.984495
    ),
    (
        '{{100, 200, 300, 500, 510}, {0.1, 0.2, 1.0, 0.3, 0.4}}'::spectrum,
        '{{100, 200, 290, 490, 510}, {0.1, 0.2, 1.0, 0.3, 0.4}}'::spectrum,
        1.0::float4, 0.5::float4, 2.0::float4, 0.042855
    )
) v(ref, query, tolerance, mz_power, intensity_power, expected_score);

SELECT * FROM finish();
ROLLBACK;
