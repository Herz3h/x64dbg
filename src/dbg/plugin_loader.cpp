/**
 @file plugin_loader.cpp

 @brief Implements the plugin loader.
 */

#include "plugin_loader.h"
#include "console.h"
#include "debugger.h"
#include "threading.h"

/**
\brief List of plugins.
*/
static std::vector<PLUG_DATA> pluginList;

/**
\brief The current plugin handle.
*/
static int curPluginHandle = 0;

/**
\brief List of plugin callbacks.
*/
static std::vector<PLUG_CALLBACK> pluginCallbackList;

/**
\brief List of plugin commands.
*/
static std::vector<PLUG_COMMAND> pluginCommandList;

/**
\brief List of plugin menus.
*/
static std::vector<PLUG_MENU> pluginMenuList;

/**
\brief Loads plugins from a specified directory.
\param pluginDir The directory to load plugins from.
*/
void pluginload(const char* pluginDir)
{
    //load new plugins
    wchar_t currentDir[deflen] = L"";
    GetCurrentDirectoryW(deflen, currentDir);
    SetCurrentDirectoryW(StringUtils::Utf8ToUtf16(pluginDir).c_str());
    char searchName[deflen] = "";
#ifdef _WIN64
    sprintf(searchName, "%s\\*.dp64", pluginDir);
#else
    sprintf(searchName, "%s\\*.dp32", pluginDir);
#endif // _WIN64
    WIN32_FIND_DATAW foundData;
    HANDLE hSearch = FindFirstFileW(StringUtils::Utf8ToUtf16(searchName).c_str(), &foundData);
    if(hSearch == INVALID_HANDLE_VALUE)
    {
        SetCurrentDirectoryW(currentDir);
        return;
    }
    PLUG_DATA pluginData;
    do
    {
        //set plugin data
        pluginData.initStruct.pluginHandle = curPluginHandle;
        char szPluginPath[MAX_PATH] = "";
        sprintf_s(szPluginPath, "%s\\%s", pluginDir, StringUtils::Utf16ToUtf8(foundData.cFileName).c_str());
        pluginData.hPlugin = LoadLibraryW(StringUtils::Utf8ToUtf16(szPluginPath).c_str()); //load the plugin library
        if(!pluginData.hPlugin)
        {
            dprintf("[PLUGIN] Failed to load plugin: %s\n", StringUtils::Utf16ToUtf8(foundData.cFileName).c_str());
            continue;
        }
        pluginData.pluginit = (PLUGINIT)GetProcAddress(pluginData.hPlugin, "pluginit");
        if(!pluginData.pluginit)
        {
            dprintf("[PLUGIN] Export \"pluginit\" not found in plugin: %s\n", StringUtils::Utf16ToUtf8(foundData.cFileName).c_str());
            FreeLibrary(pluginData.hPlugin);
            continue;
        }
        pluginData.plugstop = (PLUGSTOP)GetProcAddress(pluginData.hPlugin, "plugstop");
        pluginData.plugsetup = (PLUGSETUP)GetProcAddress(pluginData.hPlugin, "plugsetup");

        //auto-register callbacks for certain export names
        CBPLUGIN cbPlugin;
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBALLEVENTS");
        if(cbPlugin)
        {
            pluginregistercallback(curPluginHandle, CB_INITDEBUG, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_STOPDEBUG, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_CREATEPROCESS, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_EXITPROCESS, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_CREATETHREAD, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_EXITTHREAD, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_SYSTEMBREAKPOINT, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_LOADDLL, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_UNLOADDLL, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_OUTPUTDEBUGSTRING, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_EXCEPTION, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_BREAKPOINT, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_PAUSEDEBUG, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_RESUMEDEBUG, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_STEPPED, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_ATTACH, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_DETACH, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_DEBUGEVENT, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_MENUENTRY, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_WINEVENT, cbPlugin);
            pluginregistercallback(curPluginHandle, CB_WINEVENTGLOBAL, cbPlugin);
        }
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBINITDEBUG");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_INITDEBUG, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBSTOPDEBUG");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_STOPDEBUG, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBCREATEPROCESS");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_CREATEPROCESS, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBEXITPROCESS");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_EXITPROCESS, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBCREATETHREAD");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_CREATETHREAD, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBEXITTHREAD");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_EXITTHREAD, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBSYSTEMBREAKPOINT");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_SYSTEMBREAKPOINT, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBLOADDLL");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_LOADDLL, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBUNLOADDLL");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_UNLOADDLL, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBOUTPUTDEBUGSTRING");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_OUTPUTDEBUGSTRING, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBEXCEPTION");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_EXCEPTION, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBBREAKPOINT");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_BREAKPOINT, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBPAUSEDEBUG");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_PAUSEDEBUG, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBRESUMEDEBUG");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_RESUMEDEBUG, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBSTEPPED");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_STEPPED, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBATTACH");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_ATTACH, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBDETACH");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_DETACH, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBDEBUGEVENT");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_DEBUGEVENT, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBMENUENTRY");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_MENUENTRY, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBWINEVENT");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_WINEVENT, cbPlugin);
        cbPlugin = (CBPLUGIN)GetProcAddress(pluginData.hPlugin, "CBWINEVENTGLOBAL");
        if(cbPlugin)
            pluginregistercallback(curPluginHandle, CB_WINEVENTGLOBAL, cbPlugin);

        //init plugin
        if(!pluginData.pluginit(&pluginData.initStruct))
        {
            dprintf("[PLUGIN] pluginit failed for plugin: %s\n", foundData.cFileName);
            FreeLibrary(pluginData.hPlugin);
            continue;
        }
        else if(pluginData.initStruct.sdkVersion < PLUG_SDKVERSION) //the plugin SDK is not compatible
        {
            dprintf("[PLUGIN] %s is incompatible with this SDK version\n", pluginData.initStruct.pluginName);
            FreeLibrary(pluginData.hPlugin);
            continue;
        }
        else
            dprintf("[PLUGIN] %s v%d Loaded!\n", pluginData.initStruct.pluginName, pluginData.initStruct.pluginVersion);

        SectionLocker<LockPluginMenuList, false> menuLock; //exclusive lock

        //add plugin menu
        int hNewMenu = GuiMenuAdd(GUI_PLUGIN_MENU, pluginData.initStruct.pluginName);
        if(hNewMenu == -1)
        {
            dprintf("[PLUGIN] GuiMenuAdd(GUI_PLUGIN_MENU) failed for plugin: %s\n", pluginData.initStruct.pluginName);
            pluginData.hMenu = -1;
        }
        else
        {
            PLUG_MENU newMenu;
            newMenu.hEntryMenu = hNewMenu;
            newMenu.hEntryPlugin = -1;
            newMenu.pluginHandle = pluginData.initStruct.pluginHandle;
            pluginMenuList.push_back(newMenu);
            pluginData.hMenu = newMenu.hEntryMenu;
        }

        //add disasm plugin menu
        hNewMenu = GuiMenuAdd(GUI_DISASM_MENU, pluginData.initStruct.pluginName);
        if(hNewMenu == -1)
        {
            dprintf("[PLUGIN] GuiMenuAdd(GUI_DISASM_MENU) failed for plugin: %s\n", pluginData.initStruct.pluginName);
            pluginData.hMenu = -1;
        }
        else
        {
            PLUG_MENU newMenu;
            newMenu.hEntryMenu = hNewMenu;
            newMenu.hEntryPlugin = -1;
            newMenu.pluginHandle = pluginData.initStruct.pluginHandle;
            pluginMenuList.push_back(newMenu);
            pluginData.hMenuDisasm = newMenu.hEntryMenu;
        }

        //add dump plugin menu
        hNewMenu = GuiMenuAdd(GUI_DUMP_MENU, pluginData.initStruct.pluginName);
        if(hNewMenu == -1)
        {
            dprintf("[PLUGIN] GuiMenuAdd(GUI_DUMP_MENU) failed for plugin: %s\n", pluginData.initStruct.pluginName);
            pluginData.hMenu = -1;
        }
        else
        {
            PLUG_MENU newMenu;
            newMenu.hEntryMenu = hNewMenu;
            newMenu.hEntryPlugin = -1;
            newMenu.pluginHandle = pluginData.initStruct.pluginHandle;
            pluginMenuList.push_back(newMenu);
            pluginData.hMenuDump = newMenu.hEntryMenu;
        }

        //add stack plugin menu
        hNewMenu = GuiMenuAdd(GUI_STACK_MENU, pluginData.initStruct.pluginName);
        if(hNewMenu == -1)
        {
            dprintf("[PLUGIN] GuiMenuAdd(GUI_STACK_MENU) failed for plugin: %s\n", pluginData.initStruct.pluginName);
            pluginData.hMenu = -1;
        }
        else
        {
            PLUG_MENU newMenu;
            newMenu.hEntryMenu = hNewMenu;
            newMenu.hEntryPlugin = -1;
            newMenu.pluginHandle = pluginData.initStruct.pluginHandle;
            pluginMenuList.push_back(newMenu);
            pluginData.hMenuStack = newMenu.hEntryMenu;
        }
        menuLock.Unlock();

        //add the plugin to the list
        SectionLocker<LockPluginList, false> pluginLock; //exclusive lock
        pluginList.push_back(pluginData);
        pluginLock.Unlock();

        //setup plugin
        if(pluginData.plugsetup)
        {
            PLUG_SETUPSTRUCT setupStruct;
            setupStruct.hwndDlg = GuiGetWindowHandle();
            setupStruct.hMenu = pluginData.hMenu;
            setupStruct.hMenuDisasm = pluginData.hMenuDisasm;
            setupStruct.hMenuDump = pluginData.hMenuDump;
            setupStruct.hMenuStack = pluginData.hMenuStack;
            pluginData.plugsetup(&setupStruct);
        }
        curPluginHandle++;
    }
    while(FindNextFileW(hSearch, &foundData));
    SetCurrentDirectoryW(currentDir);
}

