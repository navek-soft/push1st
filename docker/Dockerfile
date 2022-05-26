FROM ubuntu:20.04
ENV TZ=Europe/Minsk
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone
RUN apt update && apt install -y wget bzip2 gdb libssl1.1 lua5.3 liblua5.3-0 luarocks 
RUN wget "https://reader:reader1@nexus.naveksoft.com/repository/ubuntu-universe-bin/push1st-main-latest.tar.bz2"
RUN mkdir -p /opt/naveksoft/push1st/
RUN tar xvjf push1st-main-latest.tar.bz2 -C /opt/naveksoft/push1st/
# COPY ./server.yml /opt/naveksoft/push1st/server.yml

#EXPOSE 6001/tcp 6002/tcp 7001/tcp 8001/udp

CMD [ "/opt/naveksoft/push1st/push1st", "-c", "/opt/naveksoft/push1st/server.yml", "-V7", "-l", "/var/log" ]