/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * skyfire_launcher: starts the original 5.4.8 (18414) client and points it at a private
 * authserver without modifying anything on disk except the realmlist in WTF/Config.wtf.
 *
 * C++ port of the classic (non-Authnet) route of the SkyFire Authnet Launcher:
 *  - the hardcoded region logon hostnames are replaced in memory with the server address
 *  - the client's ws2_32 gethostbyname import is redirected to return the server address
 *  - the client's ws2_32 connect import is redirected so every IPv4 connection goes there
 *  - the login flow patches force the classic realmlist (grunt) route
 *
 * Source: https://github.com/ProjectSkyfire/launcher (src/SkyFireLauncher/Realm)
 */

#include <windows.h>
#include <tlhelp32.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace
{
    using Bytes = std::vector<uint8_t>;

    struct LoginFlowPatch
    {
        char const* Name;
        Bytes Pattern;      // 0x00 matches any byte
        Bytes Replacement;  // written at the start of the match
    };

    std::vector<LoginFlowPatch> const LoginFlowPatchesX64 =
    {
        { "Send",  { 0x44, 0x00, 0x44, 0x24, 0x50, 0x89, 0x00, 0x1C, 0x41, 0x00, 0xC0, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
                   { 0x45, 0x31, 0xC0, 0x90, 0x90 } },
        { "Email", { 0x74, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 },
                   { 0xEB } },
        { "User",  { 0x40, 0x00, 0x00, 0x00, 0x48, 0x8B, 0xCF, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x48, 0x85, 0xC0, 0x75, 0x00, 0x48, 0x8D },
                   { 0x00 } },
        { "RaF",   { 0xE8, 0x00, 0x00, 0x00, 0x00, 0x84, 0xC0, 0x74, 0x00, 0x48, 0x8D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x8B },
                   { 0x90, 0x90, 0x90, 0x90, 0x90 } },
        { "Rcv",   { 0x01, 0x0F, 0x85, 0x00, 0x00, 0x00, 0x00, 0x49, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4D, 0x00, 0x00, 0x4C, 0x00, 0x00 },
                   { 0xFF, 0x0F, 0x84 } },
    };

    std::vector<LoginFlowPatch> const LoginFlowPatchesX86 =
    {
        { "Send",  { 0x8B, 0x00, 0x00, 0x8B, 0x00, 0x81, 0x00, 0x48, 0x01 },
                   { 0x33, 0xC0, 0x90 } },
        { "Email", { 0x00, 0x00, 0x74, 0x00, 0x33, 0x00, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x75 },
                   { 0x90, 0x90, 0xEB } },
        { "User",  { 0x40, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x85, 0xC0, 0x75 },
                   { 0x00 } },
        { "RaF",   { 0xE8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0xC0, 0x74, 0x00, 0x8D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xB5, 0x00, 0x00, 0x00, 0x00, 0x8D },
                   { 0x90, 0x90, 0x90, 0x90, 0x90 } },
        { "Rcv",   { 0x01, 0x0F, 0x85, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x75, 0x14, 0x8B, 0x8B, 0x50, 0x45, 0x00, 0x00, 0xFF, 0x75, 0x10, 0x53, 0xFF, 0x75, 0x08, 0x6A, 0x17 },
                   { 0xFF, 0x0F, 0x84 } },
    };

    // the client's hardcoded region logon hostnames and what replaces each ({} = the server address)
    std::vector<std::pair<std::string, std::string>> const HostnameTargets =
    {
        { "us.logon.worldofwarcraft.com", "{}" },
        { "kr.logon.worldofwarcraft.com", "{}" },
        { "eu.logon.worldofwarcraft.com", "{}" },
        { "tw.logon.worldofwarcraft.com", "{}" },
        { "cn.logon.warcraftchina.com", "{}" },
        { "us.logon.worldofwarcraft.com:3724", "{}:3724" },
    };

    std::string Win32Error(std::string const& what)
    {
        std::ostringstream ss;
        ss << what << " (error " << GetLastError() << ")";
        return ss.str();
    }

    Bytes ReadFileBytes(fs::path const& path)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in)
            throw std::runtime_error("could not open " + path.string());

        return Bytes(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }

    // ---- PE import table (read from the file on disk: RVAs match the loaded image) ----

    uint16_t ReadU16(Bytes const& b, size_t o) { return uint16_t(b.at(o) | (b.at(o + 1) << 8)); }
    uint32_t ReadU32(Bytes const& b, size_t o) { return uint32_t(b.at(o)) | (uint32_t(b.at(o + 1)) << 8) | (uint32_t(b.at(o + 2)) << 16) | (uint32_t(b.at(o + 3)) << 24); }
    uint64_t ReadU64(Bytes const& b, size_t o) { return uint64_t(ReadU32(b, o)) | (uint64_t(ReadU32(b, o + 4)) << 32); }

    struct Section { uint32_t Rva; uint32_t Size; uint32_t FileOffset; };

    bool IsPe64Bit(Bytes const& module)
    {
        uint32_t peOffset = ReadU32(module, 0x3C);
        return ReadU16(module, peOffset + 4) == 0x8664; // IMAGE_FILE_MACHINE_AMD64
    }

    uint32_t RvaToFileOffset(std::vector<Section> const& sections, uint32_t rva)
    {
        for (Section const& section : sections)
            if (rva >= section.Rva && rva < section.Rva + section.Size)
                return section.FileOffset + (rva - section.Rva);

        return rva; // inside the headers
    }

    std::string ReadAscii(Bytes const& b, size_t o)
    {
        std::string s;
        while (o < b.size() && b[o])
            s.push_back(char(b[o++]));
        return s;
    }

    /// RVA of the IAT slot of dllName!functionName (or its ordinal); 0 if not imported
    uint32_t FindIatSlotRva(Bytes const& module, char const* dllName, char const* functionName, uint16_t knownOrdinal)
    {
        uint32_t fileHeader = ReadU32(module, 0x3C) + 4;
        bool is64Bit = ReadU16(module, fileHeader) == 0x8664;
        uint16_t numberOfSections = ReadU16(module, fileHeader + 2);
        uint16_t sizeOfOptionalHeader = ReadU16(module, fileHeader + 16);
        uint32_t optionalHeader = fileHeader + 20;
        bool isPe32Plus = ReadU16(module, optionalHeader) == 0x20B;
        if (isPe32Plus != is64Bit)
            throw std::runtime_error("PE machine type and optional header magic disagree");

        std::vector<Section> sections;
        uint32_t sectionTable = optionalHeader + sizeOfOptionalHeader;
        for (uint16_t i = 0; i < numberOfSections; ++i)
        {
            uint32_t entry = sectionTable + i * 40;
            uint32_t virtualSize = ReadU32(module, entry + 8);
            uint32_t virtualAddress = ReadU32(module, entry + 12);
            uint32_t sizeOfRawData = ReadU32(module, entry + 16);
            uint32_t pointerToRawData = ReadU32(module, entry + 20);
            sections.push_back({ virtualAddress, std::max(virtualSize, sizeOfRawData), pointerToRawData });
        }

        uint32_t dataDirectory = optionalHeader + (isPe32Plus ? 112 : 96);
        uint32_t descriptorRva = ReadU32(module, dataDirectory + 1 * 8); // DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
        uint32_t const entrySize = isPe32Plus ? 8 : 4;
        uint64_t const ordinalFlag = isPe32Plus ? (uint64_t(1) << 63) : (uint64_t(1) << 31);

        for (;; descriptorRva += 20)
        {
            uint32_t descriptor = RvaToFileOffset(sections, descriptorRva);
            uint32_t originalFirstThunk = ReadU32(module, descriptor + 0);
            uint32_t nameRva = ReadU32(module, descriptor + 12);
            uint32_t firstThunk = ReadU32(module, descriptor + 16);
            if (!originalFirstThunk && !nameRva && !firstThunk)
                break;

            if (_stricmp(ReadAscii(module, RvaToFileOffset(sections, nameRva)).c_str(), dllName) != 0)
                continue;

            uint32_t nameThunk = originalFirstThunk ? originalFirstThunk : firstThunk;
            for (uint32_t index = 0;; ++index)
            {
                uint32_t thunkOffset = RvaToFileOffset(sections, nameThunk + index * entrySize);
                uint64_t thunk = isPe32Plus ? ReadU64(module, thunkOffset) : ReadU32(module, thunkOffset);
                if (!thunk)
                    break;

                if (!(thunk & ordinalFlag))
                {
                    // IMAGE_IMPORT_BY_NAME { WORD Hint; CHAR Name[]; }
                    if (ReadAscii(module, RvaToFileOffset(sections, uint32_t(thunk)) + 2) == functionName)
                        return firstThunk + index * entrySize;
                }
                else if (knownOrdinal && (thunk & 0xFFFF) == knownOrdinal)
                    return firstThunk + index * entrySize; // classic Winsock imports are by ordinal
            }
        }

        return 0;
    }

    // ---- remote process helpers ----

    void WriteRemote(HANDLE process, uintptr_t address, void const* data, size_t size)
    {
        DWORD oldProtect = 0;
        if (!VirtualProtectEx(process, reinterpret_cast<LPVOID>(address), size, PAGE_EXECUTE_READWRITE, &oldProtect))
            throw std::runtime_error(Win32Error("could not unprotect client memory"));

        BOOL written = WriteProcessMemory(process, reinterpret_cast<LPVOID>(address), data, size, nullptr);
        VirtualProtectEx(process, reinterpret_cast<LPVOID>(address), size, oldProtect, &oldProtect);
        if (!written)
            throw std::runtime_error(Win32Error("could not write client memory"));
    }

    uintptr_t AllocRemote(HANDLE process, size_t size)
    {
        LPVOID memory = VirtualAllocEx(process, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!memory)
            throw std::runtime_error(Win32Error("could not allocate memory in the client process"));
        return reinterpret_cast<uintptr_t>(memory);
    }

    void PutPointer(Bytes& b, size_t o, uint64_t value, bool is64Bit)
    {
        for (size_t i = 0; i < (is64Bit ? 8u : 4u); ++i)
            b[o + i] = uint8_t(value >> (8 * i));
    }

    std::pair<uintptr_t, size_t> GetMainModule(DWORD processId, std::wstring const& exeName)
    {
        // a 64-bit launcher needs TH32CS_SNAPMODULE32 to see a 32-bit client's modules; the module list
        // is not always available right after CreateProcess, so retry briefly
        for (int attempt = 0; attempt < 100; ++attempt)
        {
            if (attempt)
                std::this_thread::sleep_for(std::chrono::milliseconds(50));

            HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
            if (snapshot == INVALID_HANDLE_VALUE)
                continue;

            MODULEENTRY32W entry{};
            entry.dwSize = sizeof(entry);
            if (Module32FirstW(snapshot, &entry))
            {
                do
                {
                    if (_wcsicmp(entry.szModule, exeName.c_str()) == 0)
                    {
                        CloseHandle(snapshot);
                        return { reinterpret_cast<uintptr_t>(entry.modBaseAddr), entry.modBaseSize };
                    }
                } while (Module32NextW(snapshot, &entry));
            }

            CloseHandle(snapshot);
        }

        throw std::runtime_error("could not find the client executable module in the client process");
    }

    long FindPattern(Bytes const& haystack, Bytes const& pattern, size_t start, bool wildcard)
    {
        if (pattern.size() > haystack.size())
            return -1;

        for (size_t i = start; i + pattern.size() <= haystack.size(); ++i)
        {
            bool match = true;
            for (size_t j = 0; j < pattern.size() && match; ++j)
                if (!(wildcard && pattern[j] == 0) && haystack[i + j] != pattern[j])
                    match = false;

            if (match)
                return long(i);
        }

        return -1;
    }

    /// gethostbyname -> returns a fixed hostent for the server address, whatever name is asked for
    void InstallResolverHook(HANDLE process, uintptr_t base, Bytes const& file, bool is64Bit, std::array<uint8_t, 4> const& ip, std::string const& name)
    {
        uint32_t slotRva = FindIatSlotRva(file, "ws2_32.dll", "gethostbyname", 52);
        if (!slotRva)
            throw std::runtime_error("could not find the gethostbyname import in the client");

        size_t const ptrSize = is64Bit ? 8 : 4;
        size_t const shellcodeSize = is64Bit ? 16 : 8;
        size_t const hostentSize = is64Bit ? 32 : 16;
        size_t const hostentOffset = (shellcodeSize + ptrSize - 1) / ptrSize * ptrSize;
        size_t const addrListOffset = hostentOffset + hostentSize;
        size_t const ipOffset = addrListOffset + 2 * ptrSize;
        size_t const nameOffset = ipOffset + 4;

        Bytes block(nameOffset + name.size() + 1, 0);
        std::copy(ip.begin(), ip.end(), block.begin() + ipOffset);
        std::copy(name.begin(), name.end(), block.begin() + nameOffset);

        uintptr_t remote = AllocRemote(process, block.size());

        // struct hostent { char* h_name; char** h_aliases; short h_addrtype; short h_length; char** h_addr_list; }
        PutPointer(block, hostentOffset, remote + nameOffset, is64Bit);
        PutPointer(block, hostentOffset + ptrSize, 0, is64Bit);
        block[hostentOffset + 2 * ptrSize] = 2; // AF_INET
        block[hostentOffset + 2 * ptrSize + 2] = 4;
        PutPointer(block, hostentOffset + (is64Bit ? 24 : 12), remote + addrListOffset, is64Bit);
        PutPointer(block, addrListOffset, remote + ipOffset, is64Bit);
        PutPointer(block, addrListOffset + ptrSize, 0, is64Bit);

        if (is64Bit)
        {
            block[0] = 0x48; block[1] = 0xB8;                       // mov rax, imm64
            PutPointer(block, 2, remote + hostentOffset, true);
            block[10] = 0xC3;                                       // ret
        }
        else
        {
            block[0] = 0xB8;                                        // mov eax, imm32
            PutPointer(block, 1, remote + hostentOffset, false);
            block[5] = 0xC2; block[6] = 0x04; block[7] = 0x00;      // ret 4 (stdcall)
        }

        WriteRemote(process, remote, block.data(), block.size());

        uint64_t hook = remote;
        WriteRemote(process, base + slotRva, &hook, ptrSize);
    }

    /// connect -> rewrites the IPv4 destination address to the server, then calls the original connect
    void InstallConnectRedirect(HANDLE process, uintptr_t base, Bytes const& file, bool is64Bit, std::array<uint8_t, 4> const& ip)
    {
        uint32_t slotRva = FindIatSlotRva(file, "ws2_32.dll", "connect", 4);
        if (!slotRva)
            throw std::runtime_error("could not find the connect import in the client");

        size_t const ptrSize = is64Bit ? 8 : 4;
        uint64_t original = 0;
        if (!ReadProcessMemory(process, reinterpret_cast<LPCVOID>(base + slotRva), &original, ptrSize, nullptr))
            throw std::runtime_error(Win32Error("could not read the client's connect import"));

        Bytes code;
        if (is64Bit)
        {
            code = { 0x66, 0x83, 0x3A, 0x02,                    // cmp word ptr [rdx], AF_INET
                     0x75, 0x07,                                // jne original
                     0xC7, 0x42, 0x04, 0, 0, 0, 0,              // mov dword ptr [rdx+4], ip
                     0x48, 0xB8, 0, 0, 0, 0, 0, 0, 0, 0,        // mov rax, original
                     0xFF, 0xE0 };                              // jmp rax
            std::copy(ip.begin(), ip.end(), code.begin() + 9);
            PutPointer(code, 15, original, true);
        }
        else
        {
            code = { 0x8B, 0x44, 0x24, 0x08,                    // mov eax, [esp+8]
                     0x66, 0x83, 0x38, 0x02,                    // cmp word ptr [eax], AF_INET
                     0x75, 0x07,                                // jne original
                     0xC7, 0x40, 0x04, 0, 0, 0, 0,              // mov dword ptr [eax+4], ip
                     0xB8, 0, 0, 0, 0,                          // mov eax, original
                     0xFF, 0xE0 };                              // jmp eax
            std::copy(ip.begin(), ip.end(), code.begin() + 13);
            PutPointer(code, 18, original, false);
        }

        uintptr_t remote = AllocRemote(process, code.size());
        WriteRemote(process, remote, code.data(), code.size());

        uint64_t hook = remote;
        WriteRemote(process, base + slotRva, &hook, ptrSize);
    }

    void PatchClient(PROCESS_INFORMATION const& pi, fs::path const& exe, Bytes const& file, bool is64Bit, std::string const& address, std::array<uint8_t, 4> const& ip)
    {
        auto [base, size] = GetMainModule(pi.dwProcessId, exe.filename().wstring());

        Bytes image(size);
        if (!ReadProcessMemory(pi.hProcess, reinterpret_cast<LPCVOID>(base), image.data(), size, nullptr))
            throw std::runtime_error(Win32Error("could not read the client's memory"));

        for (auto const& [hostname, format] : HostnameTargets)
        {
            Bytes pattern(hostname.begin(), hostname.end());
            pattern.push_back(0);

            std::string replacementText = format;
            replacementText.replace(replacementText.find("{}"), 2, address);
            Bytes replacement(replacementText.begin(), replacementText.end());
            replacement.push_back(0);
            if (replacement.size() > pattern.size())
                throw std::runtime_error("the server address is longer than the client's logon hostname");

            for (long offset = FindPattern(image, pattern, 0, false); offset >= 0; offset = FindPattern(image, pattern, size_t(offset) + pattern.size(), false))
                WriteRemote(pi.hProcess, base + offset, replacement.data(), replacement.size());
        }

        InstallResolverHook(pi.hProcess, base, file, is64Bit, ip, address);
        InstallConnectRedirect(pi.hProcess, base, file, is64Bit, ip);

        for (LoginFlowPatch const& patch : is64Bit ? LoginFlowPatchesX64 : LoginFlowPatchesX86)
        {
            long offset = FindPattern(image, patch.Pattern, 0, true);
            if (offset < 0)
            {
                std::cout << "  " << patch.Name << ": pattern not found, skipped\n";
                continue;
            }

            WriteRemote(pi.hProcess, base + offset, patch.Replacement.data(), patch.Replacement.size());
            std::cout << "  " << patch.Name << ": patched\n";
        }
    }

    void SetRealmlist(fs::path const& clientDir, std::string const& address)
    {
        fs::path wtf = clientDir / "WTF";
        fs::create_directories(wtf);
        fs::path config = wtf / "Config.wtf";

        std::vector<std::string> lines;
        if (std::ifstream in{ config })
            for (std::string line; std::getline(in, line);)
                lines.push_back(line);

        std::string const wanted = "SET realmlist \"" + address + "\"";
        auto isRealmlist = [](std::string const& line)
        {
            std::string lower = line.substr(line.find_first_not_of(" \t") == std::string::npos ? line.size() : line.find_first_not_of(" \t"));
            std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return char(std::tolower(c)); });
            return lower.rfind("set realmlist ", 0) == 0;
        };

        auto itr = std::find_if(lines.begin(), lines.end(), isRealmlist);
        if (itr != lines.end())
        {
            if (*itr == wanted)
                return;
            *itr = wanted;
        }
        else
            lines.insert(lines.begin(), wanted);

        std::ofstream out(config, std::ios::trunc);
        for (std::string const& line : lines)
            out << line << "\n";
    }

    void PrintUsage()
    {
        std::cout <<
            "Usage: skyfire_launcher [options] [-- client arguments]\n"
            "  --address <ipv4>   authserver address (default 127.0.0.1)\n"
            "  --path <dir>       5.4.8 client folder (default: current folder)\n"
            "  --32 | --64        client to start: Wow.exe or Wow-64.exe (default: the launcher's bitness)\n";
    }
}

