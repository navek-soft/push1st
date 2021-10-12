# NavekSoft Push1st
PUB/SUB multiple protocol message broker (Pusher, MQTT, RAW websocket) 

## Key features
 - Cross type channel messaging ( auto casting message between channel protocol format )
 - Broadcast, Multicast, Unicast - message delivery, 
 - Muti-applications support ( credentials for every app )
 - Improved hooks  ( multiple http/https webhooks, lua hook,  for every app, triggered by register, unregister, join, leave or push into channel )
 - Support Pusher, Simple RawWebsocket ( with multiple channel subscription ), MQTT protocols (todo)
 - Support Keep-Alive for API and WebHook
 - Public, Private, Presence channels support
 - Permanent or auto-closing channel 
 - Cluster functionality ( presence user synchronization (todo), messages delivery )
 - API Pusher format support with TCP or\and UNIX socket
 - API access token, Pusher key generation, channels API
 - Support WebSocket push messaging
 - Websocket channel authorization by Bearer access token
 - IP whitelist for every enabled protocol (todo)
 - Neutral to message content 
 - Suitable for distributed one-to-many communications and distributed applications 
 - TCP/IP as basic communication protocol
 - ws/wss proto ( auto generate self-signed certificate if cer\key not specified )

## Installation guide

Install from APT ( Ubuntu, Debian ) repository installation [instruction guide](/readme/installation.md).

Build from source code [build instruction](/readme/build.md).


## Dependencies
libyaml-cpp0.6 - yaml file reader
liblua5.3-dev - lua 5.3 binding
openssl (libssl, libcrypto)
lua-cjson2
