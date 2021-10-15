### Push1ST API

[Postman collection](Push1ST-API.postman_collection.json)

#### Создание токена для авторизации Pusher private, presence каналах

`POST` `{{api-server}}/{{app-id}}/token/session/`
```json
{
  "session": "1234",
  "channel": "channel name",
  "data": ""
}
```

`Response` 200 OK `token-string`

Пример:
```bash
curl --location --request POST 'http://localhost:6002/apps/app-test/token/session/' \
--header 'Content-Type: application/json' \
--data-raw '{
    "session": "1234",
    "channel": "private-user.1",
    "data": ""
}'
```

#### Создание Bearer access token


`POST` `{{api-server}}/{{app-id}}/token/access/`
```json
{
  "origin": "*",
  "channel": "*",
  "ttl": 0
}
```

`Response` 200 OK `token-string`

Пример:
```bash
curl --location --request POST 'http://localhost:6002/apps/app-test/token/access/' \
--header 'Content-Type: application/json' \
--data-raw '{
    "origin": "*",
    "channel": "*",
    "ttl": 0
}'
```


#### Отправка сообщения в канал

`POST` `{{api-server}}/{{app-id}}/events/`
```json
{
    "name":"event_name",
    "channels":[
        "channel2"
    ],
    "socket_id":"",
    "data": "event data payload"
}
```

`Response` 200 OK 
```json
{
    "channels": {
        "channel2": 1
    },
    "msg-id": 1634029747778812625,
    "time": 171101
}
```

Пример:
```bash
curl --location --request POST 'http://localhost:6002/apps/app-test/events' \
--header 'Content-Type: application/json' \
--data-raw '{
    "name":"event_name",
    "channels":[
        "channel2"
    ],
    "socket_id":"",
    "data": "event data payload"
}'
```
