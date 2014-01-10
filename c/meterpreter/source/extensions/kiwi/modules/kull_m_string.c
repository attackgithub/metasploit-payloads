/*	Benjamin DELPY `gentilkiwi`
	http://blog.gentilkiwi.com
	benjamin@gentilkiwi.com
	Licence : http://creativecommons.org/licenses/by/3.0/fr/
*/
#include "kull_m_string.h"

VOID (WINAPI * RtlInitString)(OUT PSTRING DestinationString, IN PCSZ SourceString);
VOID (WINAPI * RtlInitUnicodeString)(OUT PUNICODE_STRING DestinationString, IN PCWSTR SourceString);
NTSTATUS (WINAPI * RtlAnsiStringToUnicodeString)(OUT PUNICODE_STRING DestinationString, IN PCANSI_STRING SourceString, IN BOOLEAN AllocateDestinationString);
NTSTATUS (WINAPI * RtlUnicodeStringToAnsiString)(OUT PANSI_STRING DestinationString, IN PCUNICODE_STRING SourceString, IN BOOLEAN AllocateDestinationString);
BOOLEAN (WINAPI * RtlEqualString)(IN const STRING *String1, IN const STRING *String2, IN BOOLEAN CaseInSensitive);
BOOLEAN (WINAPI * RtlEqualUnicodeString)(IN PCUNICODE_STRING String1, IN PCUNICODE_STRING String2, IN BOOLEAN CaseInSensitive);
LONG (WINAPI * RtlCompareUnicodeString)(IN PCUNICODE_STRING String1, IN PCUNICODE_STRING String2, IN BOOLEAN CaseInSensitive);
LONG (WINAPI * RtlCompareString)(IN const STRING *String1, IN const STRING *String2, IN BOOLEAN CaseInSensitive);
VOID (WINAPI * RtlFreeAnsiString)(IN PANSI_STRING AnsiString);
VOID (WINAPI * RtlFreeUnicodeString)(IN PUNICODE_STRING UnicodeString);
NTSTATUS (WINAPI * RtlStringFromGUID)(IN LPCGUID Guid, PUNICODE_STRING UnicodeString);
NTSTATUS (NTAPI * RtlValidateUnicodeString)(IN ULONG Flags, IN PCUNICODE_STRING UnicodeString);

VOID kull_m_string_initialise()
{
	HMODULE ntDll = GetModuleHandleA("ntdll");

	RtlInitString = (VOID (WINAPI *)(OUT PSTRING DestinationString, IN PCSZ SourceString))GetProcAddress(ntDll, "RtlInitString");
	RtlInitUnicodeString = (VOID (WINAPI *)(OUT PUNICODE_STRING DestinationString, IN PCWSTR SourceString))GetProcAddress(ntDll, "RtlInitUnicodeString");
	RtlAnsiStringToUnicodeString = (NTSTATUS (WINAPI *)(OUT PUNICODE_STRING DestinationString, IN PCANSI_STRING SourceString, IN BOOLEAN AllocateDestinationString))GetProcAddress(ntDll, "RtlAnsiStringToUnicodeString");
	RtlUnicodeStringToAnsiString = (NTSTATUS (WINAPI *)(OUT PANSI_STRING DestinationString, IN PCUNICODE_STRING SourceString, IN BOOLEAN AllocateDestinationString))GetProcAddress(ntDll, "RtlUnicodeStringToAnsiString");
	RtlEqualString = (BOOLEAN (WINAPI *)(IN const STRING *String1, IN const STRING *String2, IN BOOLEAN CaseInSensitive))GetProcAddress(ntDll, "RtlEqualString");
	RtlEqualUnicodeString = (BOOLEAN (WINAPI *)(IN PCUNICODE_STRING String1, IN PCUNICODE_STRING String2, IN BOOLEAN CaseInSensitive))GetProcAddress(ntDll, "RtlEqualUnicodeString");
	RtlCompareUnicodeString = (LONG (WINAPI *)(IN PCUNICODE_STRING String1, IN PCUNICODE_STRING String2, IN BOOLEAN CaseInSensitive))GetProcAddress(ntDll, "RtlCompareUnicodeString");
	RtlFreeUnicodeString = (VOID (WINAPI *)(IN PUNICODE_STRING UnicodeString))GetProcAddress(ntDll, "RtlFreeUnicodeString");
	RtlCompareString = (LONG (WINAPI *)(IN const STRING *String1, IN const STRING *String2, IN BOOLEAN CaseInSensitive))GetProcAddress(ntDll, "RtlCompareString");
	RtlFreeAnsiString = (VOID (WINAPI *)(IN PANSI_STRING AnsiString))GetProcAddress(ntDll, "RtlFreeAnsiString");
	RtlStringFromGUID = (NTSTATUS (WINAPI *)(IN LPCGUID Guid, PUNICODE_STRING UnicodeString))GetProcAddress(ntDll, "RtlStringFromGUID");
	RtlValidateUnicodeString = (NTSTATUS (NTAPI *)(IN ULONG Flags, IN PCUNICODE_STRING UnicodeString))GetProcAddress(ntDll, "RtlValidateUnicodeString");
}

