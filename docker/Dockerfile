FROM ubuntu:22.04 AS builder
ARG DEBIAN_FRONTEND=noninteractive \
    BUILD_NUMBER \
    VERSION \
    COMMIT
COPY . /opt/push1st
WORKDIR /opt/push1st
RUN apt update && apt -y install git build-essential cmake clang-tidy-15 libyaml-cpp-dev libssl-dev lua5.3 liblua5.3-dev
RUN rm -rf build && mkdir build && cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug . && make -C build -j

FROM ubuntu:22.04
ARG DEBIAN_FRONTEND=noninteractive \
    BRAND
WORKDIR /opt/push1st
COPY --from=builder /opt/push1st/build/bin/push1st ./
COPY --from=builder /opt/push1st/config/server.example.yml ./
COPY --from=builder /opt/push1st/config/apps/ ./apps/
COPY --from=builder /opt/push1st/config/k8s/ ./config/k8s
RUN apt update && apt install -y libssl-dev lua5.3 liblua5.3-dev libyaml-cpp-dev
RUN ls -alh ./apps
CMD [ "/opt/push1st/push1st", "-c", "/opt/push1st/server.example.yml" ]
