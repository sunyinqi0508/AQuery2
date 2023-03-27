create table source(id1 int,id2 int,id3 int,id4 int,id5 int,id6 int,v1 int,v2 int,v3 float)


LOAD DATA INFILE "data/h2o/G1_1e7_1e1_0_0_n.csv"
INTO TABLE source
FIELDS TERMINATED BY ","