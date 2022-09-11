\set ECHO none
BEGIN;

\i test/pgtap-core.sql
\i sql/pgms.sql

SELECT plan(26);

SELECT has_type('spectrum');
---SELECT has_type('spectrumrange');

SELECT has_function('load_from_mgf');
SELECT has_function('load_from_mgf', ARRAY['character varying']);
SELECT has_function('load_from_mgf', ARRAY['oid']);
SELECT function_returns('load_from_mgf', 'setof record');

SELECT has_function('spectrum_normalize');
SELECT has_function('spectrum_normalize', ARRAY['spectrum']);
SELECT function_returns('spectrum_normalize', 'spectrum');

SELECT has_function('cosine_greedy');
SELECT has_function('cosine_greedy', ARRAY['spectrum', 'spectrum', 'real', 'real', 'real']);
SELECT function_returns('cosine_greedy', 'real');

SELECT has_function('spectrum_range');
SELECT has_function('spectrum_range', ARRAY['spectrum', 'character varying']);
SELECT function_returns('spectrum_range', 'spectrumrange');

SELECT has_function('load_from_json');
SELECT has_function('load_from_json', ARRAY['jsonb']);
SELECT function_returns('load_from_json', 'setof record');

SELECT has_function('cosine_modified');
SELECT has_function('cosine_modified', ARRAY['spectrum', 'spectrum', 'real', 'real', 'real', 'real']);
SELECT function_returns('cosine_modified', 'real');

SELECT has_function('intersect_mz');
SELECT has_function('intersect_mz', ARRAY['spectrum', 'spectrum', 'real']);
SELECT function_returns('intersect_mz', 'real');

SELECT has_function('precurzor_mz_match');
SELECT has_function('precurzor_mz_match', ARRAY['real', 'real', 'real', 'character varying']);
SELECT function_returns('precurzor_mz_match', 'real');

SELECT * FROM finish();
ROLLBACK;
