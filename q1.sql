WITH
Target (Id , TradeDate , ClosePrice ) AS
( SELECT
Id , TradeDate , ClosePrice
FROM price
WHERE Id IN stock10 AND
TradeDate >= startYear10 AND
TradeDate <= startYear10 + 365 * 10),
weekly (Id , bucket , name , low , high , mean ) AS
( SELECT
Id ,
timeBucket ,
" weekly " ,
min ( ClosePrice ) ,
max ( ClosePrice ) ,
avg ( ClosePrice )
FROM Target
GROUP BY Id , getWeek ( TradeDate ) as
timeBucket ),
monthly ( Id , bucket , name , low , high , mean ) AS
( SELECT
Id ,
timeBucket ,
" monthly " ,
min ( ClosePrice ) ,
max ( ClosePrice ) ,
avg ( ClosePrice )
FROM Target
GROUP BY Id , getMonth ( TradeDate ) as
timeBucket ),
yearly (Id , bucket , name , low , high , mean ) AS
( SELECT
Id ,
timeBucket ,
" yearly " ,
min ( ClosePrice ) ,
max ( ClosePrice ) ,
avg ( ClosePrice )
FROM Target
GROUP BY Id , getYear ( TradeDate ) as
timeBucket )
SELECT
Id , bucket , name , low , high , mean
FROM
CONCATENATE ( weekly , monthly , yearly )
ASSUMING ASC Id , ASC name , ASC bucket
