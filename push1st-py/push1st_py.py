import sys
import unittest
import time
import pysher
import logging
import json
import cpush1st

# Add a logging handler so we can see the raw communication data
import logging
root = logging.getLogger()
root.setLevel(logging.DEBUG)
ch = logging.StreamHandler(sys.stdout)
root.addHandler(ch)


def  my_func(*args, **kwargs):
    print("processing Args:", args)
    print("processing Kwargs:", kwargs)

pus = cpush1st.Pusher("app-key","ubuntu-dev")
pus.Connect()
pus.Subscribe("mychannel","myevent",my_func)

raw = cpush1st.Raw("ws://ubuntu-dev:6001/raw/app/app-key/mychannel/mychannel2/")
raw.Connect()



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
