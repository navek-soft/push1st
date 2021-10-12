### Push1ST API

#### Key, Token generation

Создание токена для авторизации Pusher private, presence каналах

Метод | Запрос | Тело запроса | Ответ 
----- | ------ | -------------| -----
POST | {{api-server}}/{{app-id}}/token/session/ | {<br>"session": "1234", `# pusher session id`<br>"channel": "private-user.1", `# channel subscription name`<br>"data": "" `# custom data (optional)`<br>} | 200 OK `token-string`

Пример:
```curl
curl --location --request POST 'http://localhost:6002/apps/app-test/token/session/' \
--header 'Content-Type: application/json' \
--data-raw '{
    "session": "1234",
    "channel": "private-user.1",
    "data": "event data payload"
}'
```