/**
\brief Unregister all plugin commands.
\param pluginHandle Handle of the plugin to remove the commands from.
*/
static void plugincmdunregisterall(int pluginHandle)
{
    SHARED_ACQUIRE(LockPluginCommandList);
    auto commandList = pluginCommandList; //copy for thread-safety reasons
    SHARED_RELEASE();
    auto i = commandList.begin();
    while(i != commandList.end())
    {
        auto currentCommand = *i;
        if(currentCommand.pluginHandle == pluginHandle)
        {
            i = commandList.erase(i);
            dbgcmddel(currentCommand.command);
        }
        else
            ++i;
    }
}

/**
\brief Unloads all plugins.
*/
void pluginunload()
{
    {
        EXCLUSIVE_ACQUIRE(LockPluginList);
        for(const auto & currentPlugin : pluginList)  //call plugin stop & unregister all plugin commands
        {
            PLUGSTOP stop = currentPlugin.plugstop;
            if(stop)
                stop();
            plugincmdunregisterall(currentPlugin.initStruct.pluginHandle);
        }
    }
    {
        EXCLUSIVE_ACQUIRE(LockPluginCallbackList);
        pluginCallbackList.clear(); //remove all callbacks
    }
    {
        EXCLUSIVE_ACQUIRE(LockPluginMenuList);
        pluginMenuList.clear(); //clear menu list
    }
    {
        EXCLUSIVE_ACQUIRE(LockPluginList);
        for(const auto & currentPlugin : pluginList)  //free the libraries
            FreeLibrary(currentPlugin.hPlugin);
    }
    GuiMenuClear(GUI_PLUGIN_MENU); //clear the plugin menu
}

