import logging
import json
import websocket
import pysher
import asyncio
import requests

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
        self.disconnect()

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

class cwebsocket:
    UrlBase = None
    UrlArgs = ""
    
    def __init__(self, appkey, host="", port=6001, secure=False, secret="", user_data=None, session_prefix=None, reconnect_interval=10):
        self.loop = asyncio.get_event_loop()
        self.UrlBase = '{}://{}:{}/ws/app/{}'.format("wss" if secure else "ws", host, port,appkey)
        if session_prefix != None:
            UrlArgs = '?session={}'.format(session_prefix)

    async def wsthread(Url, callback):
        Fd = websocket.WebSocketApp(Url,on_message=callback)
        Fd.run_forever()

    def Subscribe(self, channels, callback):
        self.loop.run_until_complete(cwebsocket.wsthread('{}/{}/{}'.format(self.UrlBase,"/".join(channels),self.UrlArgs if not self.UrlArgs else ''), callback))
    

class capi:
    UrlApi = None
    def __init__(self, appid, host="", port=6002, secure=False):
        self.UrlApi = '{}://{}:{}/apps/{}'.format("https" if secure else "http", host, port,appid)

    def GetAccessToken(self, ttl=0, channel="*", origin="*"):
        r = requests.post('{}/token/access/'.format(self.UrlApi), json={"channel": channel,"origin": origin,"ttl": ttl})
        return r.status_code, r.content
    
    def GetSessionToken(self, session, channel, data=""):
        r = requests.post('{}/token/session/'.format(self.UrlApi), json={"channel": channel,"session": session,"data": json.dumps(data)})
        return r.status_code, r.content

    def GetChannels(self):
        r = requests.get('{}/channels/'.format(self.UrlApi))
        return r.status_code, r.json()
    
    def GetChannel(self, channel):
        r = requests.get('{}/channels/{}/'.format(self.UrlApi,channel))
        return r.status_code, r.json()

    def TriggerEvent(self, event, channels, data, session=""):
        r = requests.post('{}/events/'.format(self.UrlApi), json={"name": event,"channels": channels,"socket_id": session,"data": data})
        return r.status_code, r.json()

