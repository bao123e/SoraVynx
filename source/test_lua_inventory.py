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

    base = '''
    action_calls=0
    local function forbidden() action_calls=action_calls+1; error('game action called') end
    DailyMissionManager={ClaimAll=forbidden, Send=forbidden, AccountToken='PRIVATE_TOKEN',
      UserName='PRIVATE_NAME', [123456789]='PRIVATE_ID', ['secret@example.com']=forbidden}
    DailyMissionManager.self=DailyMissionManager
    setmetatable(DailyMissionManager,{__index=forbidden,__pairs=forbidden,__tostring=forbidden})
    package.loaded['Game.DailyAffairs']=DailyMissionManager
    package.loaded['Game.Dispatch']={StartCommission=forbidden}
    package.loaded['Game.Disc']={Upgrade=forbidden}
    package.loaded['Game.Heartlink']={SendGift=forbidden}
    '''
    report = inspect(base)
    for required in ['Lua report v4', 'MODULE Game.DailyAffairs table',
                     'FUNCTION loaded/Game.DailyAffairs.ClaimAll',
                     'FUNCTION loaded/Game.Dispatch.StartCommission',
                     'FUNCTION loaded/Game.Disc.Upgrade',
                     'FUNCTION loaded/Game.Heartlink.SendGift', 'Limited=no', 'ALIAS ']:
        assert required in report, required
    for secret in ['PRIVATE_TOKEN', 'PRIVATE_NAME', 'PRIVATE_ID', '123456789', 'secret@example.com', 'AccountToken', 'UserName']:
        assert secret not in report, secret
    # Arbitrary insertion order must produce the same inventory.
    reordered = inspect(base.replace('ClaimAll=forbidden, Send=forbidden', 'Send=forbidden, ClaimAll=forbidden'))
    assert report == reordered
    body = [line for line in report.splitlines() if line.startswith(('GLOBAL ', 'MODULE ', 'FUNCTION ', 'TABLE ', 'ALIAS '))]
    assert body == sorted(body)
    limited = inspect(base + '''
    package.loaded['Game.DailyHuge']={}
    for i=1,40000 do package.loaded['Game.DailyHuge']['Action_'..i]=function() action_calls=action_calls+1 end end
    ''')
    assert 'Limited=yes' in limited and len(limited.encode()) <= 198000
    assert 'MODULE Game.Heartlink table' in limited
    deep = inspect(base + '''
    local t={};package.loaded['Game.DailyDeep']=t
    for i=1,100 do t.Child={};t=t.Child end
    ''')
    assert 'Limited=yes' in deep
    print('PASS real Lua inventory: deterministic module/function names, cycles/depth/budgets, no scalar values, no metamethods or game actions')


if __name__ == '__main__':
    run()
