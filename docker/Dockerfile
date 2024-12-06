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
WORKDIR /opt/push1st
COPY --from=builder /opt/push1st-server/build/push1st ./
RUN apt update && apt install -y libssl1.1 lua5.3 liblua5.3-0
COPY app.yml.sample ./apps/
RUN ls -alh /opt/push1st/apps
COPY server.yml.sample ./server.yml
CMD [ "/opt/push1st/push1st", "-c", "/opt/push1st/server.yml", "-V7" ]
