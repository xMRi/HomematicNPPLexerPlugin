#pragma once

#include <shlwapi.h>
#include <string>
#include <sstream>

/////////////////////////////////////////////////////////////////////////////
//	CFileVersionInfo

class CFileVersionInfo : public VS_FIXEDFILEINFO
{
public:
	CFileVersionInfo();
	virtual ~CFileVersionInfo();

	bool GetFileVersionInfo(HMODULE hModule=NULL);
	bool GetFileVersionInfo(LPCSTR modulename);
	std::string	GetValue(LPCSTR lpKeyName) const;
	static bool DllGetVersion(LPCSTR modulename, DLLVERSIONINFO& dvi);

	bool IsValid() const
	{
		return m_pVersionInfo!=NULL;
	}
	std::string GetFileVersion() const
	{
		return GetValue("FileVersion");
	}
	std::string GetProductName() const
	{
		return GetValue("ProductName");
	}
	std::string GetFileDescription() const
	{
		return GetValue("FileDescription");
	}
	std::string GetLegalCopyright() const
	{
		return GetValue("LegalCopyright");
	}
protected:
	void Clear();

protected:
	BYTE* m_pVersionInfo;	// all version info

	struct TRANSLATION 
	{
		WORD langID;			// language ID
		WORD charset;			// character set (code page)
	} 
	m_translation;
};

/////////////////////////////////////////////////////////////////////////////
