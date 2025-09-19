### Connect to channels examples

#### Pusher

```python
import push1st # see /testsuite/push1st.py

def fnTrigger(*args, **kwargs):
  print(f' Args: {args}' )
  print(f' Kwargs: {kwargs}' )

pus = push1st.cpusher("app-key", "127.0.0.1", secret="secret")
pus.Connect()
pus.Subscribe("mychannel", "myevent", fnTrigger)
``` 

#### WebSocket

```python
import push1st # see /testsuite/push1st.py

def fnTrigger(*args, **kwargs):
  print(f' Args: {args}' )
  print(f' Kwargs: {kwargs}' )

pus = push1st.cwebsocket("app-key", "127.0.0.1", secret="secret")
pus.Subscribe(["mychannel"], fnTrigger)
``` 

#### API exampale ( trigger event )

```python
import push1st # see /testsuite/push1st.py

api = push1st.capi("app-test","127.0.0.1")
code, response = api.TriggerEvent("myevent",["mychannel"],{"test":"data","value":6790})
``` 
