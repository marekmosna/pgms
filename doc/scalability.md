# Mass Spectrometry Application Scalability

In the README is described the very basic approach how to use the extension. Here we are going to present how to scale and modularize the end application.

## Database schema
When you create the main table add there the autoincremented column as id mostly known as primary key.

```sql
create table spectrums (
  id SERIAL,
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
```
And create new table for particular purpose with the relation to the origin one. For instance normalized spactrum could be:

```sql
create table norm_spectrums (
  id SERIAL,
  spectrum_id int,
  spectrum pgms.spectrum
);
```
## Import data
When you're importing the data load them without any further modification.

```sql
insert into spectrums (scans, inchikey, ionmode, charge, name, pepmass, exactmass, libraryquality, molecular_formula, smiles, inchi, spectrum)  select * from pgms.load_from_mgf(/*lo_id*/) as (
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

and initialize the table by normalized spectrums

```sql
insert into norm_spectrums (spectrum_id, spectrum) select id, pgms.spectrum_normalize(spectrum) from spectrums;
```
 

## Example queries

select spectrums that are similar to the spectrum defined as a literal using the cosine_greedy similarity function

```sql
select name, molecular_formula, pepmass from spectrums
inner join norm_spectrums
on spectrums.id = norm_spectrums.spectrum_id
where pgms.cosine_greedy(norm_spectrums.spectrum, 
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