/**
\brief Register a plugin callback.
\param pluginHandle Handle of the plugin to register a callback for.
\param cbType The type of the callback to register.
\param cbPlugin The actual callback function.
*/
void pluginregistercallback(int pluginHandle, CBTYPE cbType, CBPLUGIN cbPlugin)
{
    pluginunregistercallback(pluginHandle, cbType); //remove previous callback
    PLUG_CALLBACK cbStruct;
    cbStruct.pluginHandle = pluginHandle;
    cbStruct.cbType = cbType;
    cbStruct.cbPlugin = cbPlugin;
    EXCLUSIVE_ACQUIRE(LockPluginCallbackList);
    pluginCallbackList.push_back(cbStruct);
}

/**
\brief Unregister all plugin callbacks of a certain type.
\param pluginHandle Handle of the plugin to unregister a callback from.
\param cbType The type of the callback to unregister.
*/
bool pluginunregistercallback(int pluginHandle, CBTYPE cbType)
{
    EXCLUSIVE_ACQUIRE(LockPluginCallbackList);
    for(auto it = pluginCallbackList.begin(); it != pluginCallbackList.end(); ++it)
    {
        const auto & currentCallback = *it;
        if(currentCallback.pluginHandle == pluginHandle && currentCallback.cbType == cbType)
        {
            pluginCallbackList.erase(it);
            return true;
        }
    }
    return false;
}

