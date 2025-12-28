/****************************************************************************************\
 *		Easy Handler for VS_FIXEDFILEINFO.
 *		Code taken from an article of Paul DiLascia. MSDN April 1998
 *		http://www.microsoft.com/msj/0498/c0498.aspx
\****************************************************************************************/


#include "FileVersionInfo.h"
#include <iomanip>

/////////////////////////////////////////////////////////////////////////////
//	Library

#pragma comment(lib, "version.lib")

/////////////////////////////////////////////////////////////////////////////
//	CFileVersionInfo

CFileVersionInfo::CFileVersionInfo()
	: m_translation{}
{
	m_pVersionInfo = NULL;				
}

CFileVersionInfo::~CFileVersionInfo()
{
	Clear();	
}

void CFileVersionInfo::Clear()
{
	delete [] m_pVersionInfo;
	m_pVersionInfo = NULL;

	// default = ANSI code page
	m_translation.langID = 0;
	m_translation.charset = 1252;
	// Die Basisklasse löschen
	memset(static_cast<VS_FIXEDFILEINFO*>(this), 0, sizeof(VS_FIXEDFILEINFO));
}

/////////////////////////////////////////////////////////////////////////////
//	CFileVersionInfo::GetFileVersionInfo
//		Get file version info for a given module
//		Allocates storage for all info, fills "this" with
//		VS_FIXEDFILEINFO, and sets codepage.
//

bool CFileVersionInfo::GetFileVersionInfo(HMODULE hModule)
{
	char path[_MAX_PATH];
	::GetModuleFileNameA(hModule,path,sizeof(path));
	return GetFileVersionInfo(path);
}


bool CFileVersionInfo::GetFileVersionInfo(LPCSTR szFilename)
{
	// Daten löschen
	Clear();

	// read file version info
	DWORD dwDummyHandle; 
	DWORD dwLen = ::GetFileVersionInfoSizeA(szFilename, &dwDummyHandle);
	if (dwLen==0)
		return false;

	// Buffer besorgen
	m_pVersionInfo = new BYTE[dwLen]; 
	if (!::GetFileVersionInfoA(szFilename,0,dwLen,m_pVersionInfo))
	{
		// Buffer freigeben
		Clear();
		return false;
	}

	// Daten umladen
	LPVOID lpvi;
	UINT iLen;
	if (!::VerQueryValueA(m_pVersionInfo, "\\", &lpvi, &iLen))
	{
		Clear();
		return false;
	}

	// copy fixed info to myself, which am derived from VS_FIXEDFILEINFO
	*static_cast<VS_FIXEDFILEINFO*>(this) = *static_cast<VS_FIXEDFILEINFO*>(lpvi);

	// Get translation info
	if (::VerQueryValueA(m_pVersionInfo,"\\VarFileInfo\\Translation",&lpvi,&iLen) && iLen>=4) 
		m_translation = *static_cast<TRANSLATION*>(lpvi);
	return dwSignature==VS_FFI_SIGNATURE;
}


/////////////////////////////////////////////////////////////////////////////
//	CFileVersionInfo::GetValue
//		Get string file info.
//		Key name is something like "CompanyName".
//		returns the value as a std::string.

std::string CFileVersionInfo::GetValue(LPCSTR pszKeyName) const
{
	std::string sVal;
	if (m_pVersionInfo) 
	{
		// To get a string value must pass query in the form
		//    "\StringFileInfo\<langID><codepage>\keyname"
		// where <lang-codepage> is the languageID concatenated with the
		// code page, in hex. Wow.
		std::stringstream strQuery;
		strQuery << "\\StringFileInfo\\" << std::hex 
				 << std::setfill('0') << std::setw(4) << static_cast<WORD>(m_translation.langID) 
				 << std::setfill('0') << std::setw(4) << static_cast<WORD>(m_translation.charset) << "\\" << pszKeyName;

		// Wert abfragen
		LPVOID pVal;
		UINT iLenVal;
		if (::VerQueryValueA(m_pVersionInfo,strQuery.str().c_str(), &pVal, &iLenVal))
			sVal = static_cast<LPCSTR>(pVal);
	}
	return sVal;
}

/////////////////////////////////////////////////////////////////////////////
//	CFileVersionInfo::DllGetVersion
//		Get DLL Version by calling DLL's DllGetVersion proc

bool CFileVersionInfo::DllGetVersion(LPCSTR szModulename, DLLVERSIONINFO& dvi)
{
	HINSTANCE hinst = ::LoadLibraryA(szModulename);
	if (!hinst)
		return false;

	// Must use GetProcAddress because the DLL might not implement 
	// DllGetVersion. Depending upon the DLL, the lack of implementation of the 
	// function may be a version marker in itself.
	DLLGETVERSIONPROC pDllGetVersion = reinterpret_cast<DLLGETVERSIONPROC>(::GetProcAddress(hinst, "DllGetVersion"));
	if (!pDllGetVersion)
	{
		// Entladen
		::FreeLibrary(hinst);
		return false;
	}

	// Datenbereich löschen
	memset(&dvi, 0, sizeof(dvi));			 // clear
	dvi.cbSize = sizeof(dvi);				 // set size for Windows
	bool bReturn = SUCCEEDED((*pDllGetVersion)(&dvi));

	// Free again
	::FreeLibrary(hinst);
	return bReturn;
}

/////////////////////////////////////////////////////////////////////////////
