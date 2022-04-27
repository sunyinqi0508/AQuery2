# AQuery++ DB

AQuery++ Database is an In-Memory Column-Store Database that incorporates compiled query execution.
Compiler frontend built on top of [mo-sql-parsing](https://github.com/klahnakoski/mo-sql-parsing).

## Roadmap
- [x] SQL Parser -> AQuery Parser (Front End)
- [ ] AQuery-C++ Compiler (Back End)
   -  [x] Schema and Data Model 
   -  [x] Data acquisition/output from/to csv file
   -  [x] Single table queries
      -  [x] Projections and Single Table Aggregations 
      -  [x] Group by Aggregations
      -  [x] Filters
      -  [x] Order by
      -  [x] Assumption
      -  [x] Flatten
   -  [ ] Multi-table 
      -  [ ] Join
   -  [ ] Subqueries 
- [ ] -> Optimizing Compiler

## TODO:
- [ ] C++ Meta-Programming: Elimilate template recursions as much as possible.
- [ ] IPC: Better ways to communicate between Interpreter (Python) and Executer (C++).
  - [ ] Sockets? stdin/stdout capture?
## Introduction

## Requirements

## Usage
