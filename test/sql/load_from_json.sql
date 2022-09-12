\set ECHO none
BEGIN;

\i test/pgtap-core.sql
\i sql/pgms.sql

SELECT plan(4);

SELECT is_empty(
    $$ SELECT FROM load_from_json(NULL) AS (s spectrum) $$,
    'json parser should deal with NULL json'
);

SELECT row_eq(
    $$ SELECT FROM load_from_json('$$||v||$$'::jsonb) AS (s spectrum) $$,
    ROW(),
    'json parser should deal with empty json'
)FROM unnest(ARRAY[
   '{}',
   '[]'
]) AS v;

SELECT results_eq(
    $$ SELECT "peaks_json" FROM load_from_json('$$||v||$$'::jsonb) AS ("peaks_json" spectrum) $$,
    $$ VALUES('{ {189.123456789, 189.00, -189, 189.48956}, {1.9, 1.9, -1.9, 0} }'::spectrum) $$,
    'json parser should deal with sign fixures'
)FROM unnest(ARRAY[
'{
  "peaks_json": [
      [
          189.123456789,
          1.9
      ],
      [
          189.00,
          1.9
      ],
      [
          -189,
          -1.9
      ],
      [
          189.48956,
          0
      ]
  ]
}'
]) AS v;

SELECT * FROM finish();
ROLLBACK;
