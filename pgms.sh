#!/bin/sh

psql -U postgres << 'EOF'
drop extension pgms cascade;
create extension pgms;
EOF

#psql -U postgres << 'EOF'
#select * from pgms.load_from_mgf(16481) as ("PEPMASS" float, "SMILES" varchar, spectrum pgms.spectrum);
#EOF

psql -U postgres << 'EOF'
select * from pgms.load_from_mgf('') as ("PEPMASS" float, "TITLE" varchar, spectrum pgms.spectrum);
EOF

psql -U postgres << 'EOF'
select * from pgms.load_from_mgf('
') as ("PEPMASS" float, "TITLE" varchar, spectrum pgms.spectrum);
EOF

psql -U postgres << 'EOF'
select * from pgms.load_from_mgf('hello') as ("PEPMASS" float, "TITLE" varchar, spectrum pgms.spectrum);
EOF

psql -U postgres << 'EOF'
select * from pgms.load_from_mgf('	
') as ("PEPMASS" float, "TITLE" varchar, spectrum pgms.spectrum);
EOF

#space return error
psql -U postgres << 'EOF'
select * from pgms.load_from_mgf('
BEGIN IONS
 189.48956 1.9
311.08008 1.3
399.99106 2.3
END IONS
') as (spectrum pgms.spectrum);
EOF

#tab parsed
psql -U postgres << 'EOF'
select * from pgms.load_from_mgf('
BEGIN IONS
	189.48956 1.9
311.08008 1.3
399.99106 2.3
END IONS
') as (spectrum pgms.spectrum);
EOF

psql -U postgres << 'EOF'
select * from pgms.load_from_mgf('
BEGIN IONS
PEPMASS=            498.34      25674.3       2-     
CHARGE=1+
TITLE=Example 2
189.48956 1.9
311.08008 1.3
399.99106 2.3
END IONS
BEGIN IONS
PEPMASS=413.26611887841
CHARGE=1+
TITLE=Example 2
189.48956 1.9
311.08008 1.3
399.99106 2.3
END IONS
') as ("PEPMASS" float[], "TITLE" varchar, spectrum pgms.spectrum);
EOF

psql -U postgres << 'EOF'
select * from pgms.load_from_mgf('BEGIN IONS
PEPMASS=413.26611887841
CHARGE=1+
TITLE=Example 1
189.48956 1.9
311.08008 1.3
399.99106 2.3
END IONS
BEGIN IONS
PEPMASS=413.26611887841
CHARGE= 1-
TITLE=Example 2
189.48956 1.9
311.08008 1.3
399.99106 2.3
END IONS
BEGIN IONS
PEPMASS=413.26611887841
CHARGE= 1+
TITLE=Example 3
189.48956 1.9
311.08008 1.3
399.99106 2.3
END IONS
BEGIN IONS
PEPMASS=413.26611887841
CHARGE=12-
TITLE=Example 4
189.48956 1.9
311.08008 1.3
399.99106 2.3
END IONS
') as ("PEPMASS" float, "TITLE" varchar, "CHARGE" integer, spectrum pgms.spectrum);
EOF

psql -U postgres << 'EOF'
select *, pgms.spectrum_normalize(spectrum) AS normalized from pgms.load_from_mgf('
BEGIN IONS

PEPMASS=413.26611887841
CHARGE=1+

TITLE=Example 2
189.48956 1.1
311.08008 3.3
399.99106 6.6
END IONS
BEGIN IONS

PEPMASS=413.26611887841
CHARGE=1+

TITLE=Example 2
189.48956 1.2

311.08008 0.6
399.99106 0.3
END IONS
') as ("PEPMASS" float, "TITLE" varchar, spectrum pgms.spectrum);
EOF

psql -U postgres << 'EOF'
select * from pgms.load_from_mgf('
BEGIN IONS
	
PEPMASS=413.26611887841
CHARGE=1+

TITLE=Example 2
189.48956 1.9
311.08008 1.3
	399.99106 2.3
END IONS
BEGIN IONS

PEPMASS=413.26611887841
CHARGE=1+

TITLE=Example 2
189.48956 1.9

311.08008 1.3
399.99106 2.3
END IONS
') as ("PEPMASS" float, "TITLE" varchar, spectrum pgms.spectrum);
EOF

psql -U postgres << 'EOF'
select * from pgms.load_from_mgf('
BEGIN IONS
END IONS
BEGIN IONS
PEPMASS=413.26611887841
CHARGE=1+

TITLE=       Example 3
  189.48956 1.9
311.08008 1.3
399.99106 2.3
END IONS
') as ("PEPMASS" float, "TITLE" varchar, spectrum pgms.spectrum);
EOF

psql -U postgres << 'EOF'
select * from pgms.load_from_mgf('
BEGIN IONS
PEPMASS=413.26611887841
CHARGE=1+

TITLE=       Example 3
  189.48956 1.9
311.08008 1.3
399.99106 
END IONS
') as ("PEPMASS" float, "TITLE" varchar, spectrum pgms.spectrum);
EOF

psql -U postgres << 'EOF'
select * from pgms.load_from_mgf('
BEGIN IONS
PEPMASS=413.26611887841
CHARGE=1+

TITLE=       Example 3
  189.48956 1.9
311.08008 1.3 399.99106
END IONS
') as ("PEPMASS" float, "TITLE" varchar, spectrum pgms.spectrum);
EOF

psql -U postgres << 'EOF'
select * from pgms.load_from_mgf('
BEGIN IONS
BEGIN IONS
PEPMASS=413.26611887841
CHARGE=1+

TITLE=       Example 3
  189.48956 1.9
311.08008 1.3 399.99106
END IONS
') as ("PEPMASS" float, "TITLE" varchar, spectrum pgms.spectrum);
EOF

#psql -U postgres << 'EOF'
#select * from pgms.load_from_mgf(49825) as ("PEPMASS" float, "SMILES" varchar);
#EOF

psql -U postgres << 'EOF'
select * from pgms.load_from_mgf('
TITLE=Global title
#comment
BEGIN IONS
PEPMASS=413.26611887841
CHARGE=1+
189.48956 1.9
311.08008 1.3
END IONS
BEGIN IONS
PEPMASS=413.26611887841
CHARGE=1-
189.48956 1.9
311.08008 1.3
END IONS
') as ("PEPMASS" float, "TITLE" varchar, spectrum pgms.spectrum);
EOF

psql -U postgres << 'EOF'
select * from pgms.load_from_mgf('
TITLE=Global tile
!comment
   #comment
      ;comment
BEGIN IONS
PEPMASS=413.26611887841
CHARGE=1+
189.48956 1.9
311.08008 1.3
END IONS
BEGIN IONS
PEPMASS=413.26611887841
TITLE=Local title
CHARGE=1-
189.48956 1.9
311.08008 1.3
END IONS
') as ("PEPMASS" float, "TITLE" varchar, spectrum pgms.spectrum);
EOF

psql -U postgres << 'EOF'
select pgms.spectrum_range(spectrum, '[)') as r from pgms.load_from_mgf('
BEGIN IONS
189.48956 1.9
311.08008 1.3
END IONS
BEGIN IONS
189.48956 1.9
311.08008 1.3
END IONS
') as (spectrum pgms.spectrum);
EOF

psql -U postgres << 'EOF'
select * from pgms.load_from_mgf('
BEGIN IONS
TITLE=Local1
precursor_mz=413.26611887841
END IONS
BEGIN IONS
TITLE=Local2
precursor_mz=413.00
END IONS
BEGIN IONS
TITLE=Local3
precursor_mz=113.00
END IONS
BEGIN IONS
TITLE=Local4
precursor_mz=390.00
END IONS
BEGIN IONS
TITLE=Local5
precursor_mz=470.00
END IONS
') as ("precursor_mz" float4, "TITLE" varchar)
where pgms.precurzor_mz_match(413.26611887841::float4 ,precursor_mz, 25.0::float4) = 1.0
;
EOF

psql -U postgres << 'EOF'
select * from pgms.load_from_mgf('
BEGIN IONS
TITLE=Local1
precursor_mz=413.26611887841
END IONS
BEGIN IONS
TITLE=Local2
precursor_mz=413.00
END IONS
BEGIN IONS
TITLE=Local3
precursor_mz=113.00
END IONS
BEGIN IONS
TITLE=Local4
precursor_mz=412.00
END IONS
BEGIN IONS
TITLE=Local5
precursor_mz=470.00
END IONS
') as ("precursor_mz" float4, "TITLE" varchar)
where pgms.precurzor_mz_match(413::float4 ,precursor_mz, 2500.0::float4, 'ppm') = 1.0
;
EOF

psql -U postgres << 'EOF'
select * from pgms.load_from_json('
[{
    "spectrum_id": "CCMSLIB00000001547",
    "source_file": "130618_Ger_Jenia_WT-3-Des-MCLR_MH981.4-qb.1.1..mgf",
    "task": "47daa4396adb426eaa5fa54b6ce7dd5f",
    "scan": "-1",
    "ms_level": "2",
    "library_membership": "GNPS-LIBRARY",
    "spectrum_status": "1",
    "splash": "splash10-0w2a-0001282259-0001282259",
    "submit_user": "mwang87",
    "compound_name": "3-Des-Microcystein_LR",
    "ion_source": "LC-ESI",
    "compound_source": "Isolated",
    "instrument": "qTof",
    "pi": "Gerwick",
    "data_collector": "Jenia",
    "adduct": "M+H",
    "precursor_mz": 981.54,
    "exactmass": "0.0",
    "charge": 1,
    "cas_number": "N/A",
    "pubmed_id": "N/A",
    "smiles": "CC(C)CC1NC(=O)C(C)NC(=O)C(=C)N(C)C(=O)CCC(NC(=O)C(C)C(NC(=O)C(CCCNC(N)=N)NC(=O)C(C)C(NC1=O)C(O)=O)\\C=C\\C(\\C)=C\\C(C)C(O)Cc1ccccc1)C(O)=O",
    "inchi": "InChI=1S/C48H72N10O12/c1-25(2)22-36-45(66)57-39(47(69)70)29(6)41(62)54-34(16-13-21-51-48(49)50)44(65)53-33(18-17-26(3)23-27(4)37(59)24-32-14-11-10-12-15-32)28(5)40(61)55-35(46(67)68)19-20-38(60)58(9)31(8)43(64)52-30(7)42(63)56-36/h10-12,14-15,17-18,23,25,27-30,33-37,39,59H,8,13,16,19-22,24H2,1-7,9H3,(H,52,64)(H,53,65)(H,54,62)(H,55,61)(H,56,63)(H,57,66)(H,67,68)(H,69,70)(H4,49,50,51)/b18-17+,26-23+",
    "inchiaux": "N/A",
    "library_class": "1",
    "spectrumid": "CCMSLIB00000001547",
    "ionmode": "positive",
    "create_time": "2019-10-30 21:18:25.0",
    "task_id": "aa87bf9cd0784df9956753f435c32434",
    "user_id": "null",
    "inchikey_smiles": "IYDKWWDUBYWQGF-NNAZGLEUSA-N",
    "inchikey_inchi": "",
    "formula_smiles": "C48H72N10O12",
    "formula_inchi": "",
    "url": "https://gnps.ucsd.edu/ProteoSAFe/gnpslibraryspectrum.jsp?SpectrumID=CCMSLIB00000001547",
    "parent_mass": 980.5327235480092,
    "inchikey": "IYDKWWDUBYWQGF-NNAZGLEUSA-N",
    "annotation_history": [{"Compound_Name": "3-Des-Microcystein_LR", "Ion_Source": "LC-ESI", "Compound_Source": "Isolated", "Instrument": "Ger", "PI": "Gerwick", "Data_Collector": "Jenia", "Adduct": "M+H", "Scan": "-1", "Precursor_MZ": "981.4", "ExactMass": "0.0", "Charge": "0", "CAS_Number": "N/A", "Pubmed_ID": "N/A", "Smiles": "N/A", "INCHI": "N/A", "INCHI_AUX": "N/A", "Library_Class": "1", "SpectrumID": "CCMSLIB00000001547", "Ion_Mode": "Positive", "create_time": "2014-02-04 17:56:43.0", "task_id": "47daa4396adb426eaa5fa54b6ce7dd5f", "user_id": "mwang87"}, {"Compound_Name": "3-Des-Microcystein_LR", "Ion_Source": "LC-ESI", "Compound_Source": "Isolated", "Instrument": "qTof", "PI": "Gerwick", "Data_Collector": "Jenia", "Adduct": "M+H", "Scan": "-1", "Precursor_MZ": "981.54", "ExactMass": "0.0", "Charge": "0", "CAS_Number": "N/A", "Pubmed_ID": "N/A", "Smiles": "CC(C)CC1NC(=O)C(C)NC(=O)C(=C)N(C)C(=O)CCC(NC(=O)C(C)C(NC(=O)C(CCCNC(N)=N)NC(=O)C(C)C(NC1=O)C(O)=O)\\C=C\\C(\\C)=C\\C(C)C(O)Cc1ccccc1)C(O)=O", "INCHI": "N/A", "INCHI_AUX": "N/A", "Library_Class": "1", "SpectrumID": "CCMSLIB00000001547", "Ion_Mode": "Positive", "create_time": "2019-10-30 21:18:25.0", "task_id": "aa87bf9cd0784df9956753f435c32434", "user_id": "lfnothias"}],
    "peaks_json": [
        [
            289.286377,
            8068.0
        ],
        [
            295.545288,
            22507.0
        ],
        [
            298.489624,
            3925.0
        ],
        [
            317.324951,
            18742.0
        ]
    ]
},
{
    "spectrum_id": "CCMSLIB00000001548",
    "source_file": "130618_Ger_Jenia_WT-3-Des-MCLR_MH981.4-qb.1.1..mgf",
    "task": "47daa4396adb426eaa5fa54b6ce7dd5f",
    "scan": "-1",
    "ms_level": "2",
    "library_membership": "GNPS-LIBRARY",
    "spectrum_status": "1",
    "splash": "splash10-0w2a-0001282259-0001282259",
    "submit_user": "mwang87",
    "compound_name": "3-Des-Microcystein_LR",
    "ion_source": "LC-ESI",
    "compound_source": "Isolated",
    "instrument": "qTof",
    "pi": "Gerwick",
    "data_collector": "Jenia",
    "adduct": "M+H",
    "precursor_mz": 981.54,
    "exactmass": "0.0",
    "charge": 1,
    "cas_number": "N/A",
    "pubmed_id": "N/A",
    "smiles": "CC(C)CC1NC(=O)C(C)NC(=O)C(=C)N(C)C(=O)CCC(NC(=O)C(C)C(NC(=O)C(CCCNC(N)=N)NC(=O)C(C)C(NC1=O)C(O)=O)\\C=C\\C(\\C)=C\\C(C)C(O)Cc1ccccc1)C(O)=O",
    "inchi": "InChI=1S/C48H72N10O12/c1-25(2)22-36-45(66)57-39(47(69)70)29(6)41(62)54-34(16-13-21-51-48(49)50)44(65)53-33(18-17-26(3)23-27(4)37(59)24-32-14-11-10-12-15-32)28(5)40(61)55-35(46(67)68)19-20-38(60)58(9)31(8)43(64)52-30(7)42(63)56-36/h10-12,14-15,17-18,23,25,27-30,33-37,39,59H,8,13,16,19-22,24H2,1-7,9H3,(H,52,64)(H,53,65)(H,54,62)(H,55,61)(H,56,63)(H,57,66)(H,67,68)(H,69,70)(H4,49,50,51)/b18-17+,26-23+",
    "inchiaux": "N/A",
    "library_class": "1",
    "spectrumid": "CCMSLIB00000001547",
    "ionmode": "positive",
    "create_time": "2019-10-30 21:18:25.0",
    "task_id": "aa87bf9cd0784df9956753f435c32434",
    "user_id": "null",
    "inchikey_smiles": "IYDKWWDUBYWQGF-NNAZGLEUSA-N",
    "inchikey_inchi": "",
    "formula_smiles": "C48H72N10O12",
    "formula_inchi": "",
    "url": "https://gnps.ucsd.edu/ProteoSAFe/gnpslibraryspectrum.jsp?SpectrumID=CCMSLIB00000001547",
    "parent_mass": 980.5327235480092,
    "inchikey": "IYDKWWDUBYWQGF-NNAZGLEUSA-N",
    "annotation_history": [{"Compound_Name": "3-Des-Microcystein_LR", "Ion_Source": "LC-ESI", "Compound_Source": "Isolated", "Instrument": "Ger", "PI": "Gerwick", "Data_Collector": "Jenia", "Adduct": "M+H", "Scan": "-1", "Precursor_MZ": "981.4", "ExactMass": "0.0", "Charge": "0", "CAS_Number": "N/A", "Pubmed_ID": "N/A", "Smiles": "N/A", "INCHI": "N/A", "INCHI_AUX": "N/A", "Library_Class": "1", "SpectrumID": "CCMSLIB00000001547", "Ion_Mode": "Positive", "create_time": "2014-02-04 17:56:43.0", "task_id": "47daa4396adb426eaa5fa54b6ce7dd5f", "user_id": "mwang87"}, {"Compound_Name": "3-Des-Microcystein_LR", "Ion_Source": "LC-ESI", "Compound_Source": "Isolated", "Instrument": "qTof", "PI": "Gerwick", "Data_Collector": "Jenia", "Adduct": "M+H", "Scan": "-1", "Precursor_MZ": "981.54", "ExactMass": "0.0", "Charge": "0", "CAS_Number": "N/A", "Pubmed_ID": "N/A", "Smiles": "CC(C)CC1NC(=O)C(C)NC(=O)C(=C)N(C)C(=O)CCC(NC(=O)C(C)C(NC(=O)C(CCCNC(N)=N)NC(=O)C(C)C(NC1=O)C(O)=O)\\C=C\\C(\\C)=C\\C(C)C(O)Cc1ccccc1)C(O)=O", "INCHI": "N/A", "INCHI_AUX": "N/A", "Library_Class": "1", "SpectrumID": "CCMSLIB00000001547", "Ion_Mode": "Positive", "create_time": "2019-10-30 21:18:25.0", "task_id": "aa87bf9cd0784df9956753f435c32434", "user_id": "lfnothias"}],
    "peaks_json": [
        [
            289.286377,
            8068.0
        ],
        [
            295.545288,
            22507.0
        ],
        [
            298.489624,
            3925.0
        ],
        [
            317.3249511231221321465465,
            18742.0
        ]
    ]
}]
'::jsonb) as ("precursor_mz" float4, "spectrum_id" varchar, "peaks_json" pgms.spectrum);
EOF

psql -U postgres << 'EOF'
select * from pgms.load_from_json('
{
  "spectrum_id": "CCMSLIB00000001548",
  "source_file": "130618_Ger_Jenia_WT-3-Des-MCLR_MH981.4-qb.1.1..mgf",
  "task": "47daa4396adb426eaa5fa54b6ce7dd5f",
  "scan": "-1",
  "ms_level": "2",
  "library_membership": "GNPS-LIBRARY",
  "spectrum_status": "1",
  "splash": "splash10-0w2a-0001282259-0001282259",
  "submit_user": "mwang87",
  "compound_name": "3-Des-Microcystein_LR",
  "ion_source": "LC-ESI",
  "compound_source": "Isolated",
  "instrument": "qTof",
  "pi": "Gerwick",
  "data_collector": "Jenia",
  "adduct": "M+H",
  "precursor_mz": 981.54,
  "exactmass": "0.0",
  "charge": 1,
  "cas_number": "N/A",
  "pubmed_id": "N/A",
  "smiles": "CC(C)CC1NC(=O)C(C)NC(=O)C(=C)N(C)C(=O)CCC(NC(=O)C(C)C(NC(=O)C(CCCNC(N)=N)NC(=O)C(C)C(NC1=O)C(O)=O)\\C=C\\C(\\C)=C\\C(C)C(O)Cc1ccccc1)C(O)=O",
  "inchi": "InChI=1S/C48H72N10O12/c1-25(2)22-36-45(66)57-39(47(69)70)29(6)41(62)54-34(16-13-21-51-48(49)50)44(65)53-33(18-17-26(3)23-27(4)37(59)24-32-14-11-10-12-15-32)28(5)40(61)55-35(46(67)68)19-20-38(60)58(9)31(8)43(64)52-30(7)42(63)56-36/h10-12,14-15,17-18,23,25,27-30,33-37,39,59H,8,13,16,19-22,24H2,1-7,9H3,(H,52,64)(H,53,65)(H,54,62)(H,55,61)(H,56,63)(H,57,66)(H,67,68)(H,69,70)(H4,49,50,51)/b18-17+,26-23+",
  "inchiaux": "N/A",
  "library_class": "1",
  "spectrumid": "CCMSLIB00000001547",
  "ionmode": "positive",
  "create_time": "2019-10-30 21:18:25.0",
  "task_id": "aa87bf9cd0784df9956753f435c32434",
  "user_id": "null",
  "inchikey_smiles": "IYDKWWDUBYWQGF-NNAZGLEUSA-N",
  "inchikey_inchi": "",
  "formula_smiles": "C48H72N10O12",
  "formula_inchi": "",
  "url": "https://gnps.ucsd.edu/ProteoSAFe/gnpslibraryspectrum.jsp?SpectrumID=CCMSLIB00000001547",
  "parent_mass": 980.5327235480092,
  "inchikey": "IYDKWWDUBYWQGF-NNAZGLEUSA-N",
  "annotation_history": [{"Compound_Name": "3-Des-Microcystein_LR", "Ion_Source": "LC-ESI", "Compound_Source": "Isolated", "Instrument": "Ger", "PI": "Gerwick", "Data_Collector": "Jenia", "Adduct": "M+H", "Scan": "-1", "Precursor_MZ": "981.4", "ExactMass": "0.0", "Charge": "0", "CAS_Number": "N/A", "Pubmed_ID": "N/A", "Smiles": "N/A", "INCHI": "N/A", "INCHI_AUX": "N/A", "Library_Class": "1", "SpectrumID": "CCMSLIB00000001547", "Ion_Mode": "Positive", "create_time": "2014-02-04 17:56:43.0", "task_id": "47daa4396adb426eaa5fa54b6ce7dd5f", "user_id": "mwang87"}, {"Compound_Name": "3-Des-Microcystein_LR", "Ion_Source": "LC-ESI", "Compound_Source": "Isolated", "Instrument": "qTof", "PI": "Gerwick", "Data_Collector": "Jenia", "Adduct": "M+H", "Scan": "-1", "Precursor_MZ": "981.54", "ExactMass": "0.0", "Charge": "0", "CAS_Number": "N/A", "Pubmed_ID": "N/A", "Smiles": "CC(C)CC1NC(=O)C(C)NC(=O)C(=C)N(C)C(=O)CCC(NC(=O)C(C)C(NC(=O)C(CCCNC(N)=N)NC(=O)C(C)C(NC1=O)C(O)=O)\\C=C\\C(\\C)=C\\C(C)C(O)Cc1ccccc1)C(O)=O", "INCHI": "N/A", "INCHI_AUX": "N/A", "Library_Class": "1", "SpectrumID": "CCMSLIB00000001547", "Ion_Mode": "Positive", "create_time": "2019-10-30 21:18:25.0", "task_id": "aa87bf9cd0784df9956753f435c32434", "user_id": "lfnothias"}],
  "peaks_json": [
      [
          289.286377,
          8068.0
      ],
      [
          295.545288,
          22507.0
      ],
      [
          298.489624,
          3925.0
      ],
      [
          317.3249511231221321465465,
          18742.0
      ]
  ]
}'::jsonb) as ("precursor_mz" float4, "spectrum_id" varchar, "peaks_json" pgms.spectrum);
EOF

psql -U postgres << 'EOF'
select * from pgms.load_from_json('
{
  "peaks_json": [
      [
          289.286377,
          8068.0
      ],
      [
          295.545288,
          22507.0
      ],
      [
          298.489624,
          3925.0
      ],
      [
          317.3249511231221321465465,
          18742.0
      ]
  ]
}'::jsonb) as ("precursor_mz" float4, "spectrum_id" varchar, "peaks_json" pgms.spectrum);
EOF