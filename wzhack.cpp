/*
	Hack para o warzone 2100

	Vers�es suportadas:
		* 2.3.9
		* 3.1.5
		* 3.2.3
	
	Caracter�sticas:
		* Energia infinita
		* Easter egg
	
	Descri��o:
		Este programa permite que o jogador possua uma energia sempre acima do valor
		que ele quiser. Com esta ferramenta voc� pode, por exemplo, fazer com que
		nunca a sua energia esteja abaixo de 60000 por mais de 1 minuto.
	
	Aviso:
		Se for compilar no Visual Studio, ative a op��o 'MultiByte'
		
	Autor: 
		Lucas Vieira de Jesus <lucas.engen.cc@gmail.com>
		
	Testado no:
		Microsoft Windows [vers�o 10.0.16299.125] x64
*/

#ifdef UNICODE
#undef UNICODE
#endif

/* Bibliotecas */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <Windows.h>
#include <TlHelp32.h>
#include <ShlObj.h>
#include <shlwapi.h>
#include <assert.h>
#include "wzhack.h"

/* In�cio do c�digo */

/// <summary>Get the version of warzone 2100</summary>
/// <param name="wz_filename">Path of warzone2100.exe file</param>
/// <returns>WZ_323, WZ_315, WZ_239 or WZ_UNKNOWN</returns>
int WzHack_GetWarzoneVersion(const char *wz_filename)
{
	char *buf = NULL;
	DWORD dwHandle, sz;
	HMODULE hVersion_DLL = NULL;
	GFVI fGetFileVersionInfoA = NULL;
	GFVIS fGetFileVersionInfoSizeA = NULL;
	VQV fVerQueryValueA = NULL;
	int major, minor, patch, build;
	int iversion;
	
    hVersion_DLL = LoadLibraryA("version.dll");
	if (hVersion_DLL == NULL)
	{
		WzHack_ShowMessage(CRITICAL, "Failed to load version.dll. Error %lu\n", GetLastError());
		return 1;
	}

	fGetFileVersionInfoA = (GFVI)GetProcAddress(hVersion_DLL, "GetFileVersionInfoA");
	if (fGetFileVersionInfoA == NULL)
	{
		if (hVersion_DLL)
			FreeLibrary(hVersion_DLL);

		WzHack_ShowMessage(CRITICAL, "Failed to get function address on version.dll. Error %lu\n", GetLastError());
		return 2;
	}

	fGetFileVersionInfoSizeA = (GFVIS)GetProcAddress(hVersion_DLL, "GetFileVersionInfoSizeA");
	if (fGetFileVersionInfoSizeA == NULL)
	{
		if (hVersion_DLL)
			FreeLibrary(hVersion_DLL);

		WzHack_ShowMessage(CRITICAL, "Failed to get function address on version.dll. Error %lu\n", GetLastError());
		return 3;
	}

	fVerQueryValueA = (VQV)GetProcAddress(hVersion_DLL, "VerQueryValueA");
	if (fVerQueryValueA == NULL)
	{
		if (hVersion_DLL)
			FreeLibrary(hVersion_DLL);

		WzHack_ShowMessage(CRITICAL, "Failed to get function address on version.dll. Error %lu\n", GetLastError());
		return 4;
	}

	sz = fGetFileVersionInfoSizeA(wz_filename, &dwHandle);
	if (sz == 0)
	{
		if (hVersion_DLL)
			FreeLibrary(hVersion_DLL);
		
		return 5;
	}
	
	buf = (char*)malloc((size_t)sizeof(char) * sz);
	if (!buf)
	{
		if (hVersion_DLL)
			FreeLibrary(hVersion_DLL);
		
		return 6;
	}

	if (!fGetFileVersionInfoA(wz_filename, dwHandle, sz, &buf[0]))
	{
		if (buf)
			free(buf);

		if (hVersion_DLL)
			FreeLibrary(hVersion_DLL);
		
		return 7;
	}

	VS_FIXEDFILEINFO *pvi;
	sz = sizeof(VS_FIXEDFILEINFO);

	if (fVerQueryValueA(&buf[0], "\\", (LPVOID*)&pvi, (unsigned int*)&sz) == FALSE)
	{
		if (buf)
			free(buf);
		
		if (hVersion_DLL)
			FreeLibrary(hVersion_DLL);
		
		return 8;
	}
	
	major = pvi->dwProductVersionMS >> 16;
	minor = pvi->dwFileVersionMS & 0xFFFF;
	patch = pvi->dwFileVersionLS >> 16;
	build = pvi->dwFileVersionLS & 0xFFFF;
	
	if (major == 3 && minor == 2 && patch == 3 && build == 0)
		iversion = WZ_323;
	else if (major == 3 && minor == 1 && patch == 5 && build == 0)
		iversion = WZ_315;
	else if (major == 2 && minor == 3 && patch == 9 && build == 0)
		iversion = WZ_239;
	else
		iversion = WZ_UNKNOWN;

	if (buf)
		free(buf);

	if (hVersion_DLL)
		FreeLibrary(hVersion_DLL);
		
	return iversion;
}

