CREATE TABLE test(a INT, b INT, c INT, d INT)

LOAD DATA INFILE "test.csv"
INTO TABLE test
FIELDS TERMINATED BY ","

SELECT sum(c), b, d
FROM test
group by a,b,d
-- order by d DESC, b ASC
