// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 THIONG
// Part of Imperivm Resurrected. GPL-3.0-or-later license with an exception for Steamworks: see LICENSE and ADDITIONAL-PERMISSION.md.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <tlhelp32.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>

#include "miniz.h"

namespace {

const wchar_t kAppId[] = L"752580";
const uint64_t kSteamExeSize = 4456448;
const char kTrailerMagic[8] = {'I', 'R', 'S', 'E', 'T', 'U', 'P', '1'};
const wchar_t kLayersKey[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers";
const wchar_t kLayersKeyMachine[] = L"HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers";

enum Lang { kEs, kEn, kIt };
Lang g_lang = kEn;

struct Text {
	const wchar_t *es, *en, *it;
};

const wchar_t *T(const Text &t) { return g_lang == kEs ? t.es : g_lang == kIt ? t.it : t.en; }

const Text kTitle = {L"Imperivm Resurrected: instalación", L"Imperivm Resurrected setup", L"Installazione di Imperivm Resurrected"};
const Text kIntro = {L"Este programa instala el mod en tu Imperivm III HD de Steam y deja el juego listo para arrancar: quita los modos de compatibilidad de Windows, aparta restos de otros mods y repara la configuración de pantalla. No toca tus partidas, mapas ni mods.",
	L"This program installs the mod into your Steam copy of Imperivm III HD and leaves the game ready to start: it removes Windows compatibility modes, moves leftovers of other mods aside and repairs the display settings. It does not touch your saves, maps or mods.",
	L"Questo programma installa la mod nel tuo Imperivm III HD di Steam e lascia il gioco pronto all'avvio: rimuove le modalità compatibilità di Windows, mette da parte i resti di altre mod e ripara le impostazioni dello schermo. Non tocca salvataggi, mappe o mod."};
const Text kFolderLabel = {L"Carpeta del juego:", L"Game folder:", L"Cartella del gioco:"};
const Text kChange = {L"Cambiar…", L"Change…", L"Cambia…"};
const Text kInstall = {L"Instalar", L"Install", L"Installa"};
const Text kRepair = {L"Reparar", L"Repair", L"Ripara"};
const Text kUninstall = {L"Desinstalar", L"Uninstall", L"Disinstalla"};
const Text kPlay = {L"Abrir el juego", L"Open the game", L"Apri il gioco"};
const Text kClose = {L"Cerrar", L"Close", L"Chiudi"};
const Text kNotFound = {L"No he encontrado el juego en Steam. Pulsa «Cambiar…» y elige la carpeta donde está gbr.exe.",
	L"I could not find the game in Steam. Press \"Change…\" and choose the folder that contains gbr.exe.",
	L"Non ho trovato il gioco in Steam. Premi «Cambia…» e scegli la cartella che contiene gbr.exe."};
const Text kReadyInstall = {L"Juego encontrado. Pulsa «Instalar».", L"Game found. Press \"Install\".", L"Gioco trovato. Premi «Installa»."};
const Text kReadyRepair = {L"El mod ya está instalado. «Reparar» lo actualiza a esta versión y vuelve a dejar el juego listo; «Desinstalar» lo quita.",
	L"The mod is already installed. \"Repair\" updates it to this version and leaves the game ready again; \"Uninstall\" removes it.",
	L"La mod è già installata. «Ripara» la aggiorna a questa versione e rimette a posto il gioco; «Disinstalla» la rimuove."};
const Text kPickFolder = {L"Elige la carpeta del juego (la que contiene gbr.exe)", L"Choose the game folder (the one with gbr.exe)", L"Scegli la cartella del gioco (quella con gbr.exe)"};
const Text kNoGbr = {L"Esa carpeta no tiene gbr.exe.", L"That folder has no gbr.exe.", L"Quella cartella non contiene gbr.exe."};
const Text kGameOpen = {L"El juego está abierto. Ciérralo y pulsa Aceptar para seguir.", L"The game is open. Close it and press OK to continue.", L"Il gioco è aperto. Chiudilo e premi OK per continuare."};
const Text kStillOpen = {L"El juego sigue abierto: no se ha cambiado nada.", L"The game is still open: nothing was changed.", L"Il gioco è ancora aperto: non è stato cambiato nulla."};
const Text kDone = {L"Listo. Abre el juego desde Steam (o pulsa «Abrir el juego»).", L"Done. Open the game from Steam (or press \"Open the game\").", L"Fatto. Apri il gioco da Steam (o premi «Apri il gioco»)."};
const Text kUninstalled = {L"El mod se ha quitado. El juego vuelve a ser el original.", L"The mod was removed. The game is the original again.", L"La mod è stata rimossa. Il gioco torna a essere l'originale."};
const Text kFailed = {L"Algo ha fallado. Mira los detalles de arriba.", L"Something failed. See the details above.", L"Qualcosa non è andato. Guarda i dettagli qui sopra."};
const Text kBadPayload = {L"Este instalador está dañado o incompleto. Descárgalo de nuevo.", L"This installer is damaged or incomplete. Download it again.", L"Questo installatore è danneggiato o incompleto. Scaricalo di nuovo."};
const Text kConfirmUninstall = {L"¿Quitar el mod? Se borran winmm.dll, steam_api.dll y la carpeta imperivm-resurrected (a la papelera). Tus partidas, mods y ajustes se quedan.",
	L"Remove the mod? winmm.dll, steam_api.dll and the imperivm-resurrected folder go to the recycle bin. Your saves, mods and settings stay.",
	L"Rimuovere la mod? winmm.dll, steam_api.dll e la cartella imperivm-resurrected vanno nel cestino. Salvataggi, mod e impostazioni restano."};

std::wstring g_report;
HWND g_wnd = nullptr, g_log = nullptr, g_folder = nullptr, g_install = nullptr, g_uninstall = nullptr, g_play = nullptr;
std::wstring g_game;
std::vector<unsigned char> g_payload;
std::wstring g_version;
bool g_elevated = false;

void Say(const std::wstring &line)
{
	g_report += line + L"\r\n";
	if (g_log) {
		SetWindowTextW(g_log, g_report.c_str());
		SendMessageW(g_log, EM_SETSEL, g_report.size(), g_report.size());
		SendMessageW(g_log, EM_SCROLLCARET, 0, 0);
		UpdateWindow(g_log);
	}
}

std::wstring Say3(const wchar_t *es, const wchar_t *en, const wchar_t *it) { return g_lang == kEs ? es : g_lang == kIt ? it : en; }

std::wstring Widen(const std::string &s)
{
	if (s.empty()) return L"";
	const int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
	std::wstring w(n > 0 ? n - 1 : 0, L'\0');
	if (n > 1) MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], n);
	return w;
}

