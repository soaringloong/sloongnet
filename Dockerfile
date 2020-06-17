FROM gcc:9.3 AS build-env

RUN apt update && apt install -y \
    unzip cmake libsqlite3-dev libprotobuf-dev protobuf-compiler uuid-dev libssl-dev libjsoncpp-dev libmariadbclient-dev libluajit-5.1-dev

COPY . /tmp/
WORKDIR /tmp
# RUN echo $(ls -1 /tmp)
RUN /tmp/build/build.sh -r

#FROM debian:10-slim
FROM ubuntu:20.04

LABEL maintainer="admin@sloong.com"
ENV ADDRESS_INFO=0.0.0.0:8001
ENV NODE_TYPE=Worker

RUN apt update && apt install -y \
    libsqlite3-0 libprotobuf17 libuuid1 libssl1.1  libjsoncpp1 mariadb-client libluajit-5.1-2
WORKDIR /usr/local/bin
COPY --from=build-env /tmp/build/sloongnet-release /usr/local/bin
RUN cp /usr/local/bin/*.so /usr/lib/ && \
    chmod +x /usr/local/bin/sloongnet /usr/local/bin/run.sh

RUN mkdir /data
VOLUME /data
EXPOSE 8000
ENTRYPOINT ["/usr/local/bin/run.sh"]
