\set ECHO none
BEGIN;

\i test/pgtap-core.sql
\i sql/pgms.sql

SELECT plan(5);

SELECT is(
    ROUND(
        (cosine_neutral_losses(spectrum_normalize(ref), spectrum_normalize(query), ref_pepmass_mz, query_pepmass_mz, tolerance))::numeric,
        6),
    expected_score,
    'cosine_neutral_losses should be' || to_char(expected_score, '0D000000')
) FROM (VALUES
    (
        '{{100, 150, 200, 300, 500, 510, 1100}, {0.7,0.2,0.1,1,0.2,0.005,0.5}}'::spectrum,
        '{{55, 105, 205, 304.5, 494.5, 515.5, 1045}, {0.7,0.2,0.1,1,0.2,0.005,0.5}}'::spectrum,
        0.1::float4, 1000.0::float4, 1005.0::float4, 0.081966
    ),
    (
        '{{100, 299, 300, 301, 500, 510}, {0.02,1,0.2,0.4,0.04,0.2}}'::spectrum,
        '{{105, 305, 306, 505, 517}, {0.02,1,0.2,0.04,0.2}}'::spectrum,
        2.0::float4, 1000.0::float4, 1005.0::float4, 0.967873
    ),
    (
        '{{100, 110, 200, 300, 400, 500}, {1, 0.5, 0.01, 0.8, 0.01, 0.01}}'::spectrum,
        '{{110, 200, 300, 310, 700}, {1, 0.01, 0.9, 0.5, 0.01}}'::spectrum,
        0.1::float4, 1000.0::float4, 1010.0::float4, 0.709427
    ),
    (
        '{{100, 200, 300}, {0.25, 0.25, 1}}'::spectrum,
        '{{120, 220, 320}, {0.25, 0.25, 1}}'::spectrum,
        8.0::float4, 1000.0::float4, 1010.0::float4, 0.0
    ),
    (
        '{{100, 200, 300}, {0.1, 1.0, 0.5}}'::spectrum,
        '{{120, 220, 320}, {0.02, 0.02, 1}}'::spectrum,
        2.0::float4, 270.0::float4, 289.5::float4,0.019591
    )

) v(ref, query, tolerance, ref_pepmass_mz, query_pepmass_mz, expected_score);

SELECT * FROM finish();
ROLLBACK;
