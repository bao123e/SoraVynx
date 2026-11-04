"""Exercise the fixed Shop request script in a real Lua VM; never connects to a game."""
import ctypes as c
import ctypes.util
from pathlib import Path


def run():
    library = ctypes.util.find_library('lua5.4') or ctypes.util.find_library('lua5.3')
    if not library:
        raise SystemExit('Install liblua5.4 or liblua5.3 to run Shop tests.')
    lua = c.CDLL(library)
    lua.luaL_newstate.restype = c.c_void_p
    lua.luaL_openlibs.argtypes = [c.c_void_p]
    lua.luaL_loadstring.argtypes = [c.c_void_p, c.c_char_p]
    lua.lua_pcallk.argtypes = [c.c_void_p, c.c_int, c.c_int, c.c_int, c.c_ssize_t, c.c_void_p]
    lua.lua_tolstring.argtypes = [c.c_void_p, c.c_int, c.POINTER(c.c_size_t)]
    lua.lua_tolstring.restype = c.c_void_p
    lua.lua_close.argtypes = [c.c_void_p]
    script = (Path(__file__).parent / 'assets/daily_shop.lua').read_text()
    fixture = r'''
    requests,claimed,pending=0,false,nil
    local shop
    local class={
      SendDailyShopRewardReceiveReq=function(self,callback)
        assert(self==shop and type(callback)=='function');requests=requests+1;pending=callback
      end,
      GetDailyShopReward=function(self) assert(self==shop);return claimed end,
      SendResidentShopPurchaseReq=function() error('purchase called') end,
      Upgrade=function() error('upgrade called') end
    }
    shop={class=class};PlayerData={Shop=shop}
    package.loaded['GameCore.Data.DataClass.PlayerShopData']=class
    '''

    def case(setup, checks):
        state = lua.luaL_newstate()
        assert state
        try:
            lua.luaL_openlibs(state)
            source = fixture + setup + '\nlocal function run_shop()\n' + script + '\nend\n' + checks + '\nreturn "passed"'
            status = lua.luaL_loadstring(state, source.encode())
            if not status:
                status = lua.lua_pcallk(state, 0, 1, 0, 0, None)
            size = c.c_size_t()
            value = lua.lua_tolstring(state, -1, c.byref(size))
            text = c.string_at(value, size.value).decode() if value else ''
            assert status == 0, text
            assert text == 'passed'
        finally:
            lua.lua_close(state)

    case('', '''
    local first=run_shop();assert(requests==1 and first:find('Waiting for its callback',1,true))
    local second=run_shop();assert(requests==1 and second:find('Waiting for its callback',1,true))
    claimed=true;pending({token='PRIVATE_TOKEN'},'PRIVATE_MESSAGE',false,17)
    local done=run_shop();assert(requests==1 and done:find('Callback received: 1',1,true))
    assert(done:find('Shop reward flag changed',1,true) and done:find('Server success has not been decoded',1,true))
    assert(not done:find('PRIVATE_',1,true) and not done:find('17',1,true))
    for i=1,4 do run_shop() end;assert(requests==1)
    ''')
    case('', '''
    run_shop();pending(false,{error='PRIVATE_ERROR'})
    local text=run_shop();assert(requests==1 and text:find('No verified reward-flag change',1,true))
    assert(not text:find('PRIVATE_ERROR',1,true))
    ''')
    case('class.SendDailyShopRewardReceiveReq=function(self,callback) requests=requests+1;claimed=true;callback() end', '''
    local text=run_shop();assert(requests==1 and text:find('Callback received: 1',1,true))
    assert(text:find('kinds: none',1,true));run_shop();assert(requests==1)
    ''')
    case('class.SendDailyShopRewardReceiveReq=function(self,callback) requests=requests+1;error("PRIVATE_ERROR") end', '''
    local text=run_shop();assert(requests==1 and text:find('may already have been sent',1,true))
    run_shop();assert(requests==1 and not text:find('PRIVATE_ERROR',1,true))
    ''')
    case('class.SendDailyShopRewardReceiveReq=function(self,callback) error("PRIVATE_ERROR") end', '''
    run_shop();run_shop();assert(requests==0 and _SORAVYNX_SHOP_CLAIM_V1.phase=='handler_error')
    ''')
    for setup in ['debug=nil',
                  'class.SendDailyShopRewardReceiveReq=function(self,goodsId,callback) error("must not run") end',
                  'class.GetDailyShopReward=function(self) return "PRIVATE_VALUE" end',
                  'PlayerData.Shop={class={}}',
                  "package.loaded['GameCore.Data.DataClass.PlayerShopData']=nil"]:
        case(setup, 'local text=run_shop();assert(requests==0 and text:find("Stopped:",1,true));assert(not text:find("PRIVATE_VALUE",1,true))')
    case('', '''
    run_shop();PlayerData.Shop={class=class};local text=run_shop()
    assert(requests==1 and text:find('context changed',1,true))
    ''')
    print('PASS Shop preview: exact instance/signature, one request, async/sync callback, status refresh, no retries after errors, context guard and no purchases/payload disclosure')


if __name__ == '__main__':
    run()