std::string Narrow(const std::wstring &w)
{
	if (w.empty()) return "";
	const int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string s(n > 0 ? n - 1 : 0, '\0');
	if (n > 1) WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &s[0], n, nullptr, nullptr);
	return s;
}

bool Exists(const std::wstring &p) { return GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES; }
bool IsDir(const std::wstring &p)
{
	const DWORD a = GetFileAttributesW(p.c_str());
	return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

uint64_t FileSize(const std::wstring &p)
{
	WIN32_FILE_ATTRIBUTE_DATA d;
	if (!GetFileAttributesExW(p.c_str(), GetFileExInfoStandard, &d)) return 0;
	return (static_cast<uint64_t>(d.nFileSizeHigh) << 32) | d.nFileSizeLow;
}

void MakeDirs(const std::wstring &dir)
{
	if (dir.empty() || IsDir(dir)) return;
	const size_t slash = dir.find_last_of(L"\\/");
	if (slash != std::wstring::npos) MakeDirs(dir.substr(0, slash));
	CreateDirectoryW(dir.c_str(), nullptr);
}

std::wstring Lower(std::wstring s)
{
	for (wchar_t &c : s) c = static_cast<wchar_t>(towlower(c));
	return s;
}

bool ReadAll(const std::wstring &path, std::string &out)
{
	HANDLE f = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	if (f == INVALID_HANDLE_VALUE) return false;
	LARGE_INTEGER size;
	GetFileSizeEx(f, &size);
	out.resize(static_cast<size_t>(size.QuadPart));
	DWORD got = 0;
	const BOOL ok = out.empty() || ReadFile(f, &out[0], static_cast<DWORD>(out.size()), &got, nullptr);
	CloseHandle(f);
	return ok && got == out.size();
}

bool Sha256(const unsigned char *data, size_t size, unsigned char out[32])
{
	BCRYPT_ALG_HANDLE alg = nullptr;
	if (BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0) return false;
	const bool ok = BCryptHash(alg, nullptr, 0, const_cast<PUCHAR>(data), static_cast<ULONG>(size), out, 32) == 0;
	BCryptCloseAlgorithmProvider(alg, 0);
	return ok;
}

bool LoadPayload()
{
	wchar_t self[MAX_PATH];
	GetModuleFileNameW(nullptr, self, MAX_PATH);
	std::string exe;
	if (!ReadAll(self, exe) || exe.size() < 8 + 8 + 32 + 32) return false;
	const char *end = exe.data() + exe.size();
	const char *trailer = end - (8 + 8 + 32 + 32);
	if (memcmp(trailer, kTrailerMagic, 8) != 0) return false;
	uint64_t size = 0;
	memcpy(&size, trailer + 8, 8);
	if (size == 0 || size > exe.size() - 80) return false;
	const unsigned char *zip = reinterpret_cast<const unsigned char *>(trailer) - size;
	unsigned char hash[32];
	if (!Sha256(zip, static_cast<size_t>(size), hash) || memcmp(hash, trailer + 16, 32) != 0) return false;
	g_payload.assign(zip, zip + size);
	char version[33] = {};
	memcpy(version, trailer + 48, 32);
	g_version = Widen(version);
	return true;
}

bool ProcessRunning(const wchar_t *name)
{
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snap == INVALID_HANDLE_VALUE) return false;
	PROCESSENTRY32W pe = {};
	pe.dwSize = sizeof(pe);
	bool found = false;
	for (BOOL ok = Process32FirstW(snap, &pe); ok && !found; ok = Process32NextW(snap, &pe)) found = _wcsicmp(pe.szExeFile, name) == 0;
	CloseHandle(snap);
	return found;
}

