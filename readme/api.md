### Push1ST API

#### Key, Token generation

Создание токена для авторизации Pusher private, presence каналах

Метод | Запрос | Тело запроса
----- | ------ | -------------
POST | {{api-server}}/{{app-id}}/token/session/ | <br>{<br> "session": "1234",<br> "channel": "private-user.1",<br> "data": "event data payload"<br>}
