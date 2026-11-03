"""Exercise the bundled inspection script with a real Lua VM and hostile fixtures."""
import ctypes as c
import ctypes.util
from pathlib import Path


def run():
    library = ctypes.util.find_library('lua5.4') or ctypes.util.find_library('lua5.3')
    if not library:
        raise SystemExit('Install liblua5.4 or liblua5.3 to run Lua inventory tests.')
    lua = c.CDLL(library)
    lua.luaL_newstate.restype = c.c_void_p
    lua.luaL_openlibs.argtypes = [c.c_void_p]
    lua.luaL_loadstring.argtypes = [c.c_void_p, c.c_char_p]
    lua.lua_pcallk.argtypes = [c.c_void_p, c.c_int, c.c_int, c.c_int, c.c_ssize_t, c.c_void_p]
    lua.lua_tolstring.argtypes = [c.c_void_p, c.c_int, c.POINTER(c.c_size_t)]
    lua.lua_tolstring.restype = c.c_void_p
    lua.lua_close.argtypes = [c.c_void_p]
    script = (Path(__file__).parent / 'assets/daily_inventory.lua').read_text()

    def inspect(fixture):
        state = lua.luaL_newstate()
        assert state
        try:
            lua.luaL_openlibs(state)
            source = fixture + '\nlocal report=(function()\n' + script + '\nend)()\nassert(action_calls == 0)\nreturn report'
            status = lua.luaL_loadstring(state, source.encode())
            if not status:
                status = lua.lua_pcallk(state, 0, 1, 0, 0, None)
            size = c.c_size_t()
            value = lua.lua_tolstring(state, -1, c.byref(size))
            result = c.string_at(value, size.value).decode() if value else ''
            assert status == 0, result
            return result
        finally:
            lua.lua_close(state)

    base = r'''
    action_calls=0
    local function forbidden() action_calls=action_calls+1; error('game action called') end
    local quest={ReceiveDailyReward=function(self,questIds) forbidden() end,
      AccountToken='PRIVATE_TOKEN', UserName='PRIVATE_NAME', [123456789]='PRIVATE_ID',
      ['secret@example.com']=forbidden, super={}}
    for i=1,2500 do quest.super['Inherited_'..i]=forbidden end
    quest.__index=quest
    setmetatable(quest,{__index=forbidden,__pairs=forbidden,__tostring=forbidden})
    package.loaded['GameCore.Data.DataClass.PlayerQuestData']=quest
    package.loaded['GameCore.Data.DataClass.DispatchData']={ReqApplyAgent=function(self,agentId,charIds) forbidden() end}
    package.loaded['GameCore.Data.DataClass.PlayerDiscData']={SendDiscStrengthenReq=function(self,discId,materials) forbidden() end}
    local shop={SendDailyShopRewardReceiveReq=function(self) forbidden() end}
    shop.__index=shop
    package.loaded['GameCore.Data.DataClass.PlayerShopData']=shop
    package.loaded['Game.UI.Heartlink.GiftCtrl']={SendGift=function(self,giftId,count) forbidden() end}
    PlayerData={Shop=setmetatable({},shop),Quest=setmetatable({}, {__index=quest}),UserName='PRIVATE_NAME'}
    PanelManager={OpenPanel=forbidden}
    '''
    report = inspect(base)
    for required in ['Lua report v5', 'MODULE GameCore.Data.DataClass.PlayerQuestData table',
                     'ReceiveDailyReward [nparams=2; vararg=no; params=self,questIds]',
                     'ReqApplyAgent [nparams=3; vararg=no; params=self,agentId,charIds]',
                     'SendDiscStrengthenReq [nparams=3; vararg=no; params=self,discId,materials]',
                     'SendDailyShopRewardReceiveReq [nparams=1; vararg=no; params=self]',
                     'SendGift [nparams=3; vararg=no; params=self,giftId,count]',
                     'OBJECT global/PlayerData.Shop class=GameCore.Data.DataClass.PlayerShopData',
                     'OBJECT global/PlayerData.Quest class=GameCore.Data.DataClass.PlayerQuestData',
                     'NOT_LOADED GameCore.Data.DataClass.PlayerCharData', 'Limited=no']:
        assert required in report, required
    for secret in ['PRIVATE_TOKEN', 'PRIVATE_NAME', 'PRIVATE_ID', '123456789',
                   'secret@example.com', 'AccountToken', 'UserName', 'Inherited_', '.super.', '.__index']:
        assert secret not in report, secret
    reordered = inspect(base.replace("Shop=setmetatable({},shop),Quest=setmetatable({}, {__index=quest})",
                                    "Quest=setmetatable({}, {__index=quest}),Shop=setmetatable({},shop)"))
    assert report == reordered
    body = [line for line in report.splitlines() if line.startswith(('MODULE ', 'TARGET ', 'FUNCTION ', 'OBJECT ', 'FIELD ', 'NOT_LOADED Game', 'NOT_PRESENT global/', 'DETAIL_COVERAGE '))]
    assert body == sorted(body)
    limited = inspect(base + '''
    package.loaded['GameCore.Data.DataClass.PlayerCharData']={}
    for i=1,40000 do package.loaded['GameCore.Data.DataClass.PlayerCharData']['Action_'..i]=function() action_calls=action_calls+1 end end
    ''')
    assert 'Limited=yes' in limited and len(limited.encode()) <= 198000
    assert 'ReqApplyAgent [nparams=3' in limited
    assert 'SendDiscStrengthenReq [nparams=3' in limited
    assert 'OBJECT global/PlayerData.Shop class=' in limited
    no_debug = inspect(base + "\ndebug=nil")
    assert 'nparams=unknown; vararg=unknown; params=unknown' in no_debug
    broken_debug = inspect(base + "\ndebug={getinfo=function() error('PRIVATE_ERROR') end,getlocal=function() error('PRIVATE_ERROR') end}")
    assert 'nparams=unknown' in broken_debug and 'PRIVATE_ERROR' not in broken_debug
    print('PASS real Lua inventory: targeted declared functions, debug parameters/fallback, object-class links, superclass exclusion, fair quotas and zero game actions/scalar values')


if __name__ == '__main__':
    run()