std::wstring RegString(HKEY root, const wchar_t *key, const wchar_t *value, REGSAM view = 0)
{
	wchar_t buf[1024] = {};
	DWORD size = sizeof(buf), type = 0;
	HKEY k = nullptr;
	if (RegOpenKeyExW(root, key, 0, KEY_READ | view, &k) != ERROR_SUCCESS) return L"";
	const LONG r = RegQueryValueExW(k, value, nullptr, &type, reinterpret_cast<BYTE *>(buf), &size);
	RegCloseKey(k);
	if (r != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) return L"";
	buf[1023] = 0;
	return buf;
}

std::vector<std::wstring> VdfValues(const std::string &text, const std::string &key)
{
	std::vector<std::wstring> out;
	const std::string needle = "\"" + key + "\"";
	for (size_t p = text.find(needle); p != std::string::npos; p = text.find(needle, p + 1)) {
		const size_t a = text.find('"', p + needle.size());
		if (a == std::string::npos) break;
		std::string v;
		size_t i = a + 1;
		for (; i < text.size() && text[i] != '"'; i++) {
			if (text[i] == '\\' && i + 1 < text.size()) i++;
			v += text[i];
		}
		out.push_back(Widen(v));
	}
	return out;
}

std::wstring FindGame()
{
	std::vector<std::wstring> roots;
	std::wstring steam = RegString(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath");
	if (steam.empty()) steam = RegString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Valve\\Steam", L"InstallPath");
	if (steam.empty()) steam = RegString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Valve\\Steam", L"InstallPath");
	std::replace(steam.begin(), steam.end(), L'/', L'\\');
	if (!steam.empty()) {
		roots.push_back(steam);
		std::string vdf;
		if (ReadAll(steam + L"\\steamapps\\libraryfolders.vdf", vdf)) {
			for (std::wstring p : VdfValues(vdf, "path")) {
				std::replace(p.begin(), p.end(), L'/', L'\\');
				if (std::find_if(roots.begin(), roots.end(), [&](const std::wstring &r) { return _wcsicmp(r.c_str(), p.c_str()) == 0; }) == roots.end()) roots.push_back(p);
			}
		}
	}
	for (const std::wstring &root : roots) {
		std::string acf;
		if (!ReadAll(root + L"\\steamapps\\appmanifest_" + kAppId + L".acf", acf)) continue;
		const std::vector<std::wstring> dirs = VdfValues(acf, "installdir");
		if (dirs.empty()) continue;
		const std::wstring game = root + L"\\steamapps\\common\\" + dirs[0];
		if (Exists(game + L"\\gbr.exe")) return game;
	}
	for (const std::wstring &root : roots) {
		const std::wstring game = root + L"\\steamapps\\common\\Imperivm - Great Battles of Rome HD";
		if (Exists(game + L"\\gbr.exe")) return game;
	}
	return L"";
}

std::wstring Stamp()
{
	SYSTEMTIME t;
	GetLocalTime(&t);
	wchar_t buf[32];
	swprintf_s(buf, L"%04d%02d%02d-%02d%02d%02d", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
	return buf;
}

std::wstring g_backup;

std::wstring Backup()
{
	if (g_backup.empty()) g_backup = g_game + L"\\imperivm-resurrected\\_install-backup\\" + Stamp();
	MakeDirs(g_backup);
	return g_backup;
}

bool MoveToBackup(const std::wstring &path, const std::wstring &rel)
{
	const std::wstring to = Backup() + L"\\" + rel;
	MakeDirs(to.substr(0, to.find_last_of(L'\\')));
	return MoveFileExW(path.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED) != 0;
}

bool CopyToBackup(const std::wstring &path, const std::wstring &rel)
{
	const std::wstring to = Backup() + L"\\" + rel;
	MakeDirs(to.substr(0, to.find_last_of(L'\\')));
	return CopyFileW(path.c_str(), to.c_str(), FALSE) != 0;
}

void Unblock(const std::wstring &path) { DeleteFileW((path + L":Zone.Identifier").c_str()); }

bool CanWrite(const std::wstring &dir)
{
	const std::wstring probe = dir + L"\\.imperivm-write-test";
	HANDLE f = CreateFileW(probe.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
	if (f == INVALID_HANDLE_VALUE) return false;
	CloseHandle(f);
	return true;
}

bool Extract()
{
	mz_zip_archive zip = {};
	if (!mz_zip_reader_init_mem(&zip, g_payload.data(), g_payload.size(), 0)) return false;
	bool ok = true;
	int files = 0;
	const mz_uint n = mz_zip_reader_get_num_files(&zip);
	for (mz_uint i = 0; i < n && ok; i++) {
		mz_zip_archive_file_stat st;
		if (!mz_zip_reader_file_stat(&zip, i, &st)) {
			ok = false;
			break;
		}
		std::wstring rel = Widen(st.m_filename);
		std::replace(rel.begin(), rel.end(), L'/', L'\\');
		if (rel.empty() || rel.find(L"..") != std::wstring::npos || rel[0] == L'\\' || rel.find(L':') != std::wstring::npos) {
			ok = false;
			break;
		}
		const std::wstring dest = g_game + L"\\" + rel;
		if (mz_zip_reader_is_file_a_directory(&zip, i)) {
			MakeDirs(dest);
			continue;
		}
		MakeDirs(dest.substr(0, dest.find_last_of(L'\\')));
		size_t size = 0;
		void *data = mz_zip_reader_extract_to_heap(&zip, i, &size, 0);
		if (!data) {
			ok = false;
			break;
		}
		HANDLE f = CreateFileW(dest.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		DWORD wrote = 0;
		ok = f != INVALID_HANDLE_VALUE && WriteFile(f, data, static_cast<DWORD>(size), &wrote, nullptr) && wrote == size;
		if (f != INVALID_HANDLE_VALUE) CloseHandle(f);
		mz_free(data);
		if (!ok) Say(L"  ✗ " + rel);
		files++;
	}
	mz_zip_reader_end(&zip);
	if (ok) Say(Say3(L"  ✓ Archivos del mod copiados: ", L"  ✓ Mod files copied: ", L"  ✓ File della mod copiati: ") + std::to_wstring(files));
	return ok;
}

bool HarmfulLayer(const std::wstring &token)
{
	static const wchar_t *const kPrefixes[] = {L"WIN95", L"WIN98", L"WINXP", L"WIN2000", L"WINSRV", L"WIN7", L"WIN8", L"VISTA", L"NT4"};
	for (const wchar_t *p : kPrefixes) {
		if (_wcsnicmp(token.c_str(), p, wcslen(p)) == 0) return true;
	}
	static const wchar_t *const kExact[] = {L"256COLOR", L"16BITCOLOR", L"640X480", L"RUNASADMIN", L"DPIUNAWARE", L"GDIDPISCALING", L"PERPROCESSSYSTEMDPIFORCEON"};
	for (const wchar_t *e : kExact) {
		if (_wcsicmp(token.c_str(), e) == 0) return true;
	}
	return false;
}

struct LayerValue {
	std::wstring name, kept, removed;
	bool found = false;
};

std::vector<LayerValue> ReadLayers(HKEY hive, const std::vector<std::wstring> &exes)
{
	std::vector<LayerValue> out;
	HKEY key = nullptr;
	if (RegOpenKeyExW(hive, kLayersKey, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS) return out;
	for (DWORD i = 0;; i++) {
		wchar_t name[1024], data[1024];
		DWORD nameLen = 1024, dataLen = sizeof(data), type = 0;
		if (RegEnumValueW(key, i, name, &nameLen, nullptr, &type, reinterpret_cast<BYTE *>(data), &dataLen) != ERROR_SUCCESS) break;
		if (type != REG_SZ) continue;
		bool match = false;
		for (const std::wstring &e : exes) match = match || _wcsicmp(name, e.c_str()) == 0;
		if (!match) continue;
		data[std::min<size_t>(dataLen / sizeof(wchar_t), 1023)] = 0;
		LayerValue v;
		v.name = name;
		std::wstring text = data, token;
		bool keeps = false;
		for (size_t p = 0; p <= text.size(); p++) {
			if (p < text.size() && text[p] != L' ') {
				token += text[p];
				continue;
			}
			if (token.empty()) continue;
			if (HarmfulLayer(token)) v.removed += (v.removed.empty() ? L"" : L", ") + token;
			else {
				v.kept += (v.kept.empty() ? L"" : L" ") + token;
				if (token != L"~" && token != L"$") keeps = true;
			}
			token.clear();
		}
		if (!keeps) v.kept.clear();
		v.found = !v.removed.empty();
		if (v.found) out.push_back(v);
	}
	RegCloseKey(key);
	return out;
}

void FixLayers()
{
	std::vector<std::wstring> exes = {g_game + L"\\gbr.exe"};
	const std::wstring steamExe = RegString(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamExe");
	if (!steamExe.empty()) {
		std::wstring s = steamExe;
		std::replace(s.begin(), s.end(), L'/', L'\\');
		exes.push_back(s);
	}
	bool any = false;
	for (const LayerValue &v : ReadLayers(HKEY_CURRENT_USER, exes)) {
		HKEY key = nullptr;
		if (RegOpenKeyExW(HKEY_CURRENT_USER, kLayersKey, 0, KEY_SET_VALUE | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS) continue;
		const LONG r = v.kept.empty() ? RegDeleteValueW(key, v.name.c_str())
									  : RegSetValueExW(key, v.name.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE *>(v.kept.c_str()), static_cast<DWORD>((v.kept.size() + 1) * 2));
		RegCloseKey(key);
		Say((r == ERROR_SUCCESS ? L"  ✓ " : L"  ✗ ") + Say3(L"Compatibilidad quitada (tu usuario) de ", L"Compatibility removed (your user) from ", L"Compatibilità rimossa (il tuo utente) da ") +
			v.name.substr(v.name.find_last_of(L'\\') + 1) + L": " + v.removed);
		any = true;
	}
	const std::vector<LayerValue> machine = ReadLayers(HKEY_LOCAL_MACHINE, exes);
	if (!machine.empty()) {
		std::wstring script;
		for (const LayerValue &v : machine) {
			script += v.kept.empty() ? L"reg delete \"" + std::wstring(kLayersKeyMachine) + L"\" /v \"" + v.name + L"\" /f /reg:64 & "
									 : L"reg add \"" + std::wstring(kLayersKeyMachine) + L"\" /v \"" + v.name + L"\" /t REG_SZ /d \"" + v.kept + L"\" /f /reg:64 & ";
		}
		script += L"exit 0";
		SHELLEXECUTEINFOW info = {};
		info.cbSize = sizeof(info);
		info.fMask = SEE_MASK_NOCLOSEPROCESS;
		info.hwnd = g_wnd;
		info.lpVerb = g_elevated ? L"open" : L"runas";
		info.lpFile = L"cmd.exe";
		const std::wstring args = L"/c " + script;
		info.lpParameters = args.c_str();
		info.nShow = SW_HIDE;
		bool ok = false;
		if (ShellExecuteExW(&info) && info.hProcess) {
			WaitForSingleObject(info.hProcess, 30000);
			CloseHandle(info.hProcess);
			ok = ReadLayers(HKEY_LOCAL_MACHINE, exes).empty();
		}
		for (const LayerValue &v : machine) {
			Say((ok ? L"  ✓ " : L"  ✗ ") + Say3(L"Compatibilidad para todos los usuarios ", L"Compatibility for all users ", L"Compatibilità per tutti gli utenti ") +
				(ok ? Say3(L"quitada de ", L"removed from ", L"rimossa da ") : Say3(L"NO quitada (sin permiso) de ", L"NOT removed (no permission) from ", L"NON rimossa (senza permesso) da ")) +
				v.name.substr(v.name.find_last_of(L'\\') + 1) + L": " + v.removed);
		}
		any = true;
	}
	if (!any) Say(Say3(L"  ✓ Sin modos de compatibilidad de Windows", L"  ✓ No Windows compatibility modes", L"  ✓ Nessuna modalità compatibilità di Windows"));
}

void MoveForeignDlls()
{
	static const wchar_t *const kProxies[] = {L"d3d11.dll", L"dxgi.dll", L"d3dcompiler_47.dll", L"winhttp.dll", L"version.dll", L"ddraw.dll", L"d3d9.dll", L"d3d8.dll",
		L"dinput8.dll", L"dinput.dll", L"dsound.dll", L"opengl32.dll", L"dxwrapper.dll", L"dxwrapper.ini", L"winmm.dll.previous", L"winmm.dll.failed",
		L"winmm.dll.new", L"steam_api.dll.previous"};
	int moved = 0;
	for (const wchar_t *name : kProxies) {
		const std::wstring p = g_game + L"\\" + name;
		if (!Exists(p)) continue;
		const bool ok = MoveToBackup(p, std::wstring(L"files\\") + name);
		Say((ok ? L"  ✓ " : L"  ✗ ") + Say3(L"Apartado: ", L"Moved aside: ", L"Messo da parte: ") + name);
		moved++;
	}
	if (!moved) Say(Say3(L"  ✓ Sin restos de otros mods", L"  ✓ No leftovers of other mods", L"  ✓ Nessun resto di altre mod"));
}

void CopyTree(const std::wstring &from, const std::wstring &to, int &copied)
{
	WIN32_FIND_DATAW fd;
	HANDLE h = FindFirstFileW((from + L"\\*").c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE) return;
	MakeDirs(to);
	do {
		if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L"..")) continue;
		const std::wstring a = from + L"\\" + fd.cFileName, b = to + L"\\" + fd.cFileName;
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) CopyTree(a, b, copied);
		else if (CopyFileW(a.c_str(), b.c_str(), FALSE)) copied++;
	} while (FindNextFileW(h, &fd));
	FindClose(h);
}

void RestoreSaves(const std::wstring &from, const std::wstring &to, int &restored)
{
	WIN32_FIND_DATAW fd;
	HANDLE h = FindFirstFileW((from + L"\\*").c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE) return;
	do {
		if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L"..")) continue;
		const std::wstring a = from + L"\\" + fd.cFileName, b = to + L"\\" + fd.cFileName;
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) RestoreSaves(a, b, restored);
		else if (!Exists(b)) {
			MakeDirs(to);
			if (CopyFileW(a.c_str(), b.c_str(), TRUE)) restored++;
		}
	} while (FindNextFileW(h, &fd));
	FindClose(h);
}

void FixVirtualStore()
{
	wchar_t local[MAX_PATH];
	if (FAILED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, local))) return;
	const size_t colon = g_game.find(L':');
	if (colon == std::wstring::npos) return;
	const std::wstring vs = std::wstring(local) + L"\\VirtualStore" + g_game.substr(colon + 1);
	if (!IsDir(vs)) {
		Say(Say3(L"  ✓ Sin archivos en la carpeta virtual de Windows", L"  ✓ No files in the Windows virtual folder", L"  ✓ Nessun file nella cartella virtuale di Windows"));
		return;
	}
	int copied = 0, restored = 0;
	CopyTree(vs, Backup() + L"\\VirtualStore", copied);
	RestoreSaves(vs + L"\\Profiles", g_game + L"\\Profiles", restored);
	SHFILEOPSTRUCTW op = {};
	std::wstring from = vs;
	from.push_back(L'\0');
	op.wFunc = FO_DELETE;
	op.pFrom = from.c_str();
	op.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT | FOF_ALLOWUNDO;
	const bool removed = copied >= 0 && SHFileOperationW(&op) == 0;
	Say((removed ? L"  ✓ " : L"  ✗ ") + Say3(L"Carpeta virtual de Windows guardada en la copia y vaciada (", L"Windows virtual folder saved in the backup and emptied (",
		L"Cartella virtuale di Windows salvata nel backup e svuotata (") + std::to_wstring(copied) + Say3(L" archivos; partidas recuperadas: ", L" files; saves recovered: ", L" file; salvataggi recuperati: ") +
		std::to_wstring(restored) + L")");
}

