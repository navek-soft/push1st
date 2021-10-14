import sys
import unittest
import time
import push1st
import logging
import asyncio

# Add a logging handler so we can see the raw communication data
root = logging.getLogger()
root.setLevel(logging.DEBUG)
ch = logging.StreamHandler(sys.stdout)
root.addHandler(ch)

def  pusher_func(*args, **kwargs):
    print("processing Args:", args)
    print("processing Kwargs:", kwargs)

def  ws_func(*args, **kwargs):
    print("processing Args:", args)
    print("processing Kwargs:", kwargs)

#pus = push1st.cpusher("app-key","ubuntu-dev")
#pus.Connect()
#pus.Subscribe("mychannel","myevent",pusher_func)

#raw = push1st.cwebsocket("app-key","ubuntu-dev")
#raw.Subscribe(["mychannel","mychannel"],ws_func)

api = push1st.capi("app-test","ubuntu-dev")

for x in range(1, 100):
    res, token = res, msg = api.TriggerEvent("myevent",["mychannel"],{"test":"data","value":6790},x)
    print(res)


#res, token = api.GetAccessToken()
#print(res, token)

#res, token = api.GetSessionToken("session_id","private-user.1")
#print(res, token)
# 
# res, channels = api.GetChannels()
# print(res, channels)
# 
# res, channel = api.GetChannel("mychannel")
# print(res, channel)
# 
# res, msg = api.TriggerEvent("myevent",["mychannel"],{"test":"data","value":6790})
# print(res, msg)

#pusher = pysher.Pusher("app-key", custom_host="ubuntu-dev", port=6001, secure=False)

# We can't subscribe until we've connected, so we use a callback handler
# to subscribe when able
#def connect_handler(data):
    #channel = pusher.subscribe('mychannel')
    #channel.bind('myevent', my_func)

#pusher.connection.bind('pusher:connection_established', connect_handler)
#pusher.connect()

while True:
    # Do other things in the meantime here...
    time.sleep(1)
