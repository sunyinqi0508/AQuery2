# Triggers Demo

This folder contains a demo workflow for the two types of triggers. 
- An interval-based trigger will be set up to execute a stored procedure `demoi` defined in [demo/putdata.cpp](/demo/putdata.cpp) that inserts a .csv file from `data/electricity` to the table `source` every 5 seconds. 
- A Conditional trigger will be triggered by condition `democq` defined in [demo/query.cpp](/demo/query.cpp) that checks and returns true when more than 200 rows of data are inserted into table `source`. Once triggered, it will execute a stored procedure `democa` defined in [demo/democa.cpp](/demo/action.cpp) that trains the incremental random forest by the new data. 
- See [demo/prep.a](/demo/prep.a) for parameters of the random forest. 

## Run the demo
### Preparation 
- Preprocess data
  - Put `electricity` dataset to `/data/electricity_orig`
  - Run `python3 rfdata_preproc.py` to generate .csv files to `data/electricity/`
- Use [demo/setup.sh](/demo/setup.sh) to 
  - setup stored procedures for this demo
  - compile random forest user module used in this demo
  - compile queries used in this demo 

### Running the demo
- Run AQuery prompt `python3 prompt.py` 
- Use Automated AQuery script in [demo/demo.aquery](/demo/demo.aquery) to execute the workflow. It does the following things in order:
  - Register user module, create a new random forest by running [`f demo/prep.a`](/demo/prep.a)
  - Register stored procedures. 
  - Create an Interval-based Trigger that executes payload `demoi` every 5 seconds
  - Create a Conditional Trigger that executes payload `democa` whenever condition `democq` returns a true. While condition `democq` is tested every time new data is inserted to table `source`.
  - Loads test data by running [demo/test.a](/demo/test.a)
- Use query `select predict(x) from test` to get predictions of the test data from current random forest.
  - In AQuery prompt, an extra `exec` command after the query is needed to execute the query.
- Use query `select test(x, y) from test` will also calculate l2 error.