void IniDelete(const std::wstring &ini, const wchar_t *section, const wchar_t *key, std::wstring &changed)
{
	wchar_t buf[256];
	if (!GetPrivateProfileStringW(section, key, L"", buf, 256, ini.c_str())) return;
	WritePrivateProfileStringW(section, key, nullptr, ini.c_str());
	changed += (changed.empty() ? L"" : L", ") + std::wstring(key);
}

void FixDisplay()
{
	const std::wstring modIni = g_game + L"\\imperivm-mod.ini", gameIni = g_game + L"\\Settings.ini";
	std::wstring changed;
	if (Exists(modIni)) {
		CopyToBackup(modIni, L"files\\imperivm-mod.ini");
		IniDelete(modIni, L"Display", L"Resolution", changed);
		IniDelete(modIni, L"Display", L"Monitor", changed);
		IniDelete(modIni, L"Display", L"DpiAware", changed);
		IniDelete(modIni, L"Render", L"FakeModeChange", changed);
		IniDelete(modIni, L"Render", L"Enabled", changed);
		IniDelete(modIni, L"Updates", L"Testing", changed);
		IniDelete(modIni, L"Updates", L"Starts", changed);
		IniDelete(modIni, L"Updates", L"Previous", changed);
		IniDelete(modIni, L"Updates", L"Removed", changed);
	}
	if (Exists(gameIni)) {
		wchar_t buf[32];
		GetPrivateProfileStringW(L"Options", L"Resolution", L"", buf, 32, gameIni.c_str());
		if (buf[0] && wcscmp(buf, L"0") != 0) {
			CopyToBackup(gameIni, L"files\\Settings.ini");
			WritePrivateProfileStringW(L"Options", L"Resolution", L"0", gameIni.c_str());
			changed += (changed.empty() ? L"" : L", ") + std::wstring(L"Settings.ini Resolution");
		}
	}
	Say(changed.empty() ? Say3(L"  ✓ Configuración de pantalla correcta", L"  ✓ Display settings are fine", L"  ✓ Impostazioni dello schermo corrette")
						: Say3(L"  ✓ Pantalla reiniciada (se usará la de tu monitor): ", L"  ✓ Display reset (your monitor's size will be used): ", L"  ✓ Schermo ripristinato (si userà quella del monitor): ") + changed);
}