/**
\brief Call all registered callbacks of a certain type.
\param cbType The type of callbacks to call.
\param [in,out] callbackInfo Information describing the callback. See plugin documentation for more information on this.
*/
void plugincbcall(CBTYPE cbType, void* callbackInfo)
{
    SHARED_ACQUIRE(LockPluginCallbackList);
    auto callbackList = pluginCallbackList; //copy for thread-safety reasons
    SHARED_RELEASE();
    for(const auto & currentCallback : callbackList)
    {
        if(currentCallback.cbType == cbType)
        {
            CBPLUGIN cbPlugin = currentCallback.cbPlugin;
            if(!IsBadReadPtr((const void*)cbPlugin, sizeof(duint)))
                cbPlugin(cbType, callbackInfo);
        }
    }
}

/**
\brief Register a plugin command.
\param pluginHandle Handle of the plugin to register a command for.
\param command The command text to register. This text cannot contain the '\1' character. This text is not case sensitive.
\param cbCommand The command callback.
\param debugonly true if the command can only be called during debugging.
\return true if it the registration succeeded, false otherwise.
*/
bool plugincmdregister(int pluginHandle, const char* command, CBPLUGINCOMMAND cbCommand, bool debugonly)
{
    if(!command || strlen(command) >= deflen || strstr(command, "\1"))
        return false;
    PLUG_COMMAND plugCmd;
    plugCmd.pluginHandle = pluginHandle;
    strcpy_s(plugCmd.command, command);
    if(!dbgcmdnew(command, (CBCOMMAND)cbCommand, debugonly))
        return false;
    EXCLUSIVE_ACQUIRE(LockPluginCommandList);
    pluginCommandList.push_back(plugCmd);
    EXCLUSIVE_RELEASE();
    dprintf("[PLUGIN] command \"%s\" registered!\n", command);
    return true;
}

/**
\brief Unregister a plugin command.
\param pluginHandle Handle of the plugin to unregister the command from.
\param command The command text to unregister. This text is not case sensitive.
\return true if the command was found and removed, false otherwise.
*/
bool plugincmdunregister(int pluginHandle, const char* command)
{
    if(!command || strlen(command) >= deflen || strstr(command, "\1"))
        return false;
    EXCLUSIVE_ACQUIRE(LockPluginCommandList);
    for(auto it = pluginCommandList.begin(); it != pluginCommandList.end(); ++it)
    {
        const auto & currentCommand = *it;
        if(currentCommand.pluginHandle == pluginHandle && !strcmp(currentCommand.command, command))
        {
            pluginCommandList.erase(it);
            EXCLUSIVE_RELEASE();
            if(!dbgcmddel(command))
                return false;
            dprintf("[PLUGIN] command \"%s\" unregistered!\n", command);
            return true;
        }
    }
    return false;
}

