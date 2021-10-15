import unittest
import push1st
import time
import asyncio

Host = "127.0.0.1"
AppId = "app-test"
AppKey = "app-key"
Secret = "secret"

CGREY = '\x1B[90m'
CRED = '\x1B[91m'
CEND = '\x1B[0m'

class Test_api(unittest.TestCase):
    api = push1st.capi(AppId,Host)
    def test_GetAccessToken(self):
        res, token = self.api.GetAccessToken()
        self.assertTrue(res == 200 and len(token) != 0)
        print('{:>10}::{:.<40} pass ({})'.format('API','GetAccessToken',res))

    def test_GetSessionToken(self):
        res, token = self.api.GetSessionToken("session-id","channel-id")
        self.assertTrue(res == 200 and len(token) != 0)
        print('{:>10}::{:.<40} pass ({})'.format('API','GetSessionToken',res))

    def test_GetChannels(self):
        res, channels = self.api.GetChannels()
        self.assertTrue(res == 200)
        print('{:>10}::{:.<40} pass ({})'.format('API','GetChannels',res))

    def test_TriggerEvent(self):
        res, msg = self.api.TriggerEvent("myevent",["mychannel"],{"test":"data","value":6790})
        self.assertTrue(res == 200 and len(msg) != 0)
        print('{:>10}::{:.<40} pass ({})'.format('API','TriggerEvent',res))

class Test_Pusher(unittest.TestCase):
    loop = asyncio.get_event_loop()

    IsTriggered = False

    def fnTrigger(self,*args, **kwargs):
        self.IsTriggered = True

    async def apithread(channel, event, data):
        api = push1st.capi(AppId,Host)
        res, msg = api.TriggerEvent(event,[channel],data)

    def Trigger(self, channel, event, data):
        self.loop.run_until_complete(Test_Pusher.apithread(channel, event, data))

    def Subscribe(self, channel, event, user_data=None, secret=Secret):
        pus = push1st.cpusher(AppKey, Host, secret=secret, user_data=user_data)
        pus.Connect()
        self.IsTriggered = False
        IsConnected = False
        n = 5
        while n >= 0 and pus.IsConnected == False:
            n -= 1
            time.sleep(1)
        if pus.IsConnected:
            IsConnected = True
            print('{:>10}::{:.<40} pass ({})'.format('PUSHER','Subscribe',channel))
            pus.Subscribe(channel,event, self.fnTrigger)
            self.Trigger(channel,event,{"test":"data","value":6790})
            n = 5
            while n >= 0 and self.IsTriggered == False:
                n -= 1
                time.sleep(1)
            if self.IsTriggered:
                print('{:>10}::{:.<40} pass ({})'.format('PUSHER','Event',event), end='')
        pus.Disconnect()
        return IsConnected, self.IsTriggered

    def test_PublicChannel(self):
        IsSubscribe, IsReceive = self.Subscribe("mychannel","myevent")
        self.assertTrue(IsSubscribe and IsReceive)

    def test_PrivateChannel(self):
        IsSubscribe, IsReceive = self.Subscribe("private-mychannel","myevent")
        self.assertTrue(IsSubscribe and IsReceive)

    def test_PresenceChannel(self):
        IsSubscribe, IsReceive = self.Subscribe("presence-mychannel","myevent",{"user-id": 1})
        self.assertTrue(IsSubscribe and IsReceive)

    def test_ChannelBadSecret(self):
        IsSubscribe, IsReceive = self.Subscribe("private-mychannel.bad-secret","myevent", secret=Secret+"111")
        self.assertTrue(IsSubscribe == True and IsReceive == False)


if __name__ == '__main__':
    unittest.main()
