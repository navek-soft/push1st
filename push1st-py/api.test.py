import unittest
import push1st

Host = "localhost"
AppId = "app-test"

class Test_api(unittest.TestCase):
    api = push1st.capi(AppId,Host)
    def test_GetAccessToken(self):
        res, token = self.api.GetAccessToken()
        self.assertEqual(res,200)
        self.assertTrue(len(token))

    def test_GetSessionToken(self):
        res, token = self.api.GetSessionToken("session-id","channel-id")
        self.assertEqual(res,200)
        self.assertTrue(len(token))

    def test_GetAccessTokenLoad(self):
        for x in range(1, 100):
            res, token = self.api.GetAccessToken()
            self.assertEqual(res,200)
            self.assertTrue(len(token))
        
if __name__ == '__main__':
    unittest.main()
