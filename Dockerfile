FROM ubuntu:24.04

WORKDIR /AQuery2

RUN apt update && apt install -y wget

RUN export OS_VER=`cat /etc/os-release | grep VERSION_CODENAME` &&\
    export OS_VER=${OS_VER#*=} &&\
    printf "deb https://dev.monetdb.org/downloads/deb/ "${OS_VER}" monetdb \ndeb-src https://dev.monetdb.org/downloads/deb/ "${OS_VER}" monetdb\n">/etc/apt/sources.list.d/monetdb.list

RUN wget --output-document=/etc/apt/trusted.gpg.d/monetdb.gpg https://dev.monetdb.org/downloads/MonetDB-GPG-KEY.gpg

RUN apt update && apt install -y python3 python3-pip clang-18 libmonetdbe-dev libmonetdb-client-dev monetdb5-sql-dev git monetdb5-sql monetdb-client libssl-dev libomp-dev

COPY . . 

RUN python3 -m pip install -r requirements.txt --break-system-packages
RUN python3 duckdb_install.py

ENV IS_DOCKER_IMAGE=1 CXX=clang++-18

# First run will build cache into image
RUN python3 prompt.py 

# CMD cd AQuery2 && python3 prompt.py
CMD echo "Welcome. Type python3 prompt.py to start AQuery." && bash

