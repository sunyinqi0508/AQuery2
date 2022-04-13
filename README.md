# AQuery Compiler

AQuery Compiler that compiles AQuery into C++11.
Frontend built on top of [mo-sql-parsing](https://github.com/klahnakoski/mo-sql-parsing).

## Roadmap
- [x] SQL Parser -> AQuery Parser (Front End)
- [ ] AQuery-C++ Compiler (Back End)
   -  [x] Schema and Data Model 
   -  [x] Data acquisition/output from/to csv file
   -  [ ] Single table queries
      -  [x] Projections and Single Table Aggregations 
      -  [x] Group by Aggregations
      -  [x] Filters
      -  [ ] Order by
      -  [ ] Assumption
   -  [ ] Multi-table 
      -  [ ] Join
   -  [ ] Subqueries 
- [ ] -> Optimizing Compiler
