FROM ubuntu:20.04
RUN apt-get update
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y make && apt-get install -y g++
RUN apt-get update && apt-get install -y sudo
RUN apt-get install -y --no-install-recommends apt-utils
RUN apt-get install -y libpqxx-dev
RUN apt-get update && apt-get install -y libxml++2.6-dev
RUN apt-get update && apt-get install -y libxml++2.6-doc
RUN apt-get install -y libboost-all-dev
RUN apt-get update && apt-get install -y libicu-dev
RUN apt-get update && apt-get install -y libbz2-dev
RUN apt-get update && apt-get install -y aptitude
RUN apt-get install -y postgresql postgresql-contrib
RUN mkdir /var/log/erss
RUN mkdir /code
WORKDIR /code
ADD . /code/
RUN chmod o+x run.sh