/// <summary>Inject a dll into warzone2100 process</summary>
/// <param name="warzoneHandle">Handle to warzone2100 process</param>
/// <returns>TRUE or FALSE</returns>
BOOL WzHack_InjectDLL(HANDLE warzoneHandle)
{
	LPVOID address = NULL;
	HMODULE hModule = NULL;
	DWORD size = 0;
	LPVOID addr_LoadLib = NULL;
	HANDLE hRemoteThread = NULL;
	const char *dllName = "WzHack.dll";
	char full_dll_path[MAX_PATH];
	char current_path[MAX_PATH];
	DWORD path_len;

	if(warzoneHandle == INVALID_HANDLE_VALUE || warzoneHandle == NULL)
	{
#if defined(_DEBUG) && !defined(_WINDLL)
		WzHack_ShowMessage(DEBUG, "Invalid handle to warzone2100 executable.\n");
#endif
		return FALSE;
	}
	
	if ((path_len = GetCurrentDirectoryA(MAX_PATH * sizeof(char), current_path)) == 0)
	{
		return FALSE;
	}

	if (current_path[path_len - 1] == '\\')
		snprintf(full_dll_path, MAX_PATH * sizeof(char), "%s%s", current_path, dllName);
	else
		snprintf(full_dll_path, MAX_PATH * sizeof(char), "%s\\%s", current_path, dllName);

#ifdef _DEBUG
	WzHack_ShowMessage(DEBUG, "(%s:%d) Injecting: %s\n", __FILE__, __LINE__, full_dll_path);
#endif

	hModule = GetModuleHandleA("kernel32.dll");
	if(hModule == NULL)
		return false;
	
	addr_LoadLib = (LPVOID)GetProcAddress(hModule, "LoadLibraryA");
	if(addr_LoadLib == NULL)
		return FALSE;
	
	size = (DWORD)strlen(full_dll_path);
	address = VirtualAllocEx(warzoneHandle, NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if(address == NULL)
	{
		if(hModule)
			FreeLibrary(hModule);
		
		return FALSE;
	}
	
	if(!WriteProcessMemory(warzoneHandle, address, (const void*)full_dll_path, (SIZE_T)size, NULL))
	{
		if(!VirtualFreeEx(warzoneHandle, address, size, MEM_RELEASE))
			WzHack_ShowMessage(WARNING, "We have a trouble. Virtual memory can't be freed. Error %lu\n", GetLastError());
		
		if(hModule)
			FreeLibrary(hModule);
		
		return FALSE;
	}
	
	hRemoteThread = CreateRemoteThread(warzoneHandle, NULL, NULL, (LPTHREAD_START_ROUTINE)addr_LoadLib, address,0,NULL);
	if(hRemoteThread == NULL)
		return FALSE;

	if (hModule)
		FreeLibrary(hModule);

	if (hRemoteThread)
		CloseHandle(hRemoteThread);

	return TRUE;
}

/// <summary>Returns the start index in WZ_PPO structure to the specified warzone version</summary>
/// <param name="major">Major version of warzone</param>
/// <param name="minor">Minor version of warzone</param>
/// <param name="patch">Patch version of warzone</param>
/// <param name="start">Pointer to receive the start index. Can't be NULL</param>
/// <returns>TRUE or FALSE</returns>
BOOL WzHack_GetWzPpoStartIndex(unsigned major, unsigned minor, unsigned patch, int *start)
{
	int local_start = -1;
	BOOL bOk = FALSE;

	if (start == NULL)
		return FALSE;

	for (int i = 0; i < (sizeof(wz_off) / sizeof(wz_off[0])); i++)
	{
		if (wz_off[i].major == major
			&& wz_off[i].minor == minor
			&& wz_off[i].patch == patch)
		{
			local_start = i;
			bOk = TRUE;
			break;
		}
	}

	if (IsBadWritePtr(start, sizeof(int)))
	{
		return FALSE;
	}

	*start = local_start;

	return bOk;
}

/// <summary>Find the specified process in memory</summary>
/// <param name="nome">Process name</param>
/// <param name="pid">Pointer to receive the PID of process. Can't be NULL</param>
/// <returns>TRUE or FALSE</returns>
BOOL WzHack_FindProcess(const char *nome, DWORD *pid)
{
    struct tagPROCESSENTRY32 *entradas = NULL;
	HANDLE snapHandle = NULL;
	DWORD local_pid = -1;

    entradas = (struct tagPROCESSENTRY32*)malloc(sizeof(struct tagPROCESSENTRY32));
	if (entradas == NULL)
	{
		WzHack_ShowMessage(CRITICAL, "Failed to allocate memory.\n");
        if(pid != NULL)
            *pid = local_pid;

		return FALSE;
	}

    entradas->dwSize = sizeof(struct tagPROCESSENTRY32);
	
	snapHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapHandle == NULL)
	{
		WzHack_ShowMessage(CRITICAL, "Failed to capture the running processes.\n");
        if(pid != NULL)
            *pid = local_pid;
		
		if(entradas)
			free(entradas);
		
		return FALSE;
	}

	if (!Process32First(snapHandle, entradas))
	{
		WzHack_ShowMessage(CRITICAL, "Failed to intialize scan of processes in memory. Error %lu\n", GetLastError());
        if(pid != NULL)
            *pid = local_pid;
		
		if(entradas)
			free(entradas);
		
		return FALSE;
	}

	do
	{
        const char *target = nome;
        const char *current = entradas->szExeFile;

        if (strncmp(target, current, strlen(target)) == 0)
		{
            if(pid != NULL)
                *pid = entradas->th32ProcessID;

			return TRUE;
		}

	} while (Process32Next(snapHandle, entradas));

	if(entradas)
		free(entradas);
	
    SetLastError(ERROR_FILE_NOT_FOUND);

    if(pid != NULL)
        *pid = local_pid;

	return FALSE;
}

