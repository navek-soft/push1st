import json
import pysher
import logging

class cpusher(pysher.Pusher, object):
    IsConnected = False
    ChannelsMap = []
    SessionId = None
    def __init__(self, appkey, host="", port="6001", secure=False, secret="", user_data=None, log_level=logging.NOTSET, reconnect_interval=10):
        super().__init__(appkey, secure = secure, custom_host=host, secret=secret, user_data = user_data, log_level = log_level, port = port, reconnect_interval=reconnect_interval)

    def Connect(self):
        self.connection.bind('pusher:connection_established',self.connect_handler)
        self.connect()
    
    def Disconnect(self):
        self.disconnect(1)

    def connect_handler(self, data):
        self.IsConnected = True
        obj = json.loads(data)
        self.SessionId = obj['socket_id']
        for x in self.ChannelsMap:
            channel = super().subscribe(x[0])
            channel.bind(x[1], x[2])
            
    def Subscribe(self, channel, event, callback):
        self.ChannelsMap.append([channel, event, callback])
        if self.IsConnected :
            channel = super().subscribe(channel)
            channel.bind(event, callback)
            return channel
        return None


