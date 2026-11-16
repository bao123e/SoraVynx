"""Exercise the fixed Tower daily reward script in a real Lua VM; never connects to a game."""
import ctypes as c
import ctypes.util
from pathlib import Path


def run():
    library = ctypes.util.find_library('lua5.4') or ctypes.util.find_library('lua5.3')
    if not library:
        raise SystemExit('Install liblua5.4 or liblua5.3 to run Tower tests.')
    lua = c.CDLL(library)
    lua.luaL_newstate.restype = c.c_void_p
    lua.luaL_openlibs.argtypes = [c.c_void_p]
    lua.luaL_loadstring.argtypes = [c.c_void_p, c.c_char_p]
    lua.lua_pcallk.argtypes = [c.c_void_p, c.c_int, c.c_int, c.c_int, c.c_ssize_t, c.c_void_p]
    lua.lua_tolstring.argtypes = [c.c_void_p, c.c_int, c.POINTER(c.c_size_t)]
    lua.lua_tolstring.restype = c.c_void_p
    lua.lua_close.argtypes = [c.c_void_p]
    script = (Path(__file__).parent / 'assets/daily_tower.lua').read_text()
    fixture = r'''
    requests,other_actions=0,0
    local target='Game.UI.Dorm.DormInteractive.DormInteractiveCtrl'
    local class={OnBtnClick_ReceiveDormReward=function(self) assert(self==instance);requests=requests+1 end}
    local go={activeInHierarchy=true,scene={isLoaded=true}}
    local component=io.tmpfile();local previous=getmetatable(component);local mt={}
    for k,v in next,previous do mt[k]=v end
    mt.__index=function(u,k) if k=='gameObject' then return go end;return previous.__index[k] end
    debug.setmetatable(component,mt)
    instance={class=class,view=component}
    local live_registry={[27]=instance}
    debug.getregistry=function() return live_registry end
    package.loaded[target]=class
    PlayerData={InfinityTower={class={ITDailyRewardReq=function() other_actions=other_actions+1 end}},
      Shop={SendDailyShopRewardReceiveReq=function() other_actions=other_actions+1 end}}
    '''

    def case(setup, checks):
        state = lua.luaL_newstate()
        assert state
        try:
            lua.luaL_openlibs(state)
            source = fixture + setup + '\nlocal function run_tower()\n' + script + '\nend\n' + checks + '\nreturn "passed"'
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
    local text=run_tower();assert(requests==1 and other_actions==0)
    assert(text:find('TOWER_REQUEST_ATTEMPTED',1,true))
    for i=1,5 do text=run_tower() end
    assert(requests==1 and text:find('TOWER_STATUS',1,true))
    assert(not text:find('confirmed success',1,true))
    ''')
    for setup,code in [
      ('package.loaded[target]=nil','DORM_MODULE_NOT_LOADED'),
      ('class.OnBtnClick_ReceiveDormReward=nil','DORM_ACTIVE_UI_NOT_FOUND'),
      ('debug.getinfo=nil','DORM_ACTIVE_UI_NOT_FOUND'),
      ('class.OnBtnClick_ReceiveDormReward=function(self,extra) end','DORM_ACTIVE_UI_NOT_FOUND'),
      ('class.OnBtnClick_ReceiveDormReward=function(target) end','DORM_ACTIVE_UI_NOT_FOUND'),
      ('go.activeInHierarchy=false','DORM_ACTIVE_UI_NOT_FOUND'),
      ('go.scene.isLoaded=false','DORM_ACTIVE_UI_NOT_FOUND'),
      ('instance.view=nil','DORM_ACTIVE_UI_NOT_FOUND'),
      ('instance.view={gameObject=go}','DORM_ACTIVE_UI_NOT_FOUND'),
      ('live_registry={[1]=class}','DORM_ACTIVE_UI_NOT_FOUND'),
      ('live_registry[28]={class=class,view=component}','DORM_ACTIVE_UI_AMBIGUOUS')]:
        case(setup,'local t=run_tower();assert(requests==0 and other_actions==0 and t:find("'+code+'",1,true))')
    case('debug.getlocal=nil', 'run_tower();assert(requests==1)')
    case("package.loaded['Game/UI/Dorm/DormInteractive/DormInteractiveCtrl.lua']=class;package.loaded[target]=nil", 'run_tower();assert(requests==1)')
    case('class.OnBtnClick_ReceiveDormReward=function(self) requests=requests+1;error("PRIVATE_ERROR") end', '''
    local text=run_tower();run_tower();assert(requests==1 and not text:find('PRIVATE_ERROR',1,true))
    ''')
    case('''
    local capture={[91]=instance};debug.getregistry=nil
    PanelManager={GetCurPanelId=function() return capture end}
    ''', 'run_tower();assert(requests==1 and other_actions==0)')
    case('''
    live_registry[28]=live_registry;instance.__index=nil
    live_registry[29]=setmetatable({}, {__index=function() other_actions=other_actions+1;error('metamethod called') end})
    ''', 'run_tower();assert(requests==1 and other_actions==0)')
    case('''
    for i=1,2050 do live_registry[i+100]={} end
    ''', 'local t=run_tower();assert(requests==0 and other_actions==0 and t:find("DORM_DISCOVERY_LIMIT",1,true))')
    print('PASS Dorm Claim Gifts: exact live UI class/signature, active scene, ambiguity/limit rejection, one attempt, unchanged Player/InfinityTower/Shop, no scalar/error disclosure')


if __name__ == '__main__':
    run()
