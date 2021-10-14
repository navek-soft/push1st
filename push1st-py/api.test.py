import unittest
import push1st

Host = "localhost"
AppId = "app-test"

CGREY = '\x1B[90m'
CRED = '\x1B[91m'
CEND = '\x1B[0m'

class Test_api(unittest.TestCase):
    api = push1st.capi(AppId,Host)
    def test_GetAccessToken(self):
        res, token = self.api.GetAccessToken()
        self.assertTrue(res == 200 and len(token) != 0)
        print('{:>10}::{:.<30} pass ({})'.format('API','GetAccessToken',res))

    def test_GetSessionToken(self):
        res, token = self.api.GetSessionToken("session-id","channel-id")
        self.assertTrue(res == 200 and len(token) != 0)
        print('{:>10}::{:.<30} pass ({})'.format('API','GetSessionToken',res))

        
if __name__ == '__main__':
    unittest.main()
