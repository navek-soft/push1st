import logging
import json
import websocket

class Raw:
    Url = None
    Fd = None
    def __init__(self, url):
        self.Url = url
    
    def on_message(self, wsapp, message):
        print(message)

    def Connect(self):
        self.Fd = websocket.WebSocketApp(self.Url,on_message=self.on_message)
        self.Fd.run_forever()