int main(int argc, char** argv)
{
    std::cout << "skyfire_launcher - starts the 5.4.8 (18414) client against a private server\n";

    std::string address = "127.0.0.1";
    fs::path clientDir = fs::current_path();
    bool want64 = sizeof(void*) == 8;
    std::string clientArgs;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--address" && i + 1 < argc)
            address = argv[++i];
        else if (arg == "--path" && i + 1 < argc)
            clientDir = argv[++i];
        else if (arg == "--32")
            want64 = false;
        else if (arg == "--64")
            want64 = true;
        else if (arg == "--help" || arg == "-h")
        {
            PrintUsage();
            return 0;
        }
        else if (arg == "--")
        {
            for (++i; i < argc; ++i)
                clientArgs += std::string(" ") + argv[i];
        }
        else
        {
            std::cerr << "unknown argument " << arg << "\n";
            PrintUsage();
            return 1;
        }
    }

    try
    {
        if (want64 && sizeof(void*) == 4)
            throw std::runtime_error("a 32-bit launcher cannot patch Wow-64.exe; use the 64-bit build");

        std::array<uint8_t, 4> ip{};
        {
            unsigned a, b, c, d;
            char tail;
            if (std::sscanf(address.c_str(), "%u.%u.%u.%u%c", &a, &b, &c, &d, &tail) != 4 || a > 255 || b > 255 || c > 255 || d > 255)
                throw std::runtime_error("--address must be an IPv4 address");
            ip = { uint8_t(a), uint8_t(b), uint8_t(c), uint8_t(d) };
        }

        fs::path exe = clientDir / (want64 ? "Wow-64.exe" : "Wow.exe");
        if (!fs::exists(exe))
            throw std::runtime_error(exe.string() + " not found (use --path)");

        Bytes file = ReadFileBytes(exe);
        bool is64Bit = IsPe64Bit(file);

        SetRealmlist(clientDir, address);

        std::string commandLine = "\"" + exe.string() + "\"" + clientArgs;
        STARTUPINFOA si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};
        // not CREATE_SUSPENDED: a suspended 32-bit (WOW64) client cannot list its modules yet; patching right
        // after start is still well ahead of the client's first logon attempt
        if (!CreateProcessA(exe.string().c_str(), commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, clientDir.string().c_str(), &si, &pi))
            throw std::runtime_error(Win32Error("could not start " + exe.string()));

        try
        {
            std::cout << "Patching " << exe.filename().string() << " (" << (is64Bit ? "64" : "32") << "-bit) for " << address << "\n";
            PatchClient(pi, exe, file, is64Bit, address, ip);
        }
        catch (...)
        {
            // never leave an unpatched client connecting to the real logon servers
            TerminateProcess(pi.hProcess, 1);
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
            throw;
        }

        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        std::cout << "Done.\n";
        return 0;
    }
    catch (std::exception const& e)
    {
        std::cerr << "ERROR: " << e.what() << "\nPress ENTER to continue...";
        std::cin.get();
        return 1;
    }
}
