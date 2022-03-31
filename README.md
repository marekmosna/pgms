# Postgres Mass Spectrometry Extension

The extension was developed and tested on Linux Ubuntu 20.04.3 against Postgresql 12.9

## Requirements

On Debian based Linux distributions install following packages
```bash
sudo apt-get install git postgresql build-essential autoconf libtool
```
For Debian 11

```bash
sudo apt install postgresql-server-dev-13
```

For Ubuntu 20.04

```bash
sudo apt install postgresql-server-dev-12
```

## Build

```bash
git clone https://bioinfo.uochb.cas.cz/gitlab/chemdb/pgms.git
cd pgms

autoreconf -i
./configure
make
```

## Install

```bash
sudo make install
```

## Setup PosgreSQL database

Please, log into your PostgreSQL server console as the superuser. A user without admin privileges cannot create an extension using external libraries.

```bash
psql -U postgres
```

```sql
create role test with login;
\password test
create database test owner test;
\c test
create extension pgms;
alter schema pgms owner to test;
\q
```

## Import data

Download the sources.

```bash
wget -O isdb.mgf https://zenodo.org/record/5607264/files/MultiSources_ISDB_pos.mgf
```

This particular source of data is malformed so it's necessary to sanitize them locally by:

```bash
sed -i '/END IONS/,/BEGIN IONS/{/IONS/!d}' isdb.mgf
```

Please, log into your PostgreSQL server console and import the data into the database. 

```bash
psql -U test -d test
```


The names of the columns are derived from the paramaters which the input file contains.

```sql
create table spectrums (
  scans integer,
  inchikey varchar,
  ionmode varchar,
  charge integer,
  name varchar,
  pepmass float,
  exactmass varchar,
  libraryquality varchar,
  molecular_formula varchar,
  smiles varchar,
  inchi varchar,
  spectrum pgms.spectrum
);

\lo_import isdb.mgf

insert into spectrums select * from pgms.load_from_mgf(:LASTOID) as (
    "SCANS" integer,
    "INCHIKEY" varchar,
    "IONMODE" varchar,
    "CHARGE" integer,
    "NAME" varchar,
    "PEPMASS" float,
    "EXACTMASS" varchar,
    "LIBRARYQUALITY" varchar,
    "MOLECULAR_FORMULA" varchar,
    "SMILES" varchar,
    "INCHI" varchar,
    spectrum pgms.spectrum
);
```

In case the normalized spectrums are required, use

```sql
update spectrums set spectrum = pgms.spectrum_normalize(spectrum);
```

## Example queries

select spectrums that are similar to the spectrum defined as a literal using the cosine_greedy similarity function

```sql
select name, molecular_formula, pepmass from spectrums
where pgms.cosine_greedy(spectrum, 
(
  select pgms.spectrum_normalize(spectrum) from pgms.load_from_mgf(
'BEGIN IONS
81.0334912 0.72019481254
83.04914126 1.4943088127053332
93.0334912 1.06672542884
95.04914126 0.4251834313366667
97.02840582 0.39885925914889997
101.0597059 0.11745238494333332
105.0334912 2.56022986611
107.04914129999999 2.989378747
109.0284058 0.17182621511313334
109.0647913 0.12125843647166666
111.04405590000002 1.1500647237000001
113.05970590000001 0.5300221888333333
119.0127558 0.6070848631693333
121.0284058 0.8470290901486667
121.0647913 0.1403327763183
123.04405590000002 2.79474183067
125.05970590000001 0.12530122563666668
127.075356 0.14029347218
135.04405590000002 2.2115351786760002
137.0597059 6.7625559016666665
139.0389705 0.1284870753791
139.075356 1.6466003053333333
149.02332040000002 1.15255865488
151.0389705 1.2670624623676667
151.075356 1.2451936371333334
153.0546206 0.6290735267500001
165.0182351 0.469445566508
165.0546206 1.0380875693
167.0338851 0.16699011076966666
167.0702706 0.22768428286
177.0546206 0.11160554283333333
179.0338851 0.4426202476666667
179.0702706 0.14182684573233337
181.0495352 0.6932895193
183.06518530000002 0.035763648595666664
191.0338851 0.51335409714
193.0495352 0.3155917361433333
195.06518530000002 0.4056783311
207.0287997 0.22443527437333333
209.04444980000002 0.07395862408266667
217.0495352 0.19593685042683331
219.0287997 0.24606538356666666
END IONS') as (spectrum pgms.spectrum))
) > 0.7;
```

select spectrums that are similar to the spectrum with `MOLECULAR_FORMULA` equals to `C66H95N13O12` using the cosine_greedy similarity function

```sql
select found_molecular_formula, found_inchi, found_pepmass, found_smiles, searched_smiles, cosine_greedy_score from
(
  select 
    s.molecular_formula as found_molecular_formula, 
    s.inchi as found_inchi,
    s.pepmass as found_pepmass, 
    s.smiles as found_smiles, 
    q.smiles as searched_smiles, 
    pgms.cosine_greedy(s.spectrum, q.spectrum) as cosine_greedy_score 
      from spectrums as s,
  (
    select spectrum, smiles from spectrums where molecular_formula = 'C66H95N13O12'
  ) as q
) as t
where cosine_greedy_score > 0.9
order by cosine_greedy_score desc;
```

select spectrums that are similar to the spectrum `MOLECULAR_FORMULA` equals to `C66H95N13O12` using the cosine_hungarian similarity function

```sql
select found_molecular_formula, found_inchi, found_pepmass,found_name, searched_name, cosine_hungarian_score from
(
  select
    s.molecular_formula as found_molecular_formula,
    s.inchi as found_inchi,
    s.pepmass as found_pepmass,
    s.name as found_name,
    q.name as searched_name,
    pgms.cosine_hungarian(s.spectrum, q.spectrum) as cosine_hungarian_score
      from spectrums as s,
  (
    select spectrum, name from spectrums where molecular_formula = 'C66H95N13O12'
  ) as q
) as t
where cosine_hungarian_score > 0.9
order by cosine_hungarian_score desc;
```

names of the spectrums with cosine_hungarian and cosine_greedy scores bigger than `0.8` against spectrums with `MOLECULAR_FORMULA` equals to `C66H95N13O12`  

```sql
select found_name, searched_name, score_hungarian, score_greedy from
(
    select 
        s.name as found_name,
        q.name as searched_name,
        pgms.cosine_hungarian(s.spectrum, q.spectrum) as score_hungarian,
        pgms.cosine_greedy(s.spectrum, q.spectrum) as score_greedy
        from spectrums as s,
        (
            select spectrum, name from spectrums where molecular_formula = 'C66H95N13O12'
        ) as q
) as t 
where score_hungarian > 0.8 and score_greedy > 0.8 order by score_hungarian desc;
```
