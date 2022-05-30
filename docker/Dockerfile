FROM ubuntu:20.04
ARG DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -y wget gnupg2 curl
RUN wget https://reader:reader1@nexus.naveksoft.com/repository/gpg/naveksoft.gpg.key -O naveksoft.gpg.key && \
    apt-key add naveksoft.gpg.key && \
    echo "deb [arch=amd64] https://nexus.naveksoft.com/repository/ubuntu-universe/ universe main" | tee /etc/apt/sources.list.d/naveksoft-universe.list && \
    echo "machine nexus.naveksoft.com/repository login reader password reader1" | tee /etc/apt/auth.conf.d/nexus.naveksoft.com.conf
RUN apt update && apt install -y -f push1st || :
RUN unlink /etc/systemd/system/push1st.service 
CMD [ "/opt/naveksoft/push1st/push1st", "-c", "/opt/naveksoft/push1st/server.yml", "-V4", "-l", "/var/log" ]