BOOL kull_m_string_suspectUnicodeStringStructure(IN PUNICODE_STRING pUnicodeString)
{
	return (
		pUnicodeString->Length &&
		!((pUnicodeString->Length & 1) || (pUnicodeString->MaximumLength & 1)) &&
		(pUnicodeString->Length < sizeof(wchar_t)*0xff) &&
		(pUnicodeString->Length <= pUnicodeString->MaximumLength) &&
		((pUnicodeString->MaximumLength - pUnicodeString->Length) < 4*sizeof(wchar_t)) &&
		pUnicodeString->Buffer
		);
}

BOOL kull_m_string_suspectUnicodeString(IN PUNICODE_STRING pUnicodeString)
{
	int unicodeTestFlags = IS_TEXT_UNICODE_ODD_LENGTH | IS_TEXT_UNICODE_STATISTICS;
	return IsTextUnicode(pUnicodeString->Buffer, pUnicodeString->Length, &unicodeTestFlags);
}

BOOL kull_m_string_getUnicodeString(IN PUNICODE_STRING string, IN PKULL_M_MEMORY_HANDLE source)
{
	BOOL status = FALSE;
	KULL_M_MEMORY_HANDLE hOwn = {KULL_M_MEMORY_TYPE_OWN, NULL};
	KULL_M_MEMORY_ADDRESS aDestin = {NULL, &hOwn};
	KULL_M_MEMORY_ADDRESS aSource = {string->Buffer, source};
	
	string->Buffer = NULL;
	if(aSource.address && string->MaximumLength)
	{
		if(aDestin.address = LocalAlloc(LPTR, string->MaximumLength))
		{
			string->Buffer = (PWSTR) aDestin.address;
			status = kull_m_memory_copy(&aDestin, &aSource, string->MaximumLength);
		}
	}
	return status;
}

void kull_m_string_freeUnicodeStringBuffer(PUNICODE_STRING pString)
{
	if(pString->Buffer)
		LocalFree(pString->Buffer);
}
/*
VOID kull_m_string_outputHighUnicodeString(PLSA_UNICODE_STRING pString)
{
	DWORD dwSize;
	wchar_t * ptr = NULL;
	if(pString)
	{
		ptr = pString->Buffer;
		dwSize =  pString->Length / sizeof(wchar_t);
	}
	kull_m_string_outputHighWideStringWithLen(ptr, dwSize);
}

VOID kull_m_string_outputHighWideString(wchar_t * pString)
{
	DWORD dwSize;
	if(pString)
		dwSize =  (DWORD) wcslen(pString);
	kull_m_string_outputHighWideStringWithLen(pString, dwSize);
}

VOID kull_m_string_outputHighWideStringWithLen(wchar_t * pString, DWORD dwSize)
{
	//DWORD dwhConWritten;
	//HANDLE hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if(!pString)
	{
		pString = L"(null)";
		dwSize = 6;
	}
	kprintf(L"%.*s", dwSize, pString);
	//WriteConsole(hConOut, pString, dwSize, &dwhConWritten, NULL); 
}
*/
wchar_t * kull_m_string_qad_ansi_to_unicode(const char * ansi)
{
	wchar_t * buffer = NULL;
	if(ansi)
		buffer = kull_m_string_qad_ansi_c_to_unicode(ansi, strlen(ansi));
	return buffer;
}