/**
\brief Add a new plugin (sub)menu.
\param hMenu The menu handle to add the (sub)menu to.
\param title The title of the (sub)menu.
\return The handle of the new (sub)menu.
*/
int pluginmenuadd(int hMenu, const char* title)
{
    if(!title || !strlen(title))
        return -1;
    EXCLUSIVE_ACQUIRE(LockPluginMenuList);
    int nFound = -1;
    for(unsigned int i = 0; i < pluginMenuList.size(); i++)
    {
        if(pluginMenuList.at(i).hEntryMenu == hMenu && pluginMenuList.at(i).hEntryPlugin == -1)
        {
            nFound = i;
            break;
        }
    }
    if(nFound == -1) //not a valid menu handle
        return -1;
    int hMenuNew = GuiMenuAdd(pluginMenuList.at(nFound).hEntryMenu, title);
    PLUG_MENU newMenu;
    newMenu.pluginHandle = pluginMenuList.at(nFound).pluginHandle;
    newMenu.hEntryPlugin = -1;
    newMenu.hEntryMenu = hMenuNew;
    pluginMenuList.push_back(newMenu);
    return hMenuNew;
}

/**
\brief Add a plugin menu entry to a menu.
\param hMenu The menu to add the entry to.
\param hEntry The handle you like to have the entry. This should be a unique value in the scope of the plugin that registered the \p hMenu.
\param title The menu entry title.
\return true if the \p hEntry was unique and the entry was successfully added, false otherwise.
*/
bool pluginmenuaddentry(int hMenu, int hEntry, const char* title)
{
    if(!title || !strlen(title) || hEntry == -1)
        return false;
    EXCLUSIVE_ACQUIRE(LockPluginMenuList);
    int pluginHandle = -1;
    //find plugin handle
    for(const auto & currentMenu : pluginMenuList)
    {
        if(currentMenu.hEntryMenu == hMenu && currentMenu.hEntryPlugin == -1)
        {
            pluginHandle = currentMenu.pluginHandle;
            break;
        }
    }
    if(pluginHandle == -1) //not found
        return false;
    //search if hEntry was previously used
    for(const auto & currentMenu : pluginMenuList)
        if(currentMenu.pluginHandle == pluginHandle && currentMenu.hEntryPlugin == hEntry)
            return false;
    int hNewEntry = GuiMenuAddEntry(hMenu, title);
    if(hNewEntry == -1)
        return false;
    PLUG_MENU newMenu;
    newMenu.hEntryMenu = hNewEntry;
    newMenu.hEntryPlugin = hEntry;
    newMenu.pluginHandle = pluginHandle;
    pluginMenuList.push_back(newMenu);
    return true;
}

/**
\brief Add a menu separator to a menu.
\param hMenu The menu to add the separator to.
\return true if it succeeds, false otherwise.
*/
bool pluginmenuaddseparator(int hMenu)
{
    SHARED_ACQUIRE(LockPluginMenuList);
    for(const auto & currentMenu : pluginMenuList)
    {
        if(currentMenu.hEntryMenu == hMenu && currentMenu.hEntryPlugin == -1)
        {
            GuiMenuAddSeparator(hMenu);
            return true;
        }
    }
    return false;
}

