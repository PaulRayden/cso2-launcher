#include <sstream>
#include <string>

#include "tier0/icommandline.h"
#include "hooks.h"

extern bool g_bEnableLocalization;

HOOK_DETOUR_DECLARE( hkStrTblFind );

// Original function: CLocalizedStringTable::Find
NOINLINE wchar_t* __fastcall hkStrTblFind( void* ecx, void* edx, const char* pName )
{
	static wchar_t szBuffer[1024];

	if (!g_bEnableLocalization)
	{
		size_t nameLength = strlen( pName );
		MultiByteToWideChar( CP_ACP, NULL, pName, strlen( pName ), szBuffer, nameLength + 1 );
		szBuffer[nameLength] = '\0';
		return szBuffer;
	}

	return HOOK_DETOUR_GET_ORIG( hkStrTblFind )(ecx, edx, pName);
}

//
// Formats a string as "resource/[game prefix]_[language].txt"
//
std::string GetDesiredLangFile( const std::string& szGamePrefix, const char* szLanguage )
{
	std::ostringstream oss;
	oss << "resource/" << szGamePrefix << "_" << szLanguage << ".txt";
	return oss.str();
}

HOOK_DETOUR_DECLARE( hkStrTblAddFile );

//
// Allow the user to specify some language file through command line arguments
// "-lang [desired language]"
// And load the language file from "resource/cso2_[desired language].txt".
//
// Original function: CLocalizedStringTable::AddFile
NOINLINE bool __fastcall hkStrTblAddFile( void* ecx, void* edx, const char *szFileName, const char *pPathID, bool bIncludeFallbackSearchPaths )
{
	const char* szDesiredLang = CommandLine()->ParmValue( "-lang" );

	// Make sure we have a command argument
	if (!szDesiredLang)
	{
		return HOOK_DETOUR_GET_ORIG( hkStrTblAddFile )(ecx, edx, szFileName, pPathID, bIncludeFallbackSearchPaths);
	}

	std::string_view fileNameView = szFileName;
	std::string szDesiredFile;

	// Verify if we want to replace the current language file
	if (fileNameView == "resource/cso2_koreana.txt")
	{
		szDesiredFile = GetDesiredLangFile( "cso2", szDesiredLang );
	}
	else if (fileNameView == "resource/cstrike_korean.txt")
	{
		szDesiredFile = GetDesiredLangFile( "cstrike", szDesiredLang );
	}
	else if (fileNameView == "resource/chat_korean.txt")
	{
		szDesiredFile = GetDesiredLangFile( "chat", szDesiredLang );
	}
	else if (fileNameView == "resource/valve_korean.txt")
	{
		szDesiredFile = GetDesiredLangFile( "valve", szDesiredLang );
	}
	else
	{
		// if we don't want to replace the existing file, resume ordinary behavior
		return HOOK_DETOUR_GET_ORIG( hkStrTblAddFile )
			(ecx, edx, szFileName, pPathID, bIncludeFallbackSearchPaths);
	}

	// load our desired language file
	return HOOK_DETOUR_GET_ORIG( hkStrTblAddFile )(ecx, edx, szDesiredFile.c_str(), pPathID, true);
}

ON_LOAD_LIB( vgui2 )
{
	uintptr_t dwVguiBase = GET_LOAD_LIB_MODULE();
	HOOK_DETOUR( dwVguiBase + 0xAC80, hkStrTblFind );
	HOOK_DETOUR( dwVguiBase + 0x8D90, hkStrTblAddFile );
}
