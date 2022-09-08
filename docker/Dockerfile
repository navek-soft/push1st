FROM ubuntu:20.04
ARG DEBIAN_FRONTEND=noninteractive \
    NEXUS_USER \
    NEXUS_PASSWORD
RUN apt update && apt install -y wget gnupg2 curl nano
RUN apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9A4D3B9B041D12EF0D23694D8222A313EDE992FD && \    
    echo "deb [arch=amd64] https://nexus.naveksoft.com/repository/ubuntu-universe/ universe main" | tee /etc/apt/sources.list.d/aivp-push1st.list && \
    echo "machine nexus.naveksoft.com/repository login $NEXUS_LOGIN password $NEXUS_PASSWORD" | tee /etc/apt/auth.conf.d/nexus.naveksoft.com.conf
RUN apt update && apt install -y -f push1st || :
RUN unlink /etc/systemd/system/push1st.service 
CMD [ "/opt/naveksoft/push1st/push1st", "-c", "/opt/naveksoft/push1st/server.yml", "-V4", "-l", "/var/log" ]