/**
\brief Clears a plugin menu.
\param hMenu The menu to clear.
\return true if it succeeds, false otherwise.
*/
bool pluginmenuclear(int hMenu)
{
    EXCLUSIVE_ACQUIRE(LockPluginMenuList);
    bool bFound = false;
    for(auto it = pluginMenuList.begin(); it != pluginMenuList.end(); ++it)
    {
        const auto & currentMenu = *it;
        if(currentMenu.hEntryMenu == hMenu && currentMenu.hEntryPlugin == -1)
        {
            it = pluginMenuList.erase(it);
            bFound = true;
        }
    }
    if(!bFound)
        return false;
    GuiMenuClear(hMenu);
    return true;
}

/**
\brief Call the registered CB_MENUENTRY callbacks for a menu entry.
\param hEntry The menu entry that triggered the event.
*/
void pluginmenucall(int hEntry)
{
    if(hEntry == -1)
        return;

    SectionLocker<LockPluginMenuList, true> menuLock; //shared lock
    auto i = pluginMenuList.begin();
    while(i != pluginMenuList.end())
    {
        const auto currentMenu = *i;
        ++i;
        if(currentMenu.hEntryMenu == hEntry && currentMenu.hEntryPlugin != -1)
        {
            PLUG_CB_MENUENTRY menuEntryInfo;
            menuEntryInfo.hEntry = currentMenu.hEntryPlugin;
            SectionLocker<LockPluginCallbackList, true> callbackLock; //shared lock
            auto j = pluginCallbackList.begin();
            while(j != pluginCallbackList.end())
            {
                const auto currentCallback = *j;
                ++j;
                if(currentCallback.pluginHandle == currentMenu.pluginHandle && currentCallback.cbType == CB_MENUENTRY)
                {
                    menuLock.Unlock();
                    callbackLock.Unlock();
                    currentCallback.cbPlugin(CB_MENUENTRY, &menuEntryInfo);
                    return;
                }
            }
        }
    }
}

/**
\brief Calls the registered CB_WINEVENT callbacks.
\param [in,out] message the message that triggered the event. Cannot be null.
\param [out] result The result value. Cannot be null.
\return The value the plugin told it to return. See plugin documentation for more information.
*/
bool pluginwinevent(MSG* message, long* result)
{
    PLUG_CB_WINEVENT winevent;
    winevent.message = message;
    winevent.result = result;
    winevent.retval = false;
    plugincbcall(CB_WINEVENT, &winevent);
    return winevent.retval;
}

/**
\brief Calls the registered CB_WINEVENTGLOBAL callbacks.
\param [in,out] message the message that triggered the event. Cannot be null.
\return The value the plugin told it to return. See plugin documentation for more information.
*/
bool pluginwineventglobal(MSG* message)
{
    PLUG_CB_WINEVENTGLOBAL winevent;
    winevent.message = message;
    winevent.retval = false;
    plugincbcall(CB_WINEVENTGLOBAL, &winevent);
    return winevent.retval;
}

/**
\brief Sets an icon for a menu.
\param hMenu The menu handle.
\param icon The icon (can be all kinds of formats).
*/
void pluginmenuseticon(int hMenu, const ICONDATA* icon)
{
    SHARED_ACQUIRE(LockPluginMenuList);
    for(const auto & currentMenu : pluginMenuList)
    {
        if(currentMenu.hEntryMenu == hMenu && currentMenu.hEntryPlugin == -1)
        {
            GuiMenuSetIcon(hMenu, icon);
            break;
        }
    }
}

/**
\brief Sets an icon for a menu entry.
\param pluginHandle Plugin handle.
\param hEntry The menu entry handle (unique per plugin).
\param icon The icon (can be all kinds of formats).
*/
void pluginmenuentryseticon(int pluginHandle, int hEntry, const ICONDATA* icon)
{
    if(hEntry == -1)
        return;
    SHARED_ACQUIRE(LockPluginMenuList);
    for(const auto & currentMenu : pluginMenuList)
    {
        if(currentMenu.pluginHandle == pluginHandle && currentMenu.hEntryPlugin == hEntry)
        {
            GuiMenuSetEntryIcon(currentMenu.hEntryMenu, icon);
            break;
        }
    }
}