wchar_t * kull_m_string_qad_ansi_c_to_unicode(const char * ansi, SIZE_T szStr)
{
	wchar_t * buffer = NULL;
	SIZE_T i;

	if(ansi && szStr)
		if(buffer = (wchar_t *) LocalAlloc(LPTR, (szStr + 1) * sizeof(wchar_t)))
			for(i = 0; i < szStr; i++)
				buffer[i] = ansi[i];
	return buffer;
}

PCWCHAR WPRINTF_TYPES[] =
{
	L"%02x",		// WPRINTF_HEX_SHORT
	L"%02x ",		// WPRINTF_HEX_SPACE
	L"0x%02x, ",	// WPRINTF_HEX_C
	L"\\x%02x",		// WPRINTF_HEX_PYTHON
};

void kull_m_string_wprintf_hex(LPCVOID lpData, DWORD cbData, DWORD flags)
{
	DWORD i, sep;
	PCWCHAR pType = WPRINTF_TYPES[flags & 0x0000000f];
	sep = flags >> 16;
	for (i = 0; i < cbData; i++)
	{
		kprintf(pType, ((LPCBYTE)lpData)[i]);
		if(sep && !((i+1) % sep))
			kprintf(L"\n");
	}
}

void kull_m_string_displayFileTime(IN PFILETIME pFileTime)
{
	SYSTEMTIME st;
	wchar_t buffer[0xff];
	if(pFileTime)
	{
		if(FileTimeToSystemTime(pFileTime, &st ))
		{
			if(GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, buffer, sizeof(buffer) / sizeof(wchar_t)))
			{
				kprintf(L"%s ", buffer);
				if(GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, buffer, sizeof(buffer) / sizeof(wchar_t)))
					kprintf(L"%s", buffer);
			}
		}
	}
}

void kull_m_string_displayLocalFileTime(IN PFILETIME pFileTime)
{
	FILETIME ft;
	if(pFileTime)
		if(FileTimeToLocalFileTime(pFileTime, &ft))
			kull_m_string_displayFileTime(&ft);
}

void kull_m_string_displayGUID(IN LPCGUID pGuid)
{
	UNICODE_STRING uString;
	if (NT_SUCCESS(RtlStringFromGUID(pGuid, &uString)))
	{
		kprintf(L"%wZ", &uString);
		RtlFreeUnicodeString(&uString);
	}
}

BOOL kull_m_string_args_byName(const int argc, const wchar_t * argv[], const wchar_t * name, const wchar_t ** theArgs, const wchar_t * defaultValue)
{
	BOOL result = FALSE;
	const wchar_t *pArgName, *pSeparator;
	SIZE_T argLen, nameLen = wcslen(name);
	int i;
	for(i = 0; i < argc; i++)
	{
		if((wcslen(argv[i]) > 1) && ((argv[i][0] == L'/') || (argv[i][0] == L'-')))
		{
			pArgName = argv[i] + 1;
			if(!(pSeparator = wcschr(argv[i], L':')))
				pSeparator = wcschr(argv[i], L'=');

			argLen =  (pSeparator) ? (pSeparator - pArgName) : wcslen(pArgName);
			if((argLen == nameLen) && _wcsnicmp(name, pArgName, argLen) == 0)
			{
				if(theArgs)
				{
					if(pSeparator)
					{
						*theArgs = pSeparator + 1;
						result = *theArgs[0] != L'\0';
					}
				}
				else
					result = TRUE;
				break;
			}
		}
	}

	if(!result && theArgs && defaultValue)
	{
		*theArgs = defaultValue;
		result = TRUE;
	}

	return result;
}