/// <summary>Returns the base address of the module loaded by the specified executable</summary>
/// <param name="exeName">Executable name. Sample: "foo.exe"</param>
/// <param name="moduleName">Name of the Module. Sample: "foo.dll"</param>
/// <returns>TRUE or FALSE</returns>
DWORD WzHack_GetModuleAddress(const char *exeName, const char *moduleName)
{
	DWORD local_pid = -1;
	BOOL encontrou = TRUE;
	DWORD endereco_base = -1;
    BOOL falhou = FALSE;

    (void)moduleName;
	// Primeiro verificamos se o execut�vel est� carregado na RAM
	encontrou = WzHack_FindProcess(exeName, &local_pid);
	if (!encontrou)
	{
		// N�o podemos prosseguir. N�o temos o pid do processo
#ifdef _DEBUG
		WzHack_ShowMessage(SUCCESS, "(%s:%d) Failed to find %s. Error %lu\n", __FILE__, __LINE__, exeName, GetLastError());
#endif
		return FALSE;
	}

	// Agora tiramos um snapshot de todos os modulos carregados pelo execut�vel em quest�o...
	MODULEENTRY32 *modulo = NULL;
	HANDLE msnapHandle = NULL;

    modulo = (struct tagMODULEENTRY32*)malloc(sizeof(struct tagMODULEENTRY32));
	if (modulo == NULL)
	{
		WzHack_ShowMessage(CRITICAL, "Failed to allocate memory. Error %lu\n", GetLastError());
		return FALSE;
	}

	msnapHandle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, local_pid);

	if (msnapHandle == NULL)
	{
		WzHack_ShowMessage(CRITICAL, "Failed to list modules loaded by %s (PID: %lu)\n", exeName, local_pid);
		return FALSE;
	}
	
	if (!Module32First(msnapHandle, modulo))
	{
		WzHack_ShowMessage(CRITICAL, "Failed to intialize listing of modules loaded by %s (PID %lu). Error %lu\n", exeName, local_pid,
			GetLastError());
		return FALSE;
	}

	do
	{
		const char *nome_processo_alvo = exeName;
		const char *nome_processo_atual = modulo->szModule;

		if (strncmp(nome_processo_alvo, nome_processo_atual, strlen(nome_processo_alvo)) == 0)
		{
			endereco_base = (DWORD)modulo->modBaseAddr;
			falhou = FALSE;
			break;
		}

	} while (Module32Next(msnapHandle, modulo));

