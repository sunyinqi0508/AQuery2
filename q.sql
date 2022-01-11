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