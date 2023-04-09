SELECT id1, sum(v1) AS v1 FROM source GROUP BY id1; --0.036 |  0.017 | .274

SELECT id1, id2, sum(v1) AS v1 FROM source GROUP BY id1, id2; -- 0.063 | 0.013

SELECT id3, sum(v1) AS v1, avg(v3) AS v3 FROM source GROUP BY id3; -- 2.322 | 0.406 | 2.27

SELECT id4, avg(v1) AS v1, avg(v2) AS v2, avg(v3) AS v3 FROM source GROUP BY id4; -- 0.159 | 0.022

SELECT id6, sum(v1) AS v1, sum(v2) AS v2, sum(v3) AS v3 FROM source GROUP BY id6; -- 1.778 | 0.699 | 2.283

--faster median
--SELECT id4, id5, median(v3) AS median_v3, stddev(v3) AS sd_v3 FROM source GROUP BY id4, id5; -- x4

SELECT id3, max(v1) - min(v2) AS range_v1_v2 FROM source GROUP BY id3; -- 0.857 |  0.467 | 2.236

-- select top 2 from each grp
SELECT id6, subvec(v3,0,2) AS v3 FROM source GROUP BY id6 order by v3;

-- implement corr
SELECT id2, id4, pow(corr(v1, v2), 2) AS r2 FROM source GROUP BY id2, id4; 
-- NA | 0.240 | 0.6

SELECT id1, id2, id3, id4, id5, id6, sum(v3) AS v3, count(*) AS cnt FROM source GROUP BY id1, id2, id3, id4, id5, id6; -- 2.669 | 1.232 | 2.221(1.8)