#ifdef _DEBUG
	static int v = 0;

	if (v == 0) 
	{
		WzHack_ShowMessage(DEBUG, "(%s:%d) WzHack_GetModuleAddress failed? -> %s\n", __FILE__, __LINE__, (falhou) ? "yes" : "no");
		WzHack_ShowMessage(DEBUG, "(%s:%d) Base address: %#x\n", __FILE__, __LINE__, endereco_base);
		v++;
	}
#endif

	if (modulo)
		free(modulo);

	if (msnapHandle)
		CloseHandle(msnapHandle);

	return endereco_base;
}

char *WzHack_GetSubstring(char *str, char k)
{
	char *s = str;
	
	while(*s)
	{
		if(*s == k)
			break;
		
		s++;
	}
	
	s++;
	
	return s;
}

BOOL WzHack_GetPlayerPower(unsigned player_id, HANDLE warzoneHandle, DWORD *power, int wz_version)
{
	BOOL bResult = FALSE;
	DWORD nread;
	WARZONE_BASE wz;
	DWORD local_power;
	DWORD local_offset = 0;
	int start_index;
	BOOL bOk;

	assert(warzoneHandle != NULL);

	if(power == NULL)
		return FALSE;
	
	wz.base = WzHack_GetModuleAddress("warzone2100.exe", "warzone2100.exe");
    if (wz.base == (DWORD)-1)
	{
		if(warzoneHandle)
			CloseHandle(warzoneHandle);
			
		*power = -1;
		return 1;
	}	

	if (wz_version == WZ_239)
	{
		bOk = WzHack_GetWzPpoStartIndex(2, 3, 9, &start_index);
	}
	else if (wz_version == WZ_315)
	{
		bOk = WzHack_GetWzPpoStartIndex(3, 1, 5, &start_index);
	}
	else if (wz_version == WZ_323)
	{
		bOk = WzHack_GetWzPpoStartIndex(3, 2, 3, &start_index);
	}
	else
	{
		WzHack_ShowMessage(CRITICAL, "Unsupported warzone version detected '%d'. The program cannot continue\n", wz_version);
		exit(EXIT_FAILURE);
	}

	if (bOk)
	{
		if (player_id < 0 || player_id > 10)
		{
			WzHack_ShowMessage(CRITICAL, "Bad player id: %d\n", player_id);
		}
		else
		{
			local_offset = wz_off[start_index + player_id].power_offset;
			bResult = ReadProcessMemory(warzoneHandle, (void*)(wz.base + local_offset), &local_power, sizeof(DWORD), &nread);
#ifdef _DEBUG
			WzHack_ShowMessage(DEBUG, "(%s:%d) Energy of player %u -> %u\n", __FILE__, __LINE__, player_id, local_power);
			WzHack_ShowMessage(DEBUG, "(%s:%d) Offset: %#x\n", __FILE__, __LINE__, wz_off[start_index + player_id].offset);
			WzHack_ShowMessage(DEBUG, "(%s:%d) Position on vector: %d\n", __FILE__, __LINE__, start_index + player_id);
			WzHack_ShowMessage(DEBUG, "(%s:%d) Address: %#x\n", __FILE__, __LINE__, wz.base + local_offset);
#endif
		}
	}
	else
	{
		WzHack_ShowMessage(CRITICAL, "BUG: Internal program error. Source %s line %d\n", __FILE__, __LINE__);
		exit(EXIT_FAILURE);
	}

	if (bResult)
		*power = local_power;
	else
		*power = -1;

	return bResult;	
}

