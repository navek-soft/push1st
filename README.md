# NavekSoft Push1ST
Push1ST is open source PUB/SUB multiple protocol message broker server (Pusher, MQTT, RAW websocket) 

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
 - [ ] IP whitelist for every enabled protocol (todo)
 - [x] Suitable for distributed one-to-many communications and distributed applications 
 - [x] TCP/IP as basic communication protocol
 - [x] ws/wss proto ( auto generate self-signed certificate if cer\key not specified )

## Installation guide

Install from APT ( Ubuntu, Debian ) repository installation [instruction guide](/readme/installation.md).

Build from source code [build instruction](/readme/build.md).

## Usage, run push1st server

После установки или сборки необходимо сконфигурировать ( Конфигурирование push1st ) сервер. Push1ST по умолчанию устанаваливается в директорию /opt/naveksoft/push1st
В случае установки из репозитория сервер запускается как служба systemd. ( После установки сервис не запускается автоматически. Необходим ручной запуск сервиса. )

```bash
  sudo service push1st start
 ```

## API documentation

## Support

## Author 

## License

