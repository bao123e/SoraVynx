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
    requests,other_actions,ui_lookups=0,0,0
    local tower
    local class={ITDailyRewardReq=function(self) assert(self==tower);requests=requests+1 end,
      EnterInfinityTower=function() other_actions=other_actions+1;error('battle called') end}
    tower={class=class,InfinityTowerRewardsTab={daily={received=false,state=0}}}
    PlayerData={InfinityTower=tower,Shop={SendDailyShopRewardReceiveReq=function() other_actions=other_actions+1 end}}
    package.loaded['GameCore.Data.DataClass.PlayerInfinityTowerData']=class
    CS=setmetatable({}, {__index=function() ui_lookups=ui_lookups+1;error('UI lookup called') end})
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
    local text=run_tower();assert(requests==1 and other_actions==0 and ui_lookups==0)
    assert(text:find('TOWER_REQUEST_ATTEMPTED',1,true))
    for i=1,5 do text=run_tower() end
    assert(requests==1 and text:find('TOWER_STATUS',1,true))
    tower.InfinityTowerRewardsTab.daily.received=true;tower.InfinityTowerRewardsTab.daily.state=200987654
    text=run_tower();assert(requests==1 and text:find('field changes: 2',1,true))
    assert(not text:find('200987654',1,true) and other_actions==0 and ui_lookups==0)
    ''')
    for setup,code in [
      ('PlayerData.InfinityTower=nil','TOWER_INSTANCE_UNAVAILABLE'),
      ('class.ITDailyRewardReq=nil','TOWER_HANDLER_MISSING'),
      ('debug=nil','TOWER_METADATA_UNAVAILABLE'),
      ('class.ITDailyRewardReq=function(self,extra) end','TOWER_SIGNATURE_MISMATCH'),
      ('class.ITDailyRewardReq=function(target) end','TOWER_PARAMETER_MISMATCH'),
      ('tower.InfinityTowerRewardsTab=nil','TOWER_STATE_UNAVAILABLE'),
      ("package.loaded['GameCore.Data.DataClass.PlayerInfinityTowerData']={}",'TOWER_CLASS_UNVERIFIED')]:
        case(setup,'local t=run_tower();assert(requests==0 and other_actions==0 and ui_lookups==0 and t:find("'+code+'",1,true))')
    case('debug.getlocal=nil', 'run_tower();assert(requests==1)')
    case("package.loaded['GameCore/Data/DataClass/PlayerInfinityTowerData']=class;package.loaded['GameCore.Data.DataClass.PlayerInfinityTowerData']=nil", 'run_tower();assert(requests==1)')
    case('class.ITDailyRewardReq=function(self) requests=requests+1;error("PRIVATE_ERROR") end', '''
    local text=run_tower();run_tower();assert(requests==1 and not text:find('PRIVATE_ERROR',1,true))
    ''')
    case('', '''
    run_tower();PlayerData.InfinityTower={class=class,InfinityTowerRewardsTab={}}
    local text=run_tower();assert(requests==1 and text:find('TOWER_CONTEXT_CHANGED',1,true))
    ''')
    print('PASS Tower request: exact instance/signature, one attempt, status-only refresh, reward-state observations, no payload/ID disclosure, no UI lookup or Shop/battle calls')


if __name__ == '__main__':
    run()