void Warnings()
{
	const uint64_t size = FileSize(g_game + L"\\gbr.exe");
	if (size != kSteamExeSize) {
		Say(Say3(L"  ! gbr.exe no es el de Steam (otro tamaño). Si el juego falla, en Steam: botón derecho → Propiedades → Archivos instalados → Verificar.",
			L"  ! gbr.exe is not Steam's (different size). If the game fails, in Steam: right click → Properties → Installed files → Verify.",
			L"  ! gbr.exe non è quello di Steam (dimensione diversa). Se il gioco non va, in Steam: clic destro → Proprietà → File installati → Verifica."));
	}
	DWORD sac = 0, len = sizeof(sac);
	if (RegGetValueW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\CI\\Policy", L"VerifiedAndReputablePolicyState", RRF_RT_REG_DWORD, nullptr, &sac, &len) == ERROR_SUCCESS && sac == 1) {
		Say(Say3(L"  ! El «Control inteligente de aplicaciones» de Windows está activado y puede impedir que el mod se cargue.",
			L"  ! Windows \"Smart App Control\" is on and can stop the mod from loading.",
			L"  ! Il «Controllo intelligente delle app» di Windows è attivo e può impedire il caricamento della mod."));
	}
	if (!ProcessRunning(L"steam.exe")) {
		Say(Say3(L"  ! Steam no está abierto: ábrelo antes de jugar (el online lo necesita).", L"  ! Steam is not running: open it before playing (online needs it).",
			L"  ! Steam non è aperto: aprilo prima di giocare (l'online ne ha bisogno)."));
	}
}