BOOL WzHack_SetPlayerPower(unsigned player_id, HANDLE warzoneHandle, DWORD power, int wz_version)
{
	BOOL bResult = FALSE;
	DWORD nwrite;
	WARZONE_BASE wz;
	DWORD local_offset = 0;
	int start_index;
	BOOL bOk;

	assert(warzoneHandle != NULL);

	wz.base = WzHack_GetModuleAddress("warzone2100.exe", "warzone2100.exe");
	if (wz.base == -1)
	{
		if(warzoneHandle)
			CloseHandle(warzoneHandle);
			
		return 1;
	}	
	
	switch(wz_version)
	{
		case WZ_239:
			bOk = WzHack_GetWzPpoStartIndex(2, 3, 9, &start_index);
		break;
		
		case WZ_315:
			bOk = WzHack_GetWzPpoStartIndex(3, 1, 5, &start_index);
		break;
		
		case WZ_323:
			bOk = WzHack_GetWzPpoStartIndex(3, 2, 3, &start_index);
		break;
	}

	if (bOk)
	{
		if (player_id < 0 || player_id > 10)
		{
			WzHack_ShowMessage(CRITICAL, "Bad player id: %d\n", player_id);
		}
		else
		{
#ifdef _DEBUG
			WzHack_ShowMessage(DEBUG, "(%s:%d) Setting energy of player %u to %u\n", __FILE__, __LINE__, player_id, power);
			WzHack_ShowMessage(DEBUG, "(%s:%d) Offset: %#x\n", __FILE__, __LINE__, wz_off[start_index + player_id].offset);
			WzHack_ShowMessage(DEBUG, "(%s:%d) Position on vector: %d\n", __FILE__, __LINE__, start_index + player_id);
			WzHack_ShowMessage(DEBUG, "(%s:%d) Address: %#x\n", __FILE__, __LINE__, wz.base + local_offset);
#endif
			local_offset = wz_off[start_index + player_id].power_offset;
			bResult = WriteProcessMemory(warzoneHandle, (void*)(wz.base + local_offset), (const void*)&power, sizeof(DWORD), &nwrite);
		}
	}
	else
	{
		WzHack_ShowMessage(CRITICAL, "BUG: Internal program error. Source %s line %d\n", __FILE__, __LINE__);
		exit(EXIT_FAILURE);
	}

	return bResult;
}

