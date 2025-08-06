#include <windows.h>
#include <iostream>
#include <string>
#include <cmath>
#include "mem/mem.h"
#include "update.h"

// Constants
constexpr uint64_t PLACE_ID = 17574618959; // CHANGE THIS TO YOUR ACTUAL PLACEID FOR THE PLACEID OFFSET!
constexpr const char* ROBLOX_PROCESS_NAME = "RobloxPlayerBeta.exe";
constexpr const char* ROBLOX_WINDOW_TITLE = "Roblox";
constexpr const char* DATA_MODEL_CLASS_NAME = "DataModel";

BYTE* g_baseAddress = nullptr;

bool AttachToRobloxProcess();
uintptr_t GetDataModelAddress();
void FindAndPrintOffsets(uintptr_t dataModel);

int main()
{
    SetConsoleTitle("Roblox Offset Dumper - Base by Henrick - Modified by Eelb");

    if (!AttachToRobloxProcess())
    {
        std::cerr << "Failed to attach to Roblox process." << std::endl;
        return EXIT_FAILURE;
    }

    const uintptr_t dataModel = GetDataModelAddress();
    if (!dataModel)
    {
        std::cerr << "Failed to find DataModel address." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "[+] DataModel address: 0x" << std::hex << dataModel << std::dec << std::endl;

    FindAndPrintOffsets(dataModel);

    std::cout << "Press any key to exit..." << std::endl;
    std::cin.get();

    return EXIT_SUCCESS;
}

bool AttachToRobloxProcess()
{
    DWORD processId = 0;
    HWND robloxWindow = FindWindowA(nullptr, ROBLOX_WINDOW_TITLE);

    if (!robloxWindow)
    {
        std::cerr << "Roblox window not found." << std::endl;
        return false;
    }

    GetWindowThreadProcessId(robloxWindow, &processId);
    g_procHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);

    if (!g_procHandle)
    {
        std::cerr << "Failed to open Roblox process." << std::endl;
        return false;
    }

    g_baseAddress = memory->LocateModuleBase(processId, ROBLOX_PROCESS_NAME);
    if (!g_baseAddress)
    {
        CloseHandle(g_procHandle);
        std::cerr << "Failed to find Roblox base address." << std::endl;
        return false;
    }

    std::cout << "[+] Found Roblox base address: 0x" << std::hex
        << reinterpret_cast<uintptr_t>(g_baseAddress) << std::dec << std::endl;

    return true;
}

uintptr_t GetDataModelAddress()
{
    const uintptr_t fakeDataModel = memory->read<uintptr_t>(
        reinterpret_cast<uintptr_t>(g_baseAddress) + offsets::FakeDataModelPointer);

    return fakeDataModel + offsets::FakeDataModelToRealDatamodel;
}

void FindAndPrintOffsets(uintptr_t dataModel)
{
    // Find name offset
    uintptr_t nameOffset = 0;
    while (true)
    {
        const uintptr_t namePtr = memory->read<uintptr_t>(dataModel + nameOffset);
        const std::string name = memory->readstring(namePtr);

        if (name == "Ugc" || name == "LuaApp")
        {
            std::cout << "[+] Name offset: 0x" << std::hex << (nameOffset + 0x8) << std::dec << std::endl;
            break;
        }
        nameOffset++;
    }

    // Find PlaceId offset
    uintptr_t placeIdOffset = 0;
    while (true)
    {
        const uint64_t placeId = memory->read<uint64_t>(dataModel + placeIdOffset);
        if (placeId == PLACE_ID)
        {
            std::cout << "[+] PlaceId offset: 0x" << std::hex << (placeIdOffset + 0x8) << std::dec << std::endl;
            break;
        }
        placeIdOffset++;
    }

    // Find game loaded offset
    uintptr_t gameLoadedOffset = 0;
    while (true)
    {
        const int64_t loaded = memory->read<int64_t>(dataModel + gameLoadedOffset);
        if (loaded == 31)
        {
            std::cout << "[+] GameLoaded offset: 0x" << std::hex << (gameLoadedOffset + 0x8) << std::dec << std::endl;
            break;
        }
        gameLoadedOffset++;
    }

    // Find class descriptor offset
    uintptr_t classDescOffset = 0;
    while (true)
    {
        const uintptr_t classNamePtr = memory->read<uintptr_t>(
            memory->read<uintptr_t>(dataModel + classDescOffset) + 0x8);
        const std::string className = memory->readstring(classNamePtr);

        if (className == DATA_MODEL_CLASS_NAME)
        {
            std::cout << "[+] ClassDescriptor offset: 0x" << std::hex << (classDescOffset + 0x8) << std::dec << std::endl;
            break;
        }
        classDescOffset++;
    }

    std::cout << "[+] ClassDescriptorToClassName offset: 0x8" << std::endl;
}