bool WaitGameClosed()
{
	while (ProcessRunning(L"gbr.exe")) {
		if (MessageBoxW(g_wnd, T(kGameOpen), T(kTitle), MB_OKCANCEL | MB_ICONINFORMATION) != IDOK) {
			Say(T(kStillOpen));
			return false;
		}
	}
	return true;
}

bool ModInstalled() { return Exists(g_game + L"\\winmm.dll") && IsDir(g_game + L"\\imperivm-resurrected"); }

void SetFolder(const std::wstring &game)
{
	g_game = game;
	SetWindowTextW(g_folder, game.empty() ? L"—" : game.c_str());
	EnableWindow(g_install, !game.empty());
	SetWindowTextW(g_install, !game.empty() && ModInstalled() ? T(kRepair) : T(kInstall));
	EnableWindow(g_uninstall, !game.empty() && ModInstalled());
}

void WriteInstallLog()
{
	if (g_game.empty()) return;
	MakeDirs(g_game + L"\\Logs\\imperivm");
	const std::string text = Narrow(g_report);
	HANDLE f = CreateFileW((g_game + L"\\Logs\\imperivm\\installer.log").c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
	if (f == INVALID_HANDLE_VALUE) return;
	DWORD wrote = 0;
	WriteFile(f, "\xEF\xBB\xBF", 3, &wrote, nullptr);
	WriteFile(f, text.data(), static_cast<DWORD>(text.size()), &wrote, nullptr);
	CloseHandle(f);
}

bool RelaunchElevated(const wchar_t *action)
{
	wchar_t self[MAX_PATH];
	GetModuleFileNameW(nullptr, self, MAX_PATH);
	const std::wstring args = std::wstring(L"--elevated ") + action + L" \"" + g_game + L"\"";
	SHELLEXECUTEINFOW info = {};
	info.cbSize = sizeof(info);
	info.fMask = SEE_MASK_NOCLOSEPROCESS;
	info.hwnd = g_wnd;
	info.lpVerb = L"runas";
	info.lpFile = self;
	info.lpParameters = args.c_str();
	info.nShow = SW_SHOWNORMAL;
	if (!ShellExecuteExW(&info) || !info.hProcess) return false;
	CloseHandle(info.hProcess);
	PostQuitMessage(0);
	return true;
}

void DoInstall()
{
	g_report.clear();
	g_backup.clear();
	Say(Say3(L"Instalando Imperivm Resurrected ", L"Installing Imperivm Resurrected ", L"Installazione di Imperivm Resurrected ") + g_version + L"…");
	if (!WaitGameClosed()) return;
	if (!g_elevated && !CanWrite(g_game)) {
		Say(Say3(L"No hay permiso de escritura en la carpeta del juego: pidiendo permiso a Windows…", L"No write permission in the game folder: asking Windows for permission…",
			L"Nessun permesso di scrittura nella cartella del gioco: chiedo il permesso a Windows…"));
		if (RelaunchElevated(L"install")) return;
		Say(T(kFailed));
		return;
	}
	FixLayers();
	MoveForeignDlls();
	FixVirtualStore();
	FixDisplay();
	const bool ok = Extract();
	for (const wchar_t *name : {L"winmm.dll", L"steam_api.dll"}) Unblock(g_game + L"\\" + name);
	Warnings();
	if (!g_backup.empty()) Say(Say3(L"Copia de lo cambiado: ", L"Backup of what changed: ", L"Copia di ciò che è cambiato: ") + g_backup);
	Say(L"");
	Say(ok ? T(kDone) : T(kFailed));
	WriteInstallLog();
	SetFolder(g_game);
	EnableWindow(g_play, ok);
}

void DoUninstall()
{
	if (MessageBoxW(g_wnd, T(kConfirmUninstall), T(kTitle), MB_OKCANCEL | MB_ICONQUESTION) != IDOK) return;
	g_report.clear();
	if (!WaitGameClosed()) return;
	if (!g_elevated && !CanWrite(g_game)) {
		if (RelaunchElevated(L"uninstall")) return;
	}
	std::wstring from;
	for (const wchar_t *name : {L"winmm.dll", L"steam_api.dll", L"imperivm-resurrected"}) {
		const std::wstring p = g_game + L"\\" + name;
		if (Exists(p)) from += p + L'\0';
	}
	bool ok = true;
	if (!from.empty()) {
		from.push_back(L'\0');
		SHFILEOPSTRUCTW op = {};
		op.hwnd = g_wnd;
		op.wFunc = FO_DELETE;
		op.pFrom = from.c_str();
		op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;
		ok = SHFileOperationW(&op) == 0 && !op.fAnyOperationsAborted;
	}
	Say(ok ? T(kUninstalled) : T(kFailed));
	SetFolder(g_game);
	EnableWindow(g_play, FALSE);
}

void PickFolder()
{
	BROWSEINFOW bi = {};
	bi.hwndOwner = g_wnd;
	bi.lpszTitle = T(kPickFolder);
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
	if (!pidl) return;
	wchar_t path[MAX_PATH];
	const bool ok = SHGetPathFromIDListW(pidl, path) != 0;
	CoTaskMemFree(pidl);
	if (!ok) return;
	if (!Exists(std::wstring(path) + L"\\gbr.exe")) {
		MessageBoxW(g_wnd, T(kNoGbr), T(kTitle), MB_OK | MB_ICONWARNING);
		return;
	}
	SetFolder(path);
}

enum { kIdInstall = 101, kIdUninstall, kIdPlay, kIdClose, kIdChange };

HFONT g_font = nullptr;

HWND Child(const wchar_t *cls, const wchar_t *text, DWORD style, int x, int y, int w, int h, int id, float s)
{
	HWND c = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style, static_cast<int>(x * s), static_cast<int>(y * s), static_cast<int>(w * s),
		static_cast<int>(h * s), g_wnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
	SendMessageW(c, WM_SETFONT, reinterpret_cast<WPARAM>(g_font), TRUE);
	return c;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_COMMAND:
		switch (LOWORD(wp)) {
		case kIdInstall: DoInstall(); return 0;
		case kIdUninstall: DoUninstall(); return 0;
		case kIdChange: PickFolder(); return 0;
		case kIdPlay: ShellExecuteW(hwnd, L"open", L"steam://rungameid/752580", nullptr, nullptr, SW_SHOWNORMAL); return 0;
		case kIdClose: DestroyWindow(hwnd); return 0;
		}
		break;
	case WM_CTLCOLORSTATIC:
		SetBkMode(reinterpret_cast<HDC>(wp), TRANSPARENT);
		return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(hwnd, msg, wp, lp);
}

}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int)
{
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	for (const LANGID id : {LANGIDFROMLCID(GetUserDefaultLCID()), GetUserDefaultUILanguage()}) {
		const LANGID lang = PRIMARYLANGID(id);
		if (lang == LANG_SPANISH || lang == LANG_ITALIAN) {
			g_lang = lang == LANG_SPANISH ? kEs : kIt;
			break;
		}
	}
	INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_STANDARD_CLASSES};
	InitCommonControlsEx(&icc);
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

	int argc = 0;
	wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	std::wstring action;
	if (argc >= 4 && !wcscmp(argv[1], L"--elevated")) {
		g_elevated = true;
		action = argv[2];
		g_game = argv[3];
	}

	const bool payload = LoadPayload();

	WNDCLASSEXW wc = {sizeof(wc)};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = inst;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
	wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(1));
	wc.lpszClassName = L"ImperivmResurrectedSetup";
	RegisterClassExW(&wc);
	const UINT dpi = GetDpiForSystem();
	const float s = dpi / 96.0f;
	NONCLIENTMETRICSW ncm = {sizeof(ncm)};
	SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
	ncm.lfMessageFont.lfHeight = -static_cast<LONG>(15 * s);
	g_font = CreateFontIndirectW(&ncm.lfMessageFont);
	RECT rc = {0, 0, static_cast<LONG>(640 * s), static_cast<LONG>(470 * s)};
	AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);
	std::wstring title = std::wstring(T(kTitle)) + (g_version.empty() ? L"" : L" " + g_version);
	g_wnd = CreateWindowExW(0, wc.lpszClassName, title.c_str(), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT,
		rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, inst, nullptr);

	Child(L"STATIC", T(kIntro), 0, 20, 16, 600, 76, 0, s);
	Child(L"STATIC", T(kFolderLabel), 0, 20, 100, 600, 20, 0, s);
	g_folder = Child(L"EDIT", L"", ES_READONLY | ES_AUTOHSCROLL | WS_BORDER, 20, 122, 490, 26, 0, s);
	Child(L"BUTTON", T(kChange), BS_PUSHBUTTON, 520, 121, 100, 28, kIdChange, s);
	g_log = Child(L"EDIT", L"", ES_READONLY | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | WS_BORDER, 20, 160, 600, 236, 0, s);
	g_install = Child(L"BUTTON", T(kInstall), BS_DEFPUSHBUTTON, 20, 410, 140, 36, kIdInstall, s);
	g_uninstall = Child(L"BUTTON", T(kUninstall), BS_PUSHBUTTON, 170, 410, 140, 36, kIdUninstall, s);
	g_play = Child(L"BUTTON", T(kPlay), BS_PUSHBUTTON, 340, 410, 150, 36, kIdPlay, s);
	Child(L"BUTTON", T(kClose), BS_PUSHBUTTON, 500, 410, 120, 36, kIdClose, s);
	EnableWindow(g_play, FALSE);

	ShowWindow(g_wnd, SW_SHOWNORMAL);
	UpdateWindow(g_wnd);

	if (!payload) {
		Say(T(kBadPayload));
		EnableWindow(g_install, FALSE);
		EnableWindow(g_uninstall, FALSE);
	}
	else {
		std::wstring game = g_game.empty() ? FindGame() : g_game;
		if (game.size() > 1 && game[1] == L':') game[0] = static_cast<wchar_t>(towupper(game[0]));
		SetFolder(game);
		if (g_game.empty()) Say(T(kNotFound));
		else if (action.empty()) Say(ModInstalled() ? T(kReadyRepair) : T(kReadyInstall));
		if (action == L"install" && !g_game.empty()) DoInstall();
		else if (action == L"uninstall" && !g_game.empty()) DoUninstall();
	}

	MSG m;
	while (GetMessageW(&m, nullptr, 0, 0) > 0) {
		if (!IsDialogMessageW(g_wnd, &m)) {
			TranslateMessage(&m);
			DispatchMessageW(&m);
		}
	}
	CoUninitialize();
	return 0;
}
