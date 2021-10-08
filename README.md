# NavekSoft Push1st
PUB/SUB multiple protocol message broker server (Pusher, MQTT, RAW websocket) 

## Key features
 - Cross type channel messaging ( auto casting message between channel format )
 - Broadcast, Multicast, Unicast - message delivery, 
 - TTL for message
 - Muti-applications support ( credentials for every app )
 - Improved hooks  ( multiple http/https webhook with keeap-alive, lua hook,  for every app, triggered by register, unregister, join, leave or push into channel )
 - Support Pusher, Simple RawWebsocket ( with multiple channel subscription ), MQTT protocols
 - Support Keep-Alive for API and WebHook
 - Public, Private, Presence channels support
 - Permanent or auto-closing channel 
 - Cluster functionality ( presence user synchronization, messages delivery )
 - API Pusher format support with TCP or\and UNIX socket
 - Support WebSocket push messaging
 - IP whitelist for every enabled protocol
 - Neutral to message content 
 - Suitable for distributed one-to-many communications and distributed applications 
 - TCP/IP as basic communication protocol
 - ws/wss proto ( auto generate self-signed certificate if cer\key not specified )

## Installation guide from apt repository

###### Import repository key

```bash
	wget https://reader:reader1@nexus.naveksoft.com/repository/gpg/naveksoft.gpg.key -O naveksoft.gpg.key
	sudo apt-key add naveksoft.gpg.key
```

###### Add repository to source list and adjust auth
```bash
	echo "deb [arch=amd64] https://nexus.naveksoft.com/repository/ubuntu-universe/ universe main" | sudo tee /etc/apt/sources.list.d/naveksoft-universe.list
	echo "machine nexus.naveksoft.com/repository login reader password reader1" | sudo tee /etc/apt/auth.conf.d/nexus.naveksoft.com.conf
```


###### Check available versions
```bash
	sudo apt update
	apt list -a push1st
```

###### Install from repository

```bash
	sudo apt update
	sudo apt install push1st
```

## Install additional dependecies

###### Lua 5.3 dependecies ( optional )

```bash
	sudo luarocks install lua-cjson2
```



## Dependencies
libyaml-cpp0.6 - yaml file reader
liblua5.3-dev - lua 5.3 binding
openssl (libssl, libcrypto)
lua-cjson2
