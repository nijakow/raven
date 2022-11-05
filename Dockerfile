FROM debian:latest

RUN apt-get update
RUN apt-get install -y make gcc

COPY ./src ./src

RUN cd src && make install

RUN rm -rf ./src

ENTRYPOINT [ "raven.exe" ]
