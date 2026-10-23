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
#include <string>
#include <vector>

#include "miniz.h"

namespace {

const wchar_t kAppId[] = L"752580";
const uint64_t kSteamExeSize = 4456448;
const char kTrailerMagic[8] = {'I', 'R', 'S', 'E', 'T', 'U', 'P', '1'};
const size_t kTrailerSize = 8 + 8 + 32 + 32;
const DWORD kRegTimeoutMs = 60000;
const int kFontPoints = 9;
const wchar_t kStagedSuffix[] = L".irnew";
const wchar_t kBackupFolder[] = L"imperivm-resurrected-backup";
const wchar_t kLayersKey[] = L"Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers";
const wchar_t kLayersKeyMachine[] = L"HKLM\\Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\Layers";

enum Lang { kEs, kEn, kIt };
Lang g_lang = kEn;
const wchar_t *const kLangCodes[] = {L"es", L"en", L"it"};
const wchar_t *const kLangNames[] = {L"Español", L"English", L"Italiano"};

struct Text {
	const wchar_t *es, *en, *it;
};

const wchar_t *T(const Text &t) { return g_lang == kEs ? t.es : g_lang == kIt ? t.it : t.en; }
std::wstring T3(const wchar_t *es, const wchar_t *en, const wchar_t *it) { return T({es, en, it}); }

const Text kTitle = {L"Instalación de Imperivm Resurrected", L"Imperivm Resurrected Setup", L"Installazione di Imperivm Resurrected"};
const Text kIntro = {L"Instala el mod en tu copia de Steam de Imperivm III HD y prepara el juego para que arranque: quita los modos de compatibilidad de Windows, guarda en una copia de seguridad los archivos de otros mods y restablece la resolución guardada. Tus partidas y mapas no se modifican.",
	L"Installs the mod into your Steam copy of Imperivm III HD and prepares the game to start: it removes Windows compatibility modes, moves files of other mods to a backup folder and resets the saved resolution. Your saves and maps are not modified.",
	L"Installa la mod nella tua copia Steam di Imperivm III HD e prepara il gioco all'avvio: rimuove le modalità di compatibilità di Windows, sposta i file di altre mod in una cartella di backup e ripristina la risoluzione salvata. Salvataggi e mappe non vengono modificati."};
const Text kFolderLabel = {L"Carpeta del juego:", L"Game folder:", L"Cartella del gioco:"};
const Text kChange = {L"&Cambiar…", L"&Change…", L"&Cambia…"};
const Text kInstall = {L"&Instalar", L"&Install", L"&Installa"};
const Text kRepair = {L"&Reparar", L"&Repair", L"&Ripara"};
const Text kUninstall = {L"&Desinstalar", L"&Uninstall", L"&Disinstalla"};
const Text kPlay = {L"&Abrir el juego", L"&Open the game", L"&Apri il gioco"};
const Text kClose = {L"Cerrar", L"Close", L"Chiudi"};
const Text kNotFound = {L"No se ha encontrado el juego en las bibliotecas de Steam. Pulsa «Cambiar…» y elige la carpeta que contiene gbr.exe.",
	L"The game was not found in your Steam libraries. Click “Change…” and select the folder that contains gbr.exe.",
	L"Il gioco non è stato trovato nelle librerie di Steam. Fai clic su «Cambia…» e scegli la cartella che contiene gbr.exe."};
const Text kReadyInstall = {L"Juego encontrado. Pulsa «Instalar» para continuar.", L"Game found. Click “Install” to continue.", L"Gioco trovato. Fai clic su «Installa» per continuare."};
const Text kReadyRepair = {L"El mod ya está instalado. «Reparar» lo actualiza a esta versión y vuelve a preparar el juego; «Desinstalar» lo quita.",
	L"The mod is already installed. “Repair” updates it to this version and prepares the game again; “Uninstall” removes it.",
	L"La mod è già installata. «Ripara» la aggiorna a questa versione e prepara di nuovo il gioco; «Disinstalla» la rimuove."};
const Text kPickFolder = {L"Elige la carpeta del juego (la que contiene gbr.exe)", L"Select the game folder (the one that contains gbr.exe)", L"Scegli la cartella del gioco (quella che contiene gbr.exe)"};
const Text kNoGbr = {L"La carpeta seleccionada no contiene gbr.exe.", L"The selected folder does not contain gbr.exe.", L"La cartella selezionata non contiene gbr.exe."};
const Text kGameOpen = {L"El juego está abierto. Ciérralo y pulsa Aceptar para continuar.", L"The game is running. Close it and click OK to continue.", L"Il gioco è aperto. Chiudilo e fai clic su OK per continuare."};
const Text kStillOpen = {L"El juego sigue abierto: no se ha cambiado nada.", L"The game is still running: nothing was changed.", L"Il gioco è ancora aperto: non è stato modificato nulla."};
const Text kNeedAdmin = {L"La carpeta del juego requiere permisos de administrador. Solicitando permiso…", L"The game folder requires administrator rights. Requesting permission…",
	L"La cartella del gioco richiede i permessi di amministratore. Richiesta dell'autorizzazione…"};
const Text kDenied = {L"No se ha concedido el permiso de administrador: no se ha cambiado nada.", L"Administrator permission was not granted: nothing was changed.",
	L"L'autorizzazione di amministratore non è stata concessa: non è stato modificato nulla."};
const Text kDone = {L"Instalación completada. Abre el juego desde Steam o pulsa «Abrir el juego».", L"Setup is complete. Open the game from Steam or click “Open the game”.",
	L"Installazione completata. Apri il gioco da Steam o fai clic su «Apri il gioco»."};
const Text kUninstalled = {L"El mod se ha quitado.", L"The mod was removed.", L"La mod è stata rimossa."};
const Text kInstallFailed = {L"La instalación no se ha completado. Revisa los detalles de arriba y Logs\\imperivm\\installer.log.",
	L"Setup did not complete. See the details above and Logs\\imperivm\\installer.log.", L"L'installazione non è stata completata. Controlla i dettagli qui sopra e Logs\\imperivm\\installer.log."};
const Text kUninstallFailed = {L"No se han podido quitar todos los archivos del mod. Cierra los programas que los usen e inténtalo de nuevo.",
	L"Some mod files could not be removed. Close any program that uses them and try again.", L"Non è stato possibile rimuovere tutti i file della mod. Chiudi i programmi che li usano e riprova."};
const Text kBadPayload = {L"Este instalador está dañado o incompleto. Descárgalo de nuevo.", L"This installer is damaged or incomplete. Download it again.", L"Questo installatore è danneggiato o incompleto. Scaricalo di nuovo."};
const Text kConfirmUninstall = {L"¿Quitar el mod? winmm.dll, steam_api.dll y la carpeta imperivm-resurrected se moverán a la papelera. Tus partidas, mapas y ajustes se conservan.",
	L"Remove the mod? winmm.dll, steam_api.dll and the imperivm-resurrected folder will be moved to the Recycle Bin. Your saves, maps and settings are kept.",
	L"Rimuovere la mod? winmm.dll, steam_api.dll e la cartella imperivm-resurrected verranno spostati nel Cestino. Salvataggi, mappe e impostazioni restano."};

std::wstring g_report;
HWND g_wnd = nullptr, g_log = nullptr, g_folder = nullptr, g_install = nullptr, g_uninstall = nullptr, g_play = nullptr, g_change = nullptr, g_close = nullptr;
HWND g_intro = nullptr, g_folderLabel = nullptr, g_language = nullptr;
bool g_acted = false;
std::wstring g_game;
std::wstring g_localAppData;
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

std::wstring Parent(const std::wstring &path) { return path.substr(0, path.find_last_of(L'\\')); }
std::wstring FileName(const std::wstring &path) { return path.substr(path.find_last_of(L'\\') + 1); }

std::wstring TrimSlashes(std::wstring path)
{
	while (!path.empty() && (path.back() == L'\\' || path.back() == L'/')) path.pop_back();
	return path;
}

std::wstring RealCase(const std::wstring &path)
{
	HANDLE h = CreateFileW(path.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
	if (h == INVALID_HANDLE_VALUE) return path;
	wchar_t buf[MAX_PATH * 2];
	const DWORD n = GetFinalPathNameByHandleW(h, buf, MAX_PATH * 2, FILE_NAME_NORMALIZED | VOLUME_NAME_DOS);
	CloseHandle(h);
	if (n == 0 || n >= MAX_PATH * 2) return path;
	std::wstring real = buf;
	if (real.rfind(L"\\\\?\\", 0) == 0 && real.rfind(L"\\\\?\\UNC\\", 0) != 0) real = real.substr(4);
	return _wcsicmp(real.c_str(), path.c_str()) == 0 ? real : path;
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
	if (!ReadAll(self, exe) || exe.size() < kTrailerSize) return false;
	const char *trailer = exe.data() + exe.size() - kTrailerSize;
	if (memcmp(trailer, kTrailerMagic, 8) != 0) return false;
	uint64_t size = 0;
	memcpy(&size, trailer + 8, 8);
	if (size == 0 || size > exe.size() - kTrailerSize) return false;
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

std::wstring RegString(HKEY root, const wchar_t *key, const wchar_t *value)
{
	wchar_t buf[1024] = {};
	DWORD size = sizeof(buf) - sizeof(wchar_t), type = 0;
	HKEY k = nullptr;
	if (RegOpenKeyExW(root, key, 0, KEY_READ, &k) != ERROR_SUCCESS) return L"";
	const LONG r = RegQueryValueExW(k, value, nullptr, &type, reinterpret_cast<BYTE *>(buf), &size);
	RegCloseKey(k);
	if (r != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) return L"";
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
		for (size_t i = a + 1; i < text.size() && text[i] != '"'; i++) {
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
	if (g_backup.empty()) g_backup = g_game + L"\\" + kBackupFolder + L"\\" + Stamp();
	MakeDirs(g_backup);
	return g_backup;
}

bool MoveToBackup(const std::wstring &path, const std::wstring &rel)
{
	const std::wstring to = Backup() + L"\\" + rel;
	MakeDirs(Parent(to));
	return MoveFileExW(path.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED) != 0;
}

bool CopyToBackup(const std::wstring &path, const std::wstring &rel)
{
	const std::wstring to = Backup() + L"\\" + rel;
	MakeDirs(Parent(to));
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

bool ModInstalled() { return Exists(g_game + L"\\winmm.dll") && IsDir(g_game + L"\\imperivm-resurrected"); }

void DiscardStaged(const std::vector<std::wstring> &staged)
{
	for (const std::wstring &dest : staged) DeleteFileW((dest + kStagedSuffix).c_str());
}

bool StageFiles(std::vector<std::wstring> &staged)
{
	mz_zip_archive zip = {};
	if (!mz_zip_reader_init_mem(&zip, g_payload.data(), g_payload.size(), 0)) return false;
	bool ok = true;
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
			MakeDirs(TrimSlashes(dest));
			continue;
		}
		MakeDirs(Parent(dest));
		size_t size = 0;
		void *data = mz_zip_reader_extract_to_heap(&zip, i, &size, 0);
		if (!data) {
			ok = false;
			break;
		}
		HANDLE f = CreateFileW((dest + kStagedSuffix).c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		DWORD wrote = 0;
		ok = f != INVALID_HANDLE_VALUE && WriteFile(f, data, static_cast<DWORD>(size), &wrote, nullptr) && wrote == size;
		if (f != INVALID_HANDLE_VALUE) CloseHandle(f);
		mz_free(data);
		if (f != INVALID_HANDLE_VALUE) staged.push_back(dest);
		if (!ok) Say(L"  ✗ " + rel);
	}
	mz_zip_reader_end(&zip);
	if (!ok) DiscardStaged(staged);
	return ok;
}

bool CommitFiles(const std::vector<std::wstring> &staged)
{
	int done = 0;
	for (const std::wstring &dest : staged) {
		if (MoveFileExW((dest + kStagedSuffix).c_str(), dest.c_str(), MOVEFILE_REPLACE_EXISTING)) done++;
		else Say(L"  ✗ " + dest.substr(g_game.size() + 1));
	}
	const bool ok = done == static_cast<int>(staged.size());
	if (ok) Say(T3(L"  ✓ Archivos del mod copiados: ", L"  ✓ Mod files copied: ", L"  ✓ File della mod copiati: ") + std::to_wstring(done));
	else DiscardStaged(staged);
	return ok;
}

void MoveForeignModFiles()
{
	for (const wchar_t *name : {L"winmm.dll", L"steam_api.dll"}) {
		const std::wstring p = g_game + L"\\" + name;
		if (!Exists(p)) continue;
		const bool ok = MoveToBackup(p, std::wstring(L"files\\") + name);
		Say((ok ? L"  ✓ " : L"  ✗ ") + T3(L"Movido a la copia de seguridad: ", L"Moved to the backup: ", L"Spostato nel backup: ") + name);
	}
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
};

std::vector<LayerValue> ReadLayers(HKEY hive, const std::vector<std::wstring> &exes)
{
	std::vector<LayerValue> out;
	HKEY key = nullptr;
	if (RegOpenKeyExW(hive, kLayersKey, 0, KEY_READ | KEY_WOW64_64KEY, &key) != ERROR_SUCCESS) return out;
	for (DWORD i = 0;; i++) {
		wchar_t name[1024], data[1024];
		DWORD nameLen = 1024, dataLen = sizeof(data) - sizeof(wchar_t), type = 0;
		const LONG r = RegEnumValueW(key, i, name, &nameLen, nullptr, &type, reinterpret_cast<BYTE *>(data), &dataLen);
		if (r == ERROR_NO_MORE_ITEMS) break;
		if (r != ERROR_SUCCESS || type != REG_SZ) continue;
		bool match = false;
		for (const std::wstring &e : exes) match = match || _wcsicmp(name, e.c_str()) == 0;
		if (!match) continue;
		data[dataLen / sizeof(wchar_t)] = 0;
		LayerValue v;
		v.name = name;
		const std::wstring text = data;
		std::wstring token;
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
		if (!v.removed.empty()) out.push_back(v);
	}
	RegCloseKey(key);
	return out;
}

std::vector<std::wstring> LayerExes()
{
	std::vector<std::wstring> exes = {g_game + L"\\gbr.exe"};
	std::wstring steamExe = RegString(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamExe");
	if (!steamExe.empty()) {
		std::replace(steamExe.begin(), steamExe.end(), L'/', L'\\');
		exes.push_back(steamExe);
	}
	return exes;
}

bool FixUserLayers()
{
	const std::vector<LayerValue> found = ReadLayers(HKEY_CURRENT_USER, LayerExes());
	for (const LayerValue &v : found) {
		HKEY key = nullptr;
		LONG r = RegOpenKeyExW(HKEY_CURRENT_USER, kLayersKey, 0, KEY_SET_VALUE | KEY_WOW64_64KEY, &key);
		if (r == ERROR_SUCCESS) {
			r = v.kept.empty() ? RegDeleteValueW(key, v.name.c_str())
							   : RegSetValueExW(key, v.name.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE *>(v.kept.c_str()), static_cast<DWORD>((v.kept.size() + 1) * sizeof(wchar_t)));
			RegCloseKey(key);
		}
		Say((r == ERROR_SUCCESS ? L"  ✓ " : L"  ✗ ") +
			T3(L"Modo de compatibilidad quitado para tu usuario en ", L"Compatibility mode removed for the current user from ", L"Modalità di compatibilità rimossa per l'utente corrente da ") +
			FileName(v.name) + L": " + v.removed);
	}
	return !found.empty();
}

bool FixMachineLayers()
{
	const std::vector<std::wstring> exes = LayerExes();
	const std::vector<LayerValue> machine = ReadLayers(HKEY_LOCAL_MACHINE, exes);
	if (machine.empty()) return false;
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
	bool ran = false, ok = false;
	if (ShellExecuteExW(&info) && info.hProcess) {
		ran = true;
		WaitForSingleObject(info.hProcess, kRegTimeoutMs);
		CloseHandle(info.hProcess);
		ok = ReadLayers(HKEY_LOCAL_MACHINE, exes).empty();
	}
	for (const LayerValue &v : machine) {
		Say((ok ? L"  ✓ " : L"  ✗ ") +
			(ok ? T3(L"Modo de compatibilidad quitado para todos los usuarios en ", L"Compatibility mode removed for all users from ", L"Modalità di compatibilità rimossa per tutti gli utenti da ")
				: ran ? T3(L"No se ha podido quitar el modo de compatibilidad para todos los usuarios en ", L"Could not remove the compatibility mode for all users from ",
						  L"Impossibile rimuovere la modalità di compatibilità per tutti gli utenti da ")
					  : T3(L"Modo de compatibilidad para todos los usuarios sin quitar (permiso de administrador denegado) en ",
						  L"Compatibility mode for all users not removed (administrator permission denied) from ",
						  L"Modalità di compatibilità per tutti gli utenti non rimossa (autorizzazione di amministratore negata) da ")) +
			FileName(v.name) + L": " + v.removed);
	}
	return true;
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
		Say((ok ? L"  ✓ " : L"  ✗ ") + T3(L"Movido a la copia de seguridad: ", L"Moved to the backup: ", L"Spostato nel backup: ") + name);
		moved++;
	}
	if (!moved) Say(T3(L"  ✓ No hay archivos de otros mods", L"  ✓ No files from other mods", L"  ✓ Nessun file di altre mod"));
}

int CopyTree(const std::wstring &from, const std::wstring &to, int &copied)
{
	WIN32_FIND_DATAW fd;
	HANDLE h = FindFirstFileW((from + L"\\*").c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE) return 0;
	MakeDirs(to);
	int failed = 0;
	do {
		if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L"..")) continue;
		const std::wstring a = from + L"\\" + fd.cFileName, b = to + L"\\" + fd.cFileName;
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) failed += CopyTree(a, b, copied);
		else if (CopyFileW(a.c_str(), b.c_str(), FALSE)) copied++;
		else failed++;
	} while (FindNextFileW(h, &fd));
	FindClose(h);
	return failed;
}

bool Newer(const std::wstring &a, const std::wstring &b)
{
	WIN32_FILE_ATTRIBUTE_DATA da, db;
	if (!GetFileAttributesExW(a.c_str(), GetFileExInfoStandard, &da) || !GetFileAttributesExW(b.c_str(), GetFileExInfoStandard, &db)) return false;
	return CompareFileTime(&da.ftLastWriteTime, &db.ftLastWriteTime) > 0;
}

void RestoreSaves(const std::wstring &from, const std::wstring &to, const std::wstring &rel, int &restored)
{
	WIN32_FIND_DATAW fd;
	HANDLE h = FindFirstFileW((from + L"\\*").c_str(), &fd);
	if (h == INVALID_HANDLE_VALUE) return;
	do {
		if (!wcscmp(fd.cFileName, L".") || !wcscmp(fd.cFileName, L"..")) continue;
		const std::wstring a = from + L"\\" + fd.cFileName, b = to + L"\\" + fd.cFileName, r = rel + L"\\" + fd.cFileName;
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			RestoreSaves(a, b, r, restored);
			continue;
		}
		if (Exists(b) && !Newer(a, b)) continue;
		if (Exists(b) && !CopyToBackup(b, L"files" + r)) continue;
		MakeDirs(to);
		if (CopyFileW(a.c_str(), b.c_str(), FALSE)) restored++;
	} while (FindNextFileW(h, &fd));
	FindClose(h);
}

