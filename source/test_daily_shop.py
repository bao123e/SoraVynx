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
    case("behaviour.luaClassName='UI/ShopEx/ShopCtrl.lua'", 'run_shop();assert(clicks==1)')
    case("behaviour.luaClassName='ShopCtrl';ctrl.class=nil;setmetatable(ctrl,{__index=function(t,k) return class[k] end})", 'run_shop();assert(clicks==1)')
    case("behaviour.luaClassName='Game.UI.ShopEx.ShopPanel';behaviour.luaInstance={ctrl=ctrl}", 'run_shop();assert(clicks==1 and direct_requests==0)')
    case("behaviour.luaClassName='ShopPanel';behaviour.luaInstance={children={shop=ctrl}}", 'run_shop();assert(clicks==1)')
    case("behaviour.luaClassName='Game.UI.ShopEx.ShopPanel';behaviour.luaInstance={ctrl=ctrl};objects.Length=2;objects[1]={isActiveAndEnabled=true,luaClassName='ShopCtrl',luaInstance=ctrl}", 'run_shop();assert(clicks==1)')
    # Regression: the active UI exists while package.loaded has no canonical class entry.
    case("package.loaded['Game.UI.ShopEx.ShopCtrl']=nil", 'run_shop();assert(clicks==1 and direct_requests==0)')
    case("package.loaded['Game/UI/ShopEx/ShopCtrl']=class;package.loaded['Game.UI.ShopEx.ShopCtrl']=nil;behaviour.luaClassName='ShopCtrl'", 'run_shop();assert(clicks==1)')
    case("package.loaded['Game.UI.ShopEx.ShopCtrl']=nil;ctrl.class=nil;setmetatable(ctrl,{__index=class})", 'run_shop();assert(clicks==1)')
    case("package.loaded['Game.UI.ShopEx.ShopCtrl']=nil;ctrl.class=nil;setmetatable(ctrl,{__index=function(t,k) return class[k] end})", 'run_shop();assert(clicks==1)')
    case("class.super={OnBtnClick_DailyGift=class.OnBtnClick_DailyGift};class.OnBtnClick_DailyGift=nil", 'run_shop();assert(clicks==1)')
    case("debug.getlocal=nil", 'run_shop();assert(clicks==1 and direct_requests==0)')
    case("debug.getlocal=function() error('PRIVATE_ERROR') end", 'local text=run_shop();assert(clicks==1 and not text:find("PRIVATE_ERROR",1,true))')
    case("package.loaded['Game.UI.ShopEx.ShopCtrl']=nil;behaviour.luaClassName='ShopCtrl';local original_info=debug.getinfo;debug.getinfo=function(fn,what) if what=='S' then return {source='@Game/UI/ShopEx/ShopCtrl.lua'} end return original_info(fn,what) end", 'run_shop();assert(clicks==1)')
    for setup in ['debug=nil',
                  'class.OnBtnClick_DailyGift=function(self,unexpected) error("must not run") end',
                  'behaviour.isActiveAndEnabled=false',
                  "behaviour.luaClassName='Game.UI.Other.ShopCtrl'",
                  'behaviour.luaInstance={class={}}',
                  'objects.Length=5000',
                  'CS=nil',
                  "package.loaded['Game.UI.ShopEx.ShopCtrl']=nil;behaviour.luaClassName='ShopCtrl'",
                  "debug.getinfo=function() return {isvararg=false} end",
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
    local text=run_shop();assert(clicks==1 and direct_requests==0 and text:find('Status: UI_STATUS',1,true))
    ''')
    case('', "run_shop();PlayerData.Shop={class=data};local text=run_shop();assert(clicks==1 and text:find('SESSION_CONTEXT_CHANGED',1,true))")
    # The user's .8 failure: normal FindObjectsOfType returns zero for a visible Shop.
    resource_setup = """
    local saved=objects
    CS.UnityEngine.Object.FindObjectsOfType=function(t) return {Length=0} end
    behaviour.gameObject={activeInHierarchy=true,scene={IsValid=function(self) return true end}}
    CS.UnityEngine.Resources={FindObjectsOfTypeAll=function(t) assert(t==system_type);return saved end}
    """
    case(resource_setup, 'run_shop();assert(clicks==1 and direct_requests==0)')
    case(resource_setup+";behaviour.gameObject.scene={isLoaded=true}", 'run_shop();assert(clicks==1)')
    case(resource_setup+";behaviour.gameObject.scene={isLoaded=false}", 'run_shop();assert(clicks==0)')
    case(resource_setup+";behaviour.gameObject.scene.IsValid=function(self) return false end", 'local t=run_shop();assert(clicks==0 and t:find("all=1",1,true))')
    case(resource_setup+";behaviour.gameObject.activeInHierarchy=false", 'run_shop();assert(clicks==0)')
    case(resource_setup+";behaviour.isActiveAndEnabled=false", 'run_shop();assert(clicks==0)')
    controller_setup = """
    local controller_type={};local controller_system_type={}
    CS.LuaUIController=controller_type
    typeof=function(t) if t==controller_type then return controller_system_type end assert(t==lua_behaviour_type);return system_type end
    CS.UnityEngine.Object.FindObjectsOfType=function(t) return {Length=0} end
    behaviour.isActiveAndEnabled=false
    local ui={isActiveAndEnabled=true,luaBehaviour=behaviour,gameObject={activeInHierarchy=true,scene={IsValid=function(self) return true end}}}
    CS.UnityEngine.Resources={FindObjectsOfTypeAll=function(t)
      if t==controller_system_type then return {Length=1,[0]=ui} end return {Length=0}
    end}
    """
    case(controller_setup, 'run_shop();assert(clicks==1 and direct_requests==0)')
    registry_setup = """
    CS.UnityEngine.Object.FindObjectsOfType=function(t) return {Length=0} end
    local SECRET_PANEL_ID=200987654
    local panels={[SECRET_PANEL_ID]={children={shop=ctrl}}}
    local current_id=SECRET_PANEL_ID
    PanelId={ShopEx=SECRET_PANEL_ID,Home=1}
    PanelManager={GetCurPanelId=function() return current_id end,
      CheckPanelOpen=function(id) return panels[id]~=nil end,
      OpenPanelWithContext=function() assert(panels);error('opening a panel is forbidden in this test') end}
    """
    nested_registry = """
    CS.UnityEngine.Object.FindObjectsOfType=function(t) return {Length=0} end
    local panels={[17]={children={shop=ctrl}}};local current=17;unsafe_called=0
    local function panel_lookup(id) unsafe_called=unsafe_called+1;return panels[id] end
    PanelId={Shop=17};PanelManager={GetCurPanelId=function() return current end,
      CheckPanelOpen=function(id) return panel_lookup(id)~=nil end}
    """
    case(nested_registry, 'run_shop();assert(clicks==1 and direct_requests==0 and unsafe_called==0)')
    case(registry_setup+";PanelId={[200987654]='ShopEx'}", 'run_shop();assert(clicks==1 and direct_requests==0)')
    case(registry_setup+";CS=nil", 'run_shop();assert(clicks==1 and direct_requests==0)')
    case(registry_setup, 'local text=run_shop();assert(clicks==1 and direct_requests==0);assert(not text:find("200987654",1,true));run_shop();assert(clicks==1)')
    case(registry_setup+";PanelManager.GetCurPanelId=function() return 1 end", 'local text=run_shop();assert(clicks==0 and text:find("SHOP_NOT_CURRENT",1,true))')
    case(registry_setup+";PanelId={OtherPopup=200987654}", 'local text=run_shop();assert(clicks==0 and text:find("currentShop=enum_unavailable",1,true))')
    case(registry_setup+";debug.getupvalue=nil", 'run_shop();assert(clicks==0)')
    case('CS.UnityEngine.Object.FindObjectsOfType=function(t) return {Length=0} end', 'local t=run_shop();assert(clicks==0 and t:find("normal=0",1,true))')
    for setup, code in [
        ("debug=nil", "DEBUG_ARITY_UNAVAILABLE"),
        ("class.OnBtnClick_DailyGift=function(self,other) end", "HANDLER_ARITY_MISMATCH"),
        ("class.OnBtnClick_DailyGift=function(button) end", "HANDLER_PARAMETER_MISMATCH"),
        ("class.OnBtnClick_DailyGift=nil", "SHOP_HANDLER_MISSING"),
        ("behaviour.luaInstance=nil", "SHOP_INSTANCE_UNAVAILABLE"),
        ("CS=nil", "UNITY_UI_ENUMERATION_FAILED")]:
        case(setup, 'local text=run_shop();assert(clicks==0 and direct_requests==0 and text:find("Code: '+code+'",1,true))')
    print('PASS Daily Gift UI: zero normal objects, loaded live-scene discovery, controller links, proven current-Shop Lua registry, active/prefab guards, exact handler and no duplicate/RPC fallback')


if __name__ == '__main__':
    run()
