import asyncio
import websockets
import json

async def test_ws(route, msg=None):
    url = f"ws://127.0.0.1:18080{route}"
    print(f"\n--- Testing {url} ---")

    try:
        async with websockets.connect(url) as ws:
            #print("Connected!")
            if msg is not None:
                await ws.send(msg)
                #print(f"Sent: {msg}")

            try:
                reply = await asyncio.wait_for(ws.recv(), timeout = 2.0)
                #print(f"Received: {reply}")
                return reply
            except asyncio.TimeoutError:
                #print("No response")
                return -1
    except Exception as e:
        #print(f"Failed to connect: {e}")
        return -2
            
async def main():
    assert((await test_ws("/home")) == -1)

    assert((await test_ws("/home", "1")) == 'blakenator2')

    assert((await test_ws("/signup")) == -1)

    signupData = {}
    signupData['username'] = 'Testing'
    signupData['password'] = 'Testing'
    jsonSud = json.dumps(signupData)
    assert((await test_ws("/signup", jsonSud)) == '{"status":"error","message":"User already exists."}')

    assert((await test_ws("/login")) == -1)

    loginData = {}
    loginData['username'] = 'Testing'
    loginData['password'] = 'Testing'
    jsonLid = json.dumps(loginData)
    assert((await test_ws("/login", jsonLid)) == '{"name":"Testing","id":7,"status":"success"}')

    assert((await test_ws("/inventory")) == -1)

    invAddData = {}
    invAddData['op'] = 'additem'
    invAddData['uid'] = 1
    invAddData['name'] = 'Eggs'
    jsonIAD = json.dumps(invAddData)
    assert((await test_ws("/inventory", jsonIAD)) == -1)

    invListData = {}
    invListData['op'] = 'getlist'
    invListData['uid'] = 1
    jsonILD = json.dumps(invListData)
    assert(type(await test_ws("/inventory", jsonILD)) == str)

    invDelData = {}
    invDelData['op'] = 'delitem'
    invDelData['uid'] = 1
    invDelData['iid'] = 123
    jsonIDD = json.dumps(invDelData)
    assert(type(await test_ws("/inventory", jsonIDD)) == str)

    invRecData = {}
    invRecData['op'] = 'addReceipt'
    invRecData['uid'] = 1
    invRecData['ingredients'] = ['Eggs', 'Eggs']
    jsonIRD = json.dumps(invRecData)
    assert((await test_ws("/inventory", jsonIRD)) == -1)

    assert((await test_ws("/recipes")) == -1)

    recGetData = {}
    recGetData['op'] = "getrecipes"
    recGetData['uid'] = 1
    jsonRGD = json.dumps(recGetData)
    assert(type(await test_ws("/recipes", jsonRGD)) == str)

    insGetData = {}
    insGetData['op'] = "getInstructions"
    insGetData['name'] = 'Spotted Dick'
    jsonIGD = json.dumps(insGetData)
    assert(type(await test_ws("/recipes", jsonIGD)) == str)

    assert((await test_ws("/Account")) == -1)

    accSaveData = {}
    accSaveData['op'] = 'save'
    accSaveData['uid'] = 1
    accSaveData['saved'] = ['Pasta']
    jsonASD = json.dumps(accSaveData)
    assert((await test_ws("/Account", jsonASD)) == -1)

    accLoadData = {}
    accLoadData['op'] = 'load'
    accLoadData['uid'] = 1
    jsonALD = json.dumps(accLoadData)
    assert((await test_ws("/Account", jsonALD)) == '["Pasta"]')
    print("All tests passed.")
asyncio.run(main())