void FixVirtualStore()
{
	const size_t colon = g_game.find(L':');
	if (g_localAppData.empty() || colon == std::wstring::npos) return;
	const std::wstring vs = g_localAppData + L"\\VirtualStore" + g_game.substr(colon + 1);
	if (!IsDir(vs)) {
		Say(T3(L"  ✓ No hay archivos del juego en VirtualStore", L"  ✓ No game files in VirtualStore", L"  ✓ Nessun file del gioco in VirtualStore"));
		return;
	}
	int copied = 0, restored = 0;
	const int failed = CopyTree(vs, Backup() + L"\\VirtualStore", copied);
	RestoreSaves(vs + L"\\Profiles", g_game + L"\\Profiles", L"\\Profiles", restored);
	bool removed = false;
	if (!failed) {
		SHFILEOPSTRUCTW op = {};
		std::wstring from = vs;
		from.push_back(L'\0');
		op.wFunc = FO_DELETE;
		op.pFrom = from.c_str();
		op.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT | FOF_ALLOWUNDO;
		removed = SHFileOperationW(&op) == 0 && !op.fAnyOperationsAborted;
	}
	Say((removed ? L"  ✓ " : L"  ✗ ") +
		(removed ? T3(L"Archivos del juego en VirtualStore movidos a la copia de seguridad (", L"Game files in VirtualStore moved to the backup (",
					   L"File del gioco in VirtualStore spostati nel backup (")
				 : T3(L"No se ha podido vaciar VirtualStore (", L"Could not empty VirtualStore (", L"Impossibile svuotare VirtualStore (")) +
		std::to_wstring(copied) + T3(L" archivos; partidas recuperadas: ", L" files; saves recovered: ", L" file; salvataggi recuperati: ") + std::to_wstring(restored) + L")");
}

