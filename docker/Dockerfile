FROM ubuntu:20.04 AS builder
ARG DEBIAN_FRONTEND=noninteractive \
    BUILD_NUMBER \
    VERSION
WORKDIR /opt
RUN apt update && apt -y install git build-essential libyaml-cpp-dev libssl-dev lua5.3 liblua5.3-dev
COPY push1st .
RUN cd push1st-server && make build=${BUILD_NUMBER} ver=${VERSION} bname=src version branch
RUN mv ./push1st-server/build/push1st-${VERSION}-src ./push1st-server/build/push1st

FROM ubuntu:20.04
ARG DEBIAN_FRONTEND=noninteractive \
    BRAND
WORKDIR /opt/${BRAND}/push1st
COPY --from=builder /opt/push1st-server/build/push1st ./
COPY app.example.yml ./apps/
COPY server.example.yml .
COPY server.example.yml ./server.yml
CMD [ "/opt/${BRAND}/push1st/push1st", "-c", "/opt/${BRAND}/push1st/server.yml", "-V7", "-l", "/var/log" ]