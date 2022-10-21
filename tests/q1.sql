CREATE TABLE testq1(a INT, b INT, c INT, d INT)

LOAD DATA INFILE "data/test.csv"
INTO TABLE testq1
FIELDS TERMINATED BY ","

SELECT sum(c), b, d
FROM testq1
group by a,b,d
order by d DESC, b ASC;

-- aaaa