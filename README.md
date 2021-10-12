# NavekSoft Push1ST
Push1ST is open source PUB/SUB multiple protocol message broker server ([Pusher](https://pusher.com/), MQTT, RAW WebSocket) 

## Key features
 - [x] Cross type channel messaging ( auto casting message between channel protocol format )
 - [x] Broadcast, Multicast, Unicast - message delivery, 
 - [x] Muti-applications support ( credentials for every app )
 - [x] Improved hooks  ( multiple http/https webhooks, lua hook,  for every app, triggered by register, unregister, join, leave or push into channel )
 - [x] Support multiple prtocols 
   - [x] Pusher
   - [x] Raw WebSocket ( with multiple channel subscription )
   - [ ] MQTT protocols (todo)
 - [x] Support Keep-Alive for API and WebHook
 - [x] Public, Private, Presence channels support
 - [x] Permanent or auto-closing channel 
 - [x] Cluster functionality ( presence user synchronization (todo), messages delivery )
 - [x] API Pusher format support with TCP or\and UNIX socket
 - [x] API access token, Pusher key generation, channels API
 - [x] Support WebSocket push messaging
 - [x] Websocket channel authorization by Bearer access token
 - [ ] IP whitelist for every enabled protocol
 - [x] Suitable for distributed one-to-many communications and distributed applications 
 - [x] TCP/IP as basic communication protocol
 - [x] ws/wss proto ( auto generate self-signed certificate if cer\key not specified )

## Installation guide

Install from APT ( Ubuntu, Debian ) repository [installation guide](/readme/installation.md).

Build from source code [build instruction](/readme/build.md).

## Run push1st server

После установки или сборки необходимо сконфигурировать ( [Configure push1st](/readme/configure.md) ) сервер. Push1ST по умолчанию устанаваливается в директорию /opt/naveksoft/push1st
В случае установки из репозитория сервер регистрируется как служба systemd ( служба не запускается автоматически, необходм ручной запуск. )

```bash
  sudo service push1st start
```
 
 Для запуска push1st из командной строки ( не как служба )

```bash
  /opt/naveksoft/push1st/push1st -c /opt/naveksoft/push1st/server.yml -V4
  
  # Для просмотра всех параметров коммандой строки
  /opt/naveksoft/push1st/push1st --help
  
```

## Usage and API 

Для подключения клиентов к каналам необходимо воспользоваться библиотеками:
- [Клиенты Pusher](https://pusher.com/docs/channels/channels_libraries/libraries/)
- Либой WebSocket client для RawWebSocket

ws://localhost:6001/pusher/app/`{{app-key}}`/ 

ws://localhost:6001/ws/app/`{{app-key}}`/`{channel-name-1}`/`{channel-name-2}`/[?`session=prefix`&`token=access-token`]


[Push1ST API](/readme/api.md)

- Key, Token generation methods
- Channels info
- Push events to channels

## Support

## Author 

## License

Copyright (c) 2021 Naveksoft.

This project is provided as is without any warranties. Use at your own risk.<br/>
By using Push1ST you agree with its [privacy](PRIVACY.md) policy and [license](LICENSE.md).
