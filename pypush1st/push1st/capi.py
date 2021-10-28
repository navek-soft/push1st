import json
import requests

class capi(object):
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