int WzHack_ShowMessage(types t, const char *string, ...)
{
	HANDLE hConsole = NULL;
	va_list list;
	char mem_buffer[256];
	int len = 0;
	BOOL colors_enabled = TRUE;

	va_start(list, string);
	len = vsnprintf(mem_buffer, sizeof(char) * 256, string, list);
	va_end(list);

	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!hConsole)
		colors_enabled = FALSE;

	if (!colors_enabled)
	{
		len = fprintf(stdout, mem_buffer);
	}
	else 
	{
		// Pegar a cor anterior do console
		CONSOLE_SCREEN_BUFFER_INFO *csbi = NULL;
		csbi = (CONSOLE_SCREEN_BUFFER_INFO*)malloc(sizeof(CONSOLE_SCREEN_BUFFER_INFO));
		if (csbi == NULL)
		{
			len = fprintf(stdout, mem_buffer);
		}
		else 
		{
			if (!GetConsoleScreenBufferInfo(hConsole, csbi))
			{
				len = len = fprintf(stdout, mem_buffer);
				if (csbi)
					free(csbi);
			}
			else 
			{
				if (t == WARNING)
				{
					SetConsoleTextAttribute(hConsole, (WORD)YELLOW);
					fprintf(stdout, "[!] ");
					FlushConsoleInputBuffer(hConsole);
					SetConsoleTextAttribute(hConsole, csbi->wAttributes);
					len = fprintf(stdout, mem_buffer);
				}
				else if (t == SUCCESS)
				{
					SetConsoleTextAttribute(hConsole, (WORD)GREEN);
					fprintf(stdout, "[+] ");
					FlushConsoleInputBuffer(hConsole);
					SetConsoleTextAttribute(hConsole, csbi->wAttributes);
					len = fprintf(stdout, mem_buffer);
				}
				else if(t == CRITICAL)
				{
					SetConsoleTextAttribute(hConsole, (WORD)RED);
					fprintf(stdout, "[!] ");
					FlushConsoleInputBuffer(hConsole);
					SetConsoleTextAttribute(hConsole, csbi->wAttributes);
					len = fprintf(stdout, mem_buffer);
				}
				else if(t == INFO)
				{
					SetConsoleTextAttribute(hConsole, (WORD)CYAN);
					fprintf(stdout, "[*] ");
					FlushConsoleInputBuffer(hConsole);
					SetConsoleTextAttribute(hConsole, csbi->wAttributes);
					len = fprintf(stdout, mem_buffer);
				}
				else if(t == DEFAULT)
				{
					SetConsoleTextAttribute(hConsole, (WORD)WHITE);
					fprintf(stdout, "[*] ");
					FlushConsoleInputBuffer(hConsole);
					SetConsoleTextAttribute(hConsole, csbi->wAttributes);
					len = fprintf(stdout, mem_buffer);
				}
#ifdef _DEBUG
				else if (t == DEBUG)
				{
					SetConsoleTextAttribute(hConsole, (WORD)YELLOW);
					fprintf(stdout, "[debug] ");
					FlushConsoleInputBuffer(hConsole);
					SetConsoleTextAttribute(hConsole, csbi->wAttributes);
					len = fprintf(stdout, mem_buffer);					
				}
#endif
				else 
				{
                    fprintf(stdout, "Fatal error on %s line %d: invalid param received.\n", __FILE__, __LINE__);
				}
			}
		}
	}

	return len;
}

/* Este c�digo n�o foi feito pra ser facilmente compreens�vel kkkk */

void WzHack_RunEasterEgg(HANDLE w, int a, unsigned me)
{
	BOOL b;
	int s;
	DWORD o;
	DWORD p;
	int v;
	DWORD npp = 0;

	assert(w != NULL);
	
	if (a == WZ_239)
	{
		b = WzHack_GetWzPpoStartIndex(2, 3, 9, &s);
		v = WZ_239;
	}
	else if (a == WZ_315)
	{
		b = WzHack_GetWzPpoStartIndex(3, 1, 5, &s);
		v = WZ_315;
	}
	else if (a == WZ_323)
	{
		b = WzHack_GetWzPpoStartIndex(3, 2, 3, &s);
		v = WZ_323;
	}

	for (unsigned i = 0; i < MAX_PLAYERS; i++)
	{
		if (i == me)
			continue;

        /* TODO: Include code to check the number of units of a player */

		o = wz_off[i + s].power_offset;
		if (WzHack_GetPlayerPower(i, w, &p, v)) 
		{
			if (p > 0)
			{
				WzHack_SetPlayerPower(i, w, npp, v);
			}
		}
	}
}
