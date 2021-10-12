### Push1ST API

#### Key, Token generation

Создание токена для авторизации Pusher private, presence каналах

Метод | Запрос | Тело запроса | Ответ 
----- | ------ | -------------| -----
POST | {{api-server}}/{{app-id}}/token/session/ | {<br>"session": "1234" # pusher session id,<br>"channel": "private-user.1" # channel subscription name,<br>"data": "" # optional custom presence channel data<br>} | <string> # Pusher channel authorization token

Пример:
