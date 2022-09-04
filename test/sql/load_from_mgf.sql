\set ECHO none
BEGIN;

\i test/pgtap-core.sql
\i sql/pgms.sql

SELECT plan(12);

SELECT is_empty(
    $$ SELECT FROM load_from_mgf('$$||v||$$') AS (s spectrum) $$,
    'mgf parser should deal with none mgf format'
)FROM unnest(ARRAY[
   '',
   '
',
   '	',
   'unacceptable'
]) AS v;

SELECT results_eq(
    $$ SELECT s FROM load_from_mgf('$$||v||$$') AS (s spectrum) $$,
    $$ VALUES('{ {189.48956}, {1.9} }'::spectrum) $$,
    'mgf parser should deal with indent'
)FROM unnest(ARRAY[
'BEGIN IONS
189.48956 1.9
END IONS',
'BEGIN IONS
	189.48956    1.9
END IONS'
]) AS v;

SELECT results_eq(
    $$ SELECT s FROM load_from_mgf('$$||v||$$') AS (s spectrum) $$,
    $$ VALUES('{ {189.123456789, 189.00, -189, 189.48956, 189.48956}, {1.9, 1.9, -1.9, -1.123456789101112, 0} }'::spectrum) $$,
    'mgf parser should deal with sign fixures'
)FROM unnest(ARRAY[
'BEGIN IONS
189.123456789 1.9
+189.00 +1.9
-189 -1.9
189.48956+ 1.123456789101112-
189.48956 0
END IONS'
]) AS v;

SELECT results_eq(
    $$ SELECT "PEPMASS" FROM load_from_mgf('$$||v||$$') AS ("PEPMASS" float4) $$,
    $$ VALUES(-3.14::float4) $$,
    'mgf parser should deal with suffix sign signarure notation'
)FROM unnest(ARRAY[
'BEGIN IONS
PEPMASS=3.14-
END IONS'
]) AS v;

SELECT results_eq(
    $$ SELECT "PEPMASS" FROM load_from_mgf('$$||v||$$') AS ("PEPMASS" float4[]) $$,
    $$ VALUES(ARRAY[-3.14, 2, 2.73, -1.12]) $$,
    'mgf parser should deal with arrays'
)FROM unnest(ARRAY[
'BEGIN IONS
PEPMASS=3.14- 2 +2.73 -1.12
END IONS'
]) AS v;

SELECT row_eq(
    $$ SELECT s FROM load_from_mgf('$$||v||$$') AS (s spectrum) $$,
    ROW(),
    'mgf parser should deal with comments'
)FROM unnest(ARRAY[
'!comment
 #comment
	;comment
BEGIN IONS
END IONS
'
]) AS v;

SELECT results_eq(
    $$ SELECT "TITLE" FROM load_from_mgf('$$||v||$$') AS ("TITLE" varchar) $$,
    $$ VALUES('computed parameter') $$,
    'mgf parser should deal with global parameters'
)FROM unnest(ARRAY[
'TITLE=computed parameter
BEGIN IONS
END IONS
',
'TITLE=global parameter
BEGIN IONS
TITLE=computed parameter
END IONS
'
]) AS v;

SELECT * FROM finish();
ROLLBACK;