### Push1ST API

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