void FixDisplay()
{
	static const wchar_t *const kResetKeys[][2] = {{L"Display", L"Resolution"}, {L"Display", L"Monitor"}, {L"Display", L"DpiAware"}, {L"Render", L"FakeModeChange"},
		{L"Render", L"Enabled"}, {L"Updates", L"Testing"}, {L"Updates", L"Starts"}, {L"Updates", L"Previous"}, {L"Updates", L"Removed"}};
	const std::wstring modIni = g_game + L"\\imperivm-mod.ini", gameIni = g_game + L"\\Settings.ini";
	std::wstring changed;
	bool copied = false;
	for (const auto &k : kResetKeys) {
		wchar_t buf[256];
		if (!Exists(modIni) || !GetPrivateProfileStringW(k[0], k[1], L"", buf, 256, modIni.c_str())) continue;
		if (!copied) copied = CopyToBackup(modIni, L"files\\imperivm-mod.ini");
		WritePrivateProfileStringW(k[0], k[1], nullptr, modIni.c_str());
		changed += (changed.empty() ? L"" : L", ") + std::wstring(k[1]);
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
	Say(changed.empty() ? T3(L"  ✓ La configuración de pantalla es correcta", L"  ✓ Display settings are correct", L"  ✓ Le impostazioni dello schermo sono corrette")
						: T3(L"  ✓ Configuración de pantalla restablecida (se usará la resolución del monitor): ", L"  ✓ Display settings reset (your monitor's resolution will be used): ",
							  L"  ✓ Impostazioni dello schermo ripristinate (verrà usata la risoluzione del monitor): ") +
							  changed);
}

void Warnings()
{
	if (FileSize(g_game + L"\\gbr.exe") != kSteamExeSize) {
		Say(T3(L"  ! gbr.exe no coincide con la versión de Steam. Si el juego falla, en Steam: clic derecho → Propiedades → Archivos instalados → Verificar integridad de los archivos del juego.",
			L"  ! gbr.exe does not match the Steam version. If the game fails, in Steam: right-click → Properties → Installed Files → Verify integrity of game files.",
			L"  ! gbr.exe non corrisponde alla versione di Steam. Se il gioco non funziona, in Steam: clic destro → Proprietà → File installati → Verifica integrità dei file di gioco."));
	}
	DWORD sac = 0, len = sizeof(sac);
	if (RegGetValueW(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\CI\\Policy", L"VerifiedAndReputablePolicyState", RRF_RT_REG_DWORD, nullptr, &sac, &len) == ERROR_SUCCESS && sac == 1) {
		Say(T3(L"  ! El Control inteligente de aplicaciones de Windows está activado y puede impedir que el mod se cargue.",
			L"  ! Windows Smart App Control is on and can prevent the mod from loading.",
			L"  ! Il Controllo intelligente delle app di Windows è attivo e può impedire il caricamento della mod."));
	}
	if (!ProcessRunning(L"steam.exe")) {
		Say(T3(L"  ! Steam no está abierto: ábrelo antes de jugar (es necesario para jugar en línea).", L"  ! Steam is not running: open it before playing (required for online play).",
			L"  ! Steam non è aperto: aprilo prima di giocare (è necessario per il gioco online)."));
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

void SetFolder(const std::wstring &game)
{
	g_game = RealCase(TrimSlashes(game));
	if (g_game.size() > 1 && g_game[1] == L':') g_game[0] = static_cast<wchar_t>(towupper(g_game[0]));
	SetWindowTextW(g_folder, g_game.empty() ? L"—" : g_game.c_str());
	EnableWindow(g_install, !g_game.empty() && !g_payload.empty());
	EnableWindow(g_change, TRUE);
	EnableWindow(g_close, TRUE);
	EnableWindow(g_language, TRUE);
	SetWindowTextW(g_install, !g_game.empty() && ModInstalled() ? T(kRepair) : T(kInstall));
	EnableWindow(g_uninstall, !g_game.empty() && !g_payload.empty() && ModInstalled());
}

void SetBusy()
{
	for (HWND b : {g_install, g_uninstall, g_play, g_change, g_close, g_language}) EnableWindow(b, FALSE);
}

void WriteLog()
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
	const std::wstring args = std::wstring(L"--elevated ") + action + L" \"" + g_game + L"\" \"" + TrimSlashes(g_localAppData) + L"\" " + kLangCodes[g_lang];
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
	g_acted = true;
	g_report.clear();
	g_backup.clear();
	SetBusy();
	Say(T3(L"Instalando Imperivm Resurrected ", L"Installing Imperivm Resurrected ", L"Installazione di Imperivm Resurrected ") + g_version + L"…");
	if (!WaitGameClosed()) {
		SetFolder(g_game);
		return;
	}
	bool layers = false;
	if (!g_elevated) layers = FixUserLayers();
	if (!g_elevated && !CanWrite(g_game)) {
		Say(T(kNeedAdmin));
		if (RelaunchElevated(L"install")) return;
		Say(T(kDenied));
		SetFolder(g_game);
		return;
	}
	const bool fresh = !ModInstalled();
	std::vector<std::wstring> staged;
	bool ok = StageFiles(staged);
	if (ok) {
		layers = FixMachineLayers() || layers;
		if (!layers && !g_elevated) Say(T3(L"  ✓ No hay modos de compatibilidad de Windows", L"  ✓ No Windows compatibility modes", L"  ✓ Nessuna modalità di compatibilità di Windows"));
		MoveForeignDlls();
		if (fresh) MoveForeignModFiles();
		FixVirtualStore();
		FixDisplay();
		ok = CommitFiles(staged);
		for (const wchar_t *name : {L"winmm.dll", L"steam_api.dll"}) Unblock(g_game + L"\\" + name);
		Warnings();
	}
	if (!g_backup.empty()) Say(T3(L"Copia de seguridad de los archivos modificados: ", L"Backup of changed files: ", L"Backup dei file modificati: ") + g_backup);
	Say(L"");
	Say(ok ? T(kDone) : T(kInstallFailed));
	WriteLog();
	SetFolder(g_game);
	EnableWindow(g_play, ok);
	SetFocus(ok ? g_play : g_close);
}

void DoUninstall()
{
	if (!g_elevated && MessageBoxW(g_wnd, T(kConfirmUninstall), T(kTitle), MB_OKCANCEL | MB_ICONQUESTION) != IDOK) return;
	g_acted = true;
	g_report.clear();
	SetBusy();
	Say(T3(L"Quitando Imperivm Resurrected…", L"Removing Imperivm Resurrected…", L"Rimozione di Imperivm Resurrected…"));
	if (!WaitGameClosed()) {
		SetFolder(g_game);
		return;
	}
	if (!g_elevated && !CanWrite(g_game)) {
		Say(T(kNeedAdmin));
		if (RelaunchElevated(L"uninstall")) return;
		Say(T(kDenied));
		SetFolder(g_game);
		return;
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
	Say(ok ? T(kUninstalled) : T(kUninstallFailed));
	WriteLog();
	SetFolder(g_game);
	EnableWindow(g_play, FALSE);
	SetFocus(g_close);
}

void ShowStatus()
{
	g_acted = false;
	g_report.clear();
	if (g_payload.empty()) Say(T(kBadPayload));
	else if (g_game.empty()) Say(T(kNotFound));
	else Say(ModInstalled() ? T(kReadyRepair) : T(kReadyInstall));
}

void ApplyLanguage()
{
	SetWindowTextW(g_wnd, (std::wstring(T(kTitle)) + (g_version.empty() ? L"" : L" " + g_version)).c_str());
	SetWindowTextW(g_intro, T(kIntro));
	SetWindowTextW(g_folderLabel, T(kFolderLabel));
	SetWindowTextW(g_change, T(kChange));
	SetWindowTextW(g_uninstall, T(kUninstall));
	SetWindowTextW(g_play, T(kPlay));
	SetWindowTextW(g_close, T(kClose));
	SetWindowTextW(g_install, !g_game.empty() && ModInstalled() ? T(kRepair) : T(kInstall));
	if (!g_acted) ShowStatus();
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
	if (!Exists(TrimSlashes(path) + L"\\gbr.exe")) {
		MessageBoxW(g_wnd, T(kNoGbr), T(kTitle), MB_OK | MB_ICONWARNING);
		return;
	}
	SetFolder(path);
	ShowStatus();
}

enum { kIdInstall = 101, kIdUninstall, kIdPlay, kIdClose, kIdChange, kIdLanguage };

const int kWidth = 640, kHeight = 440;

struct Placed {
	HWND hwnd;
	int x, y, w, h;
};

std::vector<Placed> g_children;
HFONT g_font = nullptr;

HWND Child(const wchar_t *cls, const wchar_t *text, DWORD style, int x, int y, int w, int h, int id)
{
	HWND c = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style, 0, 0, 0, 0, g_wnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
	g_children.push_back({c, x, y, w, h});
	return c;
}

void Layout(UINT dpi)
{
	NONCLIENTMETRICSW ncm = {sizeof(ncm)};
	SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0, dpi);
	ncm.lfMessageFont.lfHeight = -MulDiv(kFontPoints, static_cast<int>(dpi), 72);
	HFONT font = CreateFontIndirectW(&ncm.lfMessageFont);
	for (const Placed &p : g_children) {
		SetWindowPos(p.hwnd, nullptr, MulDiv(p.x, dpi, 96), MulDiv(p.y, dpi, 96), MulDiv(p.w, dpi, 96), MulDiv(p.h, dpi, 96), SWP_NOZORDER | SWP_NOACTIVATE);
		SendMessageW(p.hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
	}
	if (g_font) DeleteObject(g_font);
	g_font = font;
}

void Activate(HWND button, void (*action)())
{
	if (IsWindowEnabled(button)) action();
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_COMMAND:
		switch (LOWORD(wp)) {
		case IDOK:
		case kIdInstall: Activate(g_install, DoInstall); return 0;
		case kIdUninstall: Activate(g_uninstall, DoUninstall); return 0;
		case kIdChange: Activate(g_change, PickFolder); return 0;
		case kIdLanguage:
			if (HIWORD(wp) == CBN_SELCHANGE) {
				const LRESULT sel = SendMessageW(g_language, CB_GETCURSEL, 0, 0);
				if (sel >= kEs && sel <= kIt) g_lang = static_cast<Lang>(sel);
				ApplyLanguage();
			}
			return 0;
		case kIdPlay: ShellExecuteW(hwnd, L"open", (std::wstring(L"steam://rungameid/") + kAppId).c_str(), nullptr, nullptr, SW_SHOWNORMAL); return 0;
		case IDCANCEL:
		case kIdClose:
			if (IsWindowEnabled(g_close)) DestroyWindow(hwnd);
			return 0;
		}
		break;
	case DM_GETDEFID: return MAKELRESULT(kIdInstall, DC_HASDEFID);
	case WM_DPICHANGED: {
		const RECT *r = reinterpret_cast<const RECT *>(lp);
		SetWindowPos(hwnd, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top, SWP_NOZORDER | SWP_NOACTIVATE);
		Layout(HIWORD(wp));
		return 0;
	}
	case WM_CTLCOLORSTATIC:
		if (reinterpret_cast<HWND>(lp) == g_log || reinterpret_cast<HWND>(lp) == g_folder) break;
		SetBkMode(reinterpret_cast<HDC>(wp), TRANSPARENT);
		return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(hwnd, msg, wp, lp);
}

void PickLanguage()
{
	for (const LANGID id : {GetUserDefaultUILanguage(), LANGIDFROMLCID(GetUserDefaultLCID())}) {
		const LANGID lang = PRIMARYLANGID(id);
		if (lang == LANG_SPANISH || lang == LANG_ITALIAN || lang == LANG_ENGLISH) {
			g_lang = lang == LANG_SPANISH ? kEs : lang == LANG_ITALIAN ? kIt : kEn;
			return;
		}
	}
}

}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR, int)
{
	PickLanguage();
	INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_STANDARD_CLASSES};
	InitCommonControlsEx(&icc);
	CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

	int argc = 0;
	wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	std::wstring action, game;
	if (argv && argc >= 5 && !wcscmp(argv[1], L"--elevated")) {
		g_elevated = true;
		action = argv[2];
		game = argv[3];
		g_localAppData = argv[4];
		for (int i = 0; argc >= 6 && i < 3; i++) {
			if (!wcscmp(argv[5], kLangCodes[i])) g_lang = static_cast<Lang>(i);
		}
	}
	if (argv) LocalFree(argv);
	if (!g_elevated) {
		wchar_t local[MAX_PATH];
		if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, local))) g_localAppData = local;
	}

	const bool payload = LoadPayload();

	WNDCLASSEXW wc = {sizeof(wc)};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = inst;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
	LoadIconMetric(inst, MAKEINTRESOURCEW(1), LIM_LARGE, &wc.hIcon);
	LoadIconMetric(inst, MAKEINTRESOURCEW(1), LIM_SMALL, &wc.hIconSm);
	wc.lpszClassName = L"ImperivmResurrectedSetup";
	RegisterClassExW(&wc);
	const std::wstring title = std::wstring(T(kTitle)) + (g_version.empty() ? L"" : L" " + g_version);
	const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	g_wnd = CreateWindowExW(WS_EX_CONTROLPARENT, wc.lpszClassName, title.c_str(), style, CW_USEDEFAULT, CW_USEDEFAULT, kWidth, kHeight, nullptr, nullptr, inst, nullptr);
	const UINT dpi = GetDpiForWindow(g_wnd);
	RECT rc = {0, 0, MulDiv(kWidth, dpi, 96), MulDiv(kHeight, dpi, 96)};
	AdjustWindowRectExForDpi(&rc, style, FALSE, WS_EX_CONTROLPARENT, dpi);
	SetWindowPos(g_wnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);

	g_intro = Child(L"STATIC", T(kIntro), 0, 20, 16, 600, 64, 0);
	g_folderLabel = Child(L"STATIC", T(kFolderLabel), 0, 20, 90, 380, 18, 0);
	g_language = Child(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 522, 82, 98, 120, kIdLanguage);
	for (const wchar_t *name : kLangNames) SendMessageW(g_language, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name));
	SendMessageW(g_language, CB_SETCURSEL, g_lang, 0);
	g_folder = Child(L"EDIT", L"", ES_READONLY | ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, 20, 110, 494, 24, 0);
	g_change = Child(L"BUTTON", T(kChange), BS_PUSHBUTTON | WS_TABSTOP, 522, 109, 98, 26, kIdChange);
	g_log = Child(L"EDIT", L"", ES_READONLY | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | WS_BORDER | WS_TABSTOP, 20, 146, 600, 236, 0);
	g_install = Child(L"BUTTON", T(kInstall), BS_DEFPUSHBUTTON | WS_TABSTOP, 20, 396, 130, 30, kIdInstall);
	g_uninstall = Child(L"BUTTON", T(kUninstall), BS_PUSHBUTTON | WS_TABSTOP, 158, 396, 130, 30, kIdUninstall);
	g_play = Child(L"BUTTON", T(kPlay), BS_PUSHBUTTON | WS_TABSTOP, 352, 396, 150, 30, kIdPlay);
	g_close = Child(L"BUTTON", T(kClose), BS_PUSHBUTTON | WS_TABSTOP, 510, 396, 110, 30, kIdClose);
	Layout(dpi);
	EnableWindow(g_play, FALSE);

	ShowWindow(g_wnd, SW_SHOWNORMAL);
	UpdateWindow(g_wnd);

	if (!payload) {
		ShowStatus();
		EnableWindow(g_install, FALSE);
		EnableWindow(g_uninstall, FALSE);
		SetFocus(g_close);
	}
	else {
		SetFolder(game.empty() ? FindGame() : game);
		if (action.empty() || g_game.empty()) ShowStatus();
		SetFocus(g_game.empty() ? g_change : g_install);
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
	if (g_font) DeleteObject(g_font);
	CoUninitialize();
	return 0;
}
