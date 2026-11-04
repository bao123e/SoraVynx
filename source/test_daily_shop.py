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
    clicks,direct_requests,claimed,gift_open=0,0,false,false
    local ctrl
    local class={OnBtnClick_DailyGift=function(self)
      assert(self==ctrl);clicks=clicks+1;gift_open=true
    end}
    ctrl={class=class}
    local data={GetDailyShopReward=function(self) assert(self==PlayerData.Shop);return claimed end,
      SendDailyShopRewardReceiveReq=function() direct_requests=direct_requests+1;error('direct request called') end}
    PlayerData={Shop={class=data}}
    package.loaded['Game.UI.ShopEx.ShopCtrl']=class
    package.loaded['GameCore.Data.DataClass.PlayerShopData']=data
    local behaviour={isActiveAndEnabled=true,luaClassName='Game/UI/ShopEx/ShopCtrl.lua',luaInstance=ctrl}
    local objects={Length=1,[0]=behaviour}
    local lua_behaviour_type={};local system_type={}
    CS={LuaBehaviourBase=lua_behaviour_type,UnityEngine={Object={FindObjectsOfType=function(t)
      assert(t==system_type);return objects
    end}}}
    typeof=function(t) assert(t==lua_behaviour_type);return system_type end
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
    local text=run_shop();assert(clicks==1 and gift_open and direct_requests==0)
    assert(text:find('Status: UI_HANDLER_INVOKED',1,true))
    for i=1,5 do text=run_shop() end
    assert(clicks==1 and direct_requests==0 and text:find('Status: UI_STATUS',1,true))
    claimed=true;text=run_shop();assert(text:find('Shop reward flag changed',1,true))
    ''')
    case("behaviour.luaClassName='Game.UI.ShopEx.ShopCtrl'", 'run_shop();assert(clicks==1 and direct_requests==0)')
    case("behaviour.luaClassName='ShopCtrl';ctrl=setmetatable({}, {__index=class});behaviour.luaInstance=ctrl", 'run_shop();assert(clicks==1)')
    case('objects.Length=2;objects[1]=behaviour', 'run_shop();assert(clicks==1 and direct_requests==0)')
    for setup in ['debug=nil',
                  'class.OnBtnClick_DailyGift=function(self,unexpected) error("must not run") end',
                  'behaviour.isActiveAndEnabled=false',
                  "behaviour.luaClassName='Game.UI.Other.ShopCtrl'",
                  'behaviour.luaInstance={class={}}',
                  'objects.Length=5000',
                  'CS=nil',
                  "package.loaded['Game.UI.ShopEx.ShopCtrl']=nil",
                  'objects.Length=2;objects[1]={isActiveAndEnabled=true,luaClassName="ShopCtrl",luaInstance={class=class}}']:
        case(setup, 'local text=run_shop();assert(clicks==0 and direct_requests==0 and text:find("Status: STOPPED",1,true))')
    case('class.OnBtnClick_DailyGift=function(self) clicks=clicks+1;error("PRIVATE_ERROR") end', '''
    local text=run_shop();run_shop();assert(clicks==1 and direct_requests==0)
    assert(text:find('Status: UI_STATUS',1,true) and not text:find('PRIVATE_ERROR',1,true))
    ''')
    case('CS.UnityEngine.Object.FindObjectsOfType=function(t) error("PRIVATE_ERROR") end', '''
    local text=run_shop();assert(clicks==0 and direct_requests==0 and not text:find('PRIVATE_ERROR',1,true))
    ''')
    case('', '''
    run_shop();class.OnBtnClick_DailyGift=function(self) error('new handler must not run') end
    local text=run_shop();assert(clicks==1 and direct_requests==0 and text:find('context changed',1,true))
    ''')
    print('PASS Daily Gift UI: live active controller, exact class/handler, one invocation, normalised names, duplicate/ambiguity guards, no RPC fallback or error disclosure')


if __name__ == '__main__':
    run()
