### Push1ST API

[Postman collection](Push1ST-API.postman_collection.json)

<table>
  <caption><h6>Создание токена для авторизации Pusher private, presence каналах</h6></caption>
 <tr>
   <td><code>POST</code></td><td>{{api-server}}/{{app-id}}/token/session/</td><td><code>200</code> OK</td>
 </tr>
 <tr>
   <td valign=top><code>Body</code></td>
   <td>
     <pre>{
  "session": "1234",
  "channel": "channel name",
  "data": ""
}
</pre>
   </td>
   <td  valign=top><code>token-string</code></td>
  </tr>
  <tfoot>
    <tr>
       <td></td>
      <td colspan=2>
<pre>
  curl --location --request POST 'http://localhost:6002/apps/app-test/token/session/' \
    --header 'Content-Type: application/json' \
    --data-raw '{
      "session": "1234",
      "channel": "private-user.1",
      "data": ""
  }'</code>
      </td>
    </tr>
  <tfoot>
</table>


<table>
  <caption><h6>Создание Bearer access token</h6></caption>
 <tr>
   <td><code>POST</code></td><td>{{api-server}}/{{app-id}}/token/access/</td><td><code>200</code> OK</td>
 </tr>
 <tr>
   <td valign=top><code>Body</code></td>
   <td>
     <pre>{
  "origin": "*",
  "channel": "*",
  "ttl": 0
}
</pre>
   </td>
   <td  valign=top><code>token-string</code></td>
  </tr>
  <tfoot>
    <tr>
       <td></td>
      <td colspan=2>
<pre>
  curl --location --request POST 'http://localhost:6002/apps/app-test/token/access/' \
    --header 'Content-Type: application/json' \
    --data-raw '{
      "origin": "*",
      "channel": "*",
      "ttl": 0
  }'</code>
      </td>
    </tr>
  <tfoot>
</table>


  
<table>
  <caption><h6>Отправка сообщения в канал</h6></caption>
 <tr>
   <td><code>POST</code></td><td>{{api-server}}/{{app-id}}/events</td><td><code>200</code> OK</td>
 </tr>
 <tr>
   <td valign=top><code>Body</code></td>
   <td>
     <pre>{
  "name":"myevent",
  "channels":[
    "mychannel"
  ],
  "socket_id":"",
  "data": "event data payload"
}</pre>
   </td>
   <td  valign=top><pre>{
  "channels": {
    "channel2": 1
  },
  "msg-id": 1634029747778812625,
  "time": 171101
}</pre></td>
  </tr>
  <tfoot>
    <tr>
       <td></td>
      <td colspan=2>
<pre>curl --location --request POST 'http://localhost:6002/apps/app-test/events' \
  --header 'Content-Type: application/json' \
  --data-raw '{
    "name":"event_name",
    "channels":[
        "channel2"
    ],
    "socket_id":"",
    "data": "event data payload"
}'</code>
      </td>
    </tr>
  <tfoot>
</table>
