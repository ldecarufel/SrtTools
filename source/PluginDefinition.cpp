// SRT Tools Notepad++ Plugin
// Copyright (C)2024 Louis de Carufel (https://github.com/ldecarufel)

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.


#include "PluginDefinition.h"
#include "menuCmdID.h"
#include <stdio.h>
#include <Shlwapi.h>
#include <string>
#include "SrtToolsPanel.h"

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;

PluginConfig pluginConfig;
std::wstring confPath;
SrtToolPanel toolPanelInstance;

//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE hModule)
{
	toolPanelInstance.init((HINSTANCE)hModule, NULL);
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{
    //--------------------------------------------//
    //-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
    //--------------------------------------------//
    // with function :
    // setCommand(int index,                      // zero based number to indicate the order of command
    //            TCHAR *commandName,             // the command name that you want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
    //            bool check0nInit                // optional. Make this menu item be checked visually
    //            );
	setCommand(FUNC_SRTTOOLSPANEL_INDEX, TEXT("SrtTools Panel"), menu_displayToolPanel, NULL, false);
	setCommand(1, TEXT("---"), NULL, NULL, false);
	setCommand(2, TEXT("Edit Configuration File"), menu_editConfigFile, NULL, false);
	setCommand(3, TEXT("About"), menu_displayAboutDialog, NULL, false);
}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
	// Don't forget to deallocate your shortcut here
}


//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}

//----------------------------------------------//
//-- STEP 4. DEFINE YOUR ASSOCIATED FUNCTIONS --//
//----------------------------------------------//

HWND getCurrentScintillaHandle() {
    int currentEdit;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&currentEdit);
	return (currentEdit == 0)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;
}

const TCHAR *pluginConfName = TEXT("srttool.ini");
const TCHAR *srtToolsxSectionName = TEXT("SrtTools");
const TCHAR *removeExtraText = TEXT("removeExtraText");

void getConfigFromFile(const TCHAR* configFilePath, PluginConfig& config)
{
	TCHAR cmdNames[MAX_PATH];
	::GetPrivateProfileSectionNames(cmdNames, MAX_PATH, configFilePath);
	TCHAR *pFn = cmdNames;

	if (*pFn && wcscmp(pFn, srtToolsxSectionName) == 0)
	{
		int val = GetPrivateProfileInt(pFn, removeExtraText, 0, configFilePath);
		config.m_removeExtraText = val != 0;
	}
}
// 
// if conf file does not exist, then create it and load it.
void loadConfigFile()
{
	TCHAR confDir[MAX_PATH];
	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)confDir);
	confPath = confDir;
	confPath += TEXT("\\");
	confPath += pluginConfName;
	
	const char defaultConfigFileContents[] = "\
; This section contains the configuration for the SrtTools plugin.\n\
; If you modifythis file  directly, please restart your Notepad++ to take effect.\n\
; * removeExtraText: Determines if the tool will cleanup extra text not part of SRT definition.\n\
[SrtTools]\n\
removeExtraText=0\n\
\n";

	if (!::PathFileExists(confPath.c_str()))
	{
		FILE *f = generic_fopen(confPath.c_str(), TEXT("w"));
		if (f)
		{
			fwrite(defaultConfigFileContents, sizeof(defaultConfigFileContents[0]), strlen(defaultConfigFileContents), f);
			fflush(f);
			fclose(f);
		}
		/*
		else
		{
			std::wstring msg = confPath;
			msg += TEXT(" is absent, and this file cannot be create.");
			::MessageBox(nppData._nppHandle, msg.c_str(), TEXT("Not present"), MB_OK);
		}
		*/
	}
	getConfigFromFile(confPath.c_str(), pluginConfig);
}

void menu_displayAboutDialog()
{
	std::wstring aboutMsg = TEXT("Version: ");
	aboutMsg += TEXT(VERSION_VALUE);
	aboutMsg += TEXT("\r");
	aboutMsg += TEXT("License: GPL\r");
	aboutMsg += TEXT("Author: Louis de Carufel <github.com/ldecarufel>\r");
	::MessageBox(nppData._nppHandle, aboutMsg.c_str(), TEXT("SrtTool Plugin for Notepad++"), MB_OK);
}

void menu_editConfigFile()
{
	if (!::PathFileExists(confPath.c_str()))
	{
		std::wstring msg = confPath + TEXT(" is not present.\rPlease create this file manually.");
		::MessageBox(nppData._nppHandle, msg.c_str(), TEXT("Configuration file is absent"), MB_OK);
		return;
	}
	::SendMessage(nppData._nppHandle, NPPM_DOOPEN, 0, (LPARAM)confPath.c_str());
}

void menu_displayToolPanel()
{
	toolPanelInstance.setParent(nppData._nppHandle);
	tTbData	data = {0};

	if (!toolPanelInstance.isCreated())
	{
		toolPanelInstance.create(&data);

		// define the default docking behaviour
		data.uMask = DWS_DF_FLOATING;

		data.pszModuleName = toolPanelInstance.getPluginFileName();

		// the dlgID should be the index of funcItem where the current function pointer is
		data.dlgID = FUNC_SRTTOOLSPANEL_INDEX;
		::SendMessage(nppData._nppHandle, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&data);
	}

	toolPanelInstance.setCleanOutput(pluginConfig.m_removeExtraText);
	toolPanelInstance.display();
}
