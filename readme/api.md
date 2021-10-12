### Push1ST API

#### Создание токена для авторизации Pusher private, presence каналах

`POST` `{{api-server}}/{{app-id}}/token/session/`
```json
{
  "session": "1234",
  "channel": 
  "data": {}
}
```

`POST` | `{{api-server}}/{{app-id}}/token/session/`
------ | ------------------------------------------
Body   | {<br>"session": "1234", `# pusher session id`<br>"channel": "private-user.1", `# channel subscription name`<br>"data": "" `# custom data (optional)`<br>}
Response       | 200 OK `token-string`
       
Пример:
```bash
curl --location --request POST 'http://localhost:6002/apps/app-test/token/session/' \
--header 'Content-Type: application/json' \
--data-raw '{
    "session": "1234",
    "channel": "private-user.1",
    "data": "event data payload"
}'
```

#### Создание Bearer access token

Метод | Запрос | Тело запроса | Ответ 
----- | ------ | -------------| -----
POST | {{api-server}}/{{app-id}}/token/access/ | {<br>"channel": "\*", `# * for any channel`<br>"origin": "\*", `# * any origin`<br>"ttl": 0 `# seconds token time, 0 - infinite`<br>} | 200 OK `token-string`

Пример:
```bash
curl --location --request POST 'http://localhost:6002/apps/app-test/token/access/' \
--header 'Content-Type: application/json' \
--data-raw '{
   "channel": "*",
   "origin": "*",
   "ttl": 0
}'
```


#### Создание Bearer access token

Метод | Запрос | Тело запроса | Ответ 
----- | ------ | -------------| -----
POST | {{api-server}}/{{app-id}}/token/access/ | {<br>"channel": "\*", `# * for any channel`<br>"origin": "\*", `# * any origin`<br>"ttl": 0 `# seconds token time, 0 - infinite`<br>} | 200 OK `token-string`

Пример:
```bash
curl --location --request POST 'http://localhost:6002/apps/app-test/token/access/' \
--header 'Content-Type: application/json' \
--data-raw '{
   "channel": "*",
   "origin": "*",
   "ttl": 0
}'
```
