#pragma once

#include <cstdint>
#include <mutex>

#if defined(_WIN32)
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
#endif

namespace arranger
{
constexpr int maxSharedRegions = 256;
struct SharedRegion
{
    char track[96] {};
    char name[160] {};
    double startSeconds = 0.0;
    double durationSeconds = 0.0;
};

struct SharedMap
{
    std::uint32_t count = 0;
    std::uint32_t documents = 0;
    std::uint64_t revision = 0;
    SharedRegion regions[maxSharedRegions] {};
};

// The ARA Event FX and regular VST3 Hub are different DLLs. A normal C++
// singleton is not shared across them, so this small Windows-only bridge
// copies the current region snapshot through a named memory mapping.
class SharedArrangementMap final
{
public:
    static SharedArrangementMap& instance()
    {
        static SharedArrangementMap bridge;
        return bridge;
    }

    void publish(const SharedMap& map)
    {
#if defined(_WIN32)
        if (! ensure() || ! acquire(100))
            return;
        block->map = map;
        block->magic = magic;
        block->version = version;
        ReleaseMutex(mutex);
#else
        (void) map;
#endif
    }

    SharedMap read()
    {
        SharedMap result;
#if defined(_WIN32)
        if (! ensure() || ! acquire(10))
            return result;
        if (block->magic == magic && block->version == version)
            result = block->map;
        ReleaseMutex(mutex);
#endif
        return result;
    }

private:
    SharedArrangementMap() = default;
    ~SharedArrangementMap()
    {
#if defined(_WIN32)
        if (block != nullptr) UnmapViewOfFile(block);
        if (mapping != nullptr) CloseHandle(mapping);
        if (mutex != nullptr) CloseHandle(mutex);
#endif
    }
    SharedArrangementMap(const SharedArrangementMap&) = delete;
    SharedArrangementMap& operator=(const SharedArrangementMap&) = delete;

#if defined(_WIN32)
    struct Block
    {
        std::uint32_t magic = 0;
        std::uint32_t version = 0;
        SharedMap map;
    };
    static constexpr std::uint32_t magic = 0x414d4150; // AMAP
    static constexpr std::uint32_t version = 1;
    HANDLE mutex = nullptr;
    HANDLE mapping = nullptr;
    Block* block = nullptr;
    std::mutex initializationMutex;

    bool acquire(DWORD timeout) const
    {
        const auto result = WaitForSingleObject(mutex, timeout);
        return result == WAIT_OBJECT_0 || result == WAIT_ABANDONED;
    }

    bool ensure()
    {
        const std::scoped_lock lock(initializationMutex);
        if (block != nullptr) return true;
        // Called on ARA model updates or the Hub message thread, never audio.
        mutex = CreateMutexW(nullptr, FALSE, L"Local\\MoonRiverArrangerMapMutexV1");
        mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                     0, static_cast<DWORD>(sizeof(Block)), L"Local\\MoonRiverArrangerMapV1");
        if (mutex != nullptr && mapping != nullptr)
            block = static_cast<Block*>(MapViewOfFile(mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Block)));
        if (block != nullptr) return true;
        if (mapping != nullptr) { CloseHandle(mapping); mapping = nullptr; }
        if (mutex != nullptr) { CloseHandle(mutex); mutex = nullptr; }
        return false;
    }
#endif
};
}
