"""
def Pusher(appkey, host="", port="6001", secure=False, secret="", user_data=None, log_level=logging.NOTSET, reconnect_interval=10):
    return cpusher(appkey, host, port, secure, secret, user_data, log_level, reconnect_interval)
"""
from push1st.cpusher import cpusher as Pusher
from push1st.capi import capi as Api
