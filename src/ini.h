#pragma once
#include <string>
#include <windows.h>

class IniFile
{
public:
	explicit IniFile(std::wstring path) : m_path(std::move(path)) {}

	std::wstring GetString(LPCWSTR sec, LPCWSTR key,
		LPCWSTR def = L"") const
	{
		wchar_t buf[1024] = { 0 };
		GetPrivateProfileString(sec, key, def, buf, _countof(buf), m_path.c_str());
		return buf;
	}

	int GetInt(LPCWSTR sec, LPCWSTR key, int def = 0) const
	{
		return GetPrivateProfileInt(sec, key, def, m_path.c_str());
	}

	bool GetBool(LPCWSTR sec, LPCWSTR key, bool def = false) const
	{
		std::wstring s = GetString(sec, key, def ? L"1" : L"0");
		return s == L"1" || _wcsicmp(s.c_str(), L"true") == 0;
	}

	double GetDouble(LPCWSTR sec, LPCWSTR key, double def = 0.0) const
	{
		std::wstring s = GetString(sec, key, L"");
		return s.empty() ? def : _wtof(s.c_str());
	}

	void SetString(LPCWSTR sec, LPCWSTR key, const std::wstring& v)
	{
		WritePrivateProfileString(sec, key, v.c_str(), m_path.c_str());
	}

	void SetInt(LPCWSTR sec, LPCWSTR key, int v)
	{
		wchar_t buf[32];
		StringCchPrintf(buf, _countof(buf), L"%d", v);
		WritePrivateProfileString(sec, key, buf, m_path.c_str());
	}

	void SetBool(LPCWSTR sec, LPCWSTR key, bool v)
	{
		WritePrivateProfileString(sec, key, v ? L"1" : L"0", m_path.c_str());
	}

	void SetDouble(LPCWSTR sec, LPCWSTR key, double v)
	{
		wchar_t buf[64];
		StringCchPrintf(buf, _countof(buf), L"%.6f", v);
		WritePrivateProfileString(sec, key, buf, m_path.c_str());
	}

private:
	std::wstring m_path;
};

extern IniFile ini;