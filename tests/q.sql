FUNCTION
execStrategy ( alloc , mavgday , mavgmonth , px ) {
buySignal := mavgday > mavgmonth ;
f := a + b ;
alloc * prd (
CASE maxs ( buySignal )
WHEN TRUE THEN
CASE buySignal
WHEN TRUE THEN 1 / px
ELSE px
END
ELSE 1
END )
}
FUNCTION covariance (x , y ) {
xmean := avg (x) ;
ymean := avg (y) ;
avg (( x - xmean ) * (y - ymean ))
}
FUNCTION sd ( x) {
sqrt ( covariance (x , x) )
}
FUNCTION pairCorr (x , y ) {
covariance (x , y ) / ( sd (x) * sd (y ))
}
<k>
`
p:5
q:2
phi:(p+1)?1.
theta:q?1.
"p q phi theta"
p
q
phi
theta
l:()
e:()

`
L1:10?20
Le1:10?2.
L2:3?20
Le2:3?2.
"L1 Le1 L2 Le2"
L1
Le1
L2
Le2

`
"Add L1, then predict"
l:l,L1
e:e,Le1
predict:(phi(0)) + (sum ({[x](phi(x+1)) * (l(((#l)-1)-x))}[!p])) - (sum ({[x](theta(x)) * (e(((#e)-1)-x))}[!q]))
predict

`
"Add L2, then predict"
l:l,L2
e:e,Le2
predict:(phi(0)) + (sum ({[x](phi(x+1)) * (l(((#l)-1)-x))}[!p])) - (sum ({[x](theta(x)) * (e(((#e)-1)-x))}[!q]))
predict

 </k>

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
