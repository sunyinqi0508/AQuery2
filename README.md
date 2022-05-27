# AQuery++ DB
## Introduction

AQuery++ Database is a cross-platform, In-Memory Column-Store Database that incorporates compiled query execution.
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

## Requirements
Recent version of Linux, Windows or MacOS, with recent C++ compiler that has C++17 (1z) support (e.g. gcc 6.0, MSVC 2017, clang 6.0), and python 3.6 or above.

## Usage
`python3 prompt.py` will launch the interactive command prompt. The server binary will be autometically rebuilt and started.
#### Commands:
- `<sql statement>`: parse sql statement
- `f <filename>`: parse all sql statements in file
- `print`: printout parsed sql statements
- `exec`: execute last parsed statement(s)
- `r`: run the last generated code snippet
- `save <OPTIONAL: filename>`: save current code snippet. will use random filename if not specified.
- `exit`: quit the prompt
#### Example:
   `f moving_avg.a` <br>
   `exec`
