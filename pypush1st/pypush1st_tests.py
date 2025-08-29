import push1st
import time
import asyncio
import random
import sys

def event_channel(*args, **kwargs):
    print("Event");
    return

async def SendEventLoop(pid, start, stop):
    api = push1st.Api("app-test","127.0.0.1")
    ids = [*range(start, stop)]
    while 1:
        chname = random.choice(["user.","private-user."]) + str(random.choice(ids))
        res, msg = api.TriggerEvent("event",[chname],{"test":"data","value":6790})
        print('{:>10}::{:.<40} pass ({})'.format('API',chname,res))
        await asyncio.sleep(random.randint(2, 10) * 0.07)

async def SubscribeRandom(pid, start, stop, count):
    con = None
    con = push1st.Pusher("app-key","127.0.0.1","6001",False,"secret")
    ids = [*range(start, stop)]
    con.Connect();

    for i in range(count) :
        id = random.choice(ids)
        ids.remove(id)
        chname = random.choice(["user.","private-user."]) + str(id)
        print("Th {} subscribe to {}".format(pid,chname))
        con.Subscribe(chname,"event",event_channel)

    await asyncio.sleep(random.randint(2, 10) * 0.02)
    
    del con

async def AsyncJoin(Num):
    while 1:
        tasks = [asyncio.ensure_future(
            SubscribeRandom(i,1,8,4)) for i in range(Num)]
        await asyncio.wait(tasks)

def RunAsync():
    ioloop = asyncio.get_event_loop()
    ioloop.run_until_complete(AsyncJoin(4))
    ioloop.close()

def RunSingle():
    con = cpush1st.cpusher("app-key","127.0.0.1","6001",False,"secret")
    con.Connect();
    con.Subscribe("user.1","event",event_channel)
    con.Subscribe("private-user.1","event",event_channel)
    
    input("Press Enter to continue...")

def RunApi():
    ioloop = asyncio.get_event_loop()
    ioloop.run_until_complete(SendEventLoop(0,1,8))
    ioloop.close()

# python3 pypush1st_tests.py RunAsync | RunSingle | RunApi

if __name__ == '__main__':
    globals()[sys.argv[1]]()
    