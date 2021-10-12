### Push1ST API

#### Key, Token generation

Создание токена для авторизации Pusher private, presence каналах

Метод | Запрос | Тело запроса
----- | ------ | -------------
POST | {{api-server}}/{{app-id}}/token/session/ | ```json { "session": "1234", "channel": "private-user.1", "data": "event data payload" } ```
