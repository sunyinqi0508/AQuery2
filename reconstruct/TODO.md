# TODO:

## 1. double scans in projections 
   - first for special aggrigations and singular columns
   - Then in group by node decide if we have special group by aggregations
   - If sp_gb_agg exists, the entire groupby aggregation is done in C plugin
   - If not, group by is done in SQL

## 2. ColRef supports multiple objects
   - A.a = B.b then in projection A.a B.b will refer to same projection
   - Colref::ProjEq(ColRef v) => this == v or v in this.proj_eqs