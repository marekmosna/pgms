\set ECHO none
BEGIN;

\i test/pgtap-core.sql
\i sql/pgms.sql

SELECT plan(3);

SELECT row_eq(
    $$ SELECT spectrum_normalize('{{1.0, 2.0},{2.0, 1.0}}'::spectrum) $$,
    ROW('{{1.0, 2.0},{1.0, 0.5}}'::spectrum),
    'normalize of the spectrum should match'
);

SELECT row_eq(
    $$ SELECT spectrum_normalize('{{2.0, 1.0, 3.0},{2.0, 1.0, 4.0}}'::spectrum) $$,
    ROW('{{1.0, 2.0, 3.0},{0.25, 0.5, 1}}'::spectrum),
    'normalize of the unordered spectrum should match'
);

SELECT row_eq(
    $$ SELECT spectrum_normalize('{{1.0, 2.0},{0.0, 0.0}}'::spectrum) $$,
    ROW('{{1.0, 2.0},{0.0, 0.0}}'::spectrum),
    'normalize of the spectrum should remains in case of MAX is zero'
);

SELECT * FROM finish();
ROLLBACK;
