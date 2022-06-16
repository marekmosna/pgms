# Mass Spectrometry Extension API Documentation

## Parser functions

```sql
--- Read given Large Object Oid in Mascote Generic Format and returns the set of records
--- @param Oid Large Object identificator
--- @return Set of untyped records with selected columns
--- select * from pgms.load_from_mgf(:LASTOID) as (
---    "SCANS" integer,
---    "INCHIKEY" varchar,
---    "IONMODE" varchar,
---    "PEPMASS" float,
---    spectrum pgms.spectrum
---    ...
---);
load_from_mgf(Oid) RETURNS SETOF record

--- Read given text literal in Mascote Generic Format and returns the set of records
--- @param varchar MGF formated text
--- @return Set of untyped records with selected columns
--- select * from pgms.load_from_mgf(
--- 'BEGIN IONS
--- 81.0334912 0.72019481254
--- 83.04914126 1.4943088127053332
--- 93.0334912 1.06672542884
--- ...
--- END IONS') as (
---    spectrum pgms.spectrum
---);
load_from_mgf(varchar) RETURNS SETOF record

--- Read given text literal in JSON format and returns the set of records
--- @param jsonb JSON formated text
--- @return Set of untyped records with selected columns
--- select * from pgms.load_from_json('
--- {
---  "peaks_json": [
---      [
---           289.286377,
---          8068.0
---      ],
---      [
---          295.545288,
---          22507.0
---      ],
---      [
---          298.489624,
---          3925.0
---      ],
---      [
---          317.3249511231221321465465,
---          18742.0
---      ]
---  ]
--- }'::jsonb) as (
---    spectrum pgms.spectrum
---);
load_from_json(jsonb) RETURNS SETOF record
```
## Similarity evaluation functions

```sql
--- Compute cosine greedy similarity score
--- @param spectrum reference spectrum
--- @param spectrum query spectrum
--- @param float4 tolerance
--- @return cosine greedy similarity score
cosine_greedy(spectrum, spectrum, float4=0.1) RETURNS float4

--- Compute cosine greedy similarity score
--- @param spectrum reference spectrum
--- @param spectrum query spectrum
--- @param float4 tolerance
--- @param float4 mass power
--- @param float4 intenzity power
--- @return cosine greedy similarity score
cosine_greedy(spectrum, spectrum, float4, float4, float4) RETURNS float4

--- Compute cosine hungarian similarity score
--- @param spectrum reference spectrum
--- @param spectrum query spectrum
--- @param float4 tolerance
--- @param float4 mass power
--- @param float4 intenzity power
--- @return cosine hungarian similarity score
cosine_hungarian(spectrum, spectrum, float4=0.1, float4=0.0, float4=1.0) RETURNS float4

--- Compute modified cosine similarity score
--- @param spectrum reference spectrum
--- @param spectrum query spectrum
--- @param float4 pepmass shift (reference_pepmass - query_pepmass)
--- @param float4 tolerance (default value 1.0)
--- @param float4 mass power (default value 0.0)
--- @param float4 intenzity power (default value 1.0)
--- @return modified cosine similarity score
cosine_modified(spectrum, spectrum, float4, float4=0.1, float4=0.0, float4=1.0) RETURNS float4 

--- Compute intersection of masses as similarity score
--- @param spectrum reference spectrum
--- @param spectrum query spectrum
--- @param float4 tolerance
--- @return intersection similarity score
intersect_mz(spectrum, spectrum, float4=0.1) RETURNS float4

--- Compute  similarity score based on precursor
--- @param float4 reference precursor
--- @param float4 query precursor
--- @param float4 tolerance
--- @param varchar type of tolerance [Dalton, ppm](default 'Dalton')
--- @return precursor similarity score
precurzor_mz_match(float4, float4, float4=1.0, varchar='Dalton') RETURNS float4
```

## Filter functions

```sql
--- Normalize mass spectrum (provides peaks in interval <0, 1>)
--- @param spectrum ion spectrum
--- @return normalized spectrum
spectrum_normalize(spectrum) RETURNS spectrum

--- In case of float4 mass precursor function just returns its value. In case of array of values the function returns the 1st value of array
--- @param float4/float4[] mass precursor
--- @return valid mass precursor
precursor_mz_correction(float4) RETURNS float4
precursor_mz_correction(float4[]) RETURNS float4
```
