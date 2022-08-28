FROM ubuntu:latest

RUN cp /bin/bash /bin/sh

RUN apt update && apt install -y wget

RUN export OS_VER=`cat /etc/os-release | grep VERSION_CODENAME` &&\
    export OS_VER=${OS_VER#*=} &&\
    printf "deb https://dev.monetdb.org/downloads/deb/ "${OS_VER}" monetdb \ndeb-src https://dev.monetdb.org/downloads/deb/ "${OS_VER}" monetdb\n">/etc/apt/sources.list.d/monetdb.list

RUN wget --output-document=/etc/apt/trusted.gpg.d/monetdb.gpg https://dev.monetdb.org/downloads/MonetDB-GPG-KEY.gpg

RUN apt update && apt install -y python3 python3-pip clang-14 libmonetdbe-dev git 

RUN git clone https://github.com/sunyinqi0508/AQuery2 

RUN python3 -m pip install -r AQuery2/requirements.txt

ENV IS_DOCKER_IMAGE=1 CXX=clang-14

CMD cd AQuery2 && python3 prompt.py



