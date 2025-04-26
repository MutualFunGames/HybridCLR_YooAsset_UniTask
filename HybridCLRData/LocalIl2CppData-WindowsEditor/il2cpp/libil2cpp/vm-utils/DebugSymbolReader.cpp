#include "il2cpp-config.h"

#include "os/File.h"
#include "os/Image.h"
#include "os/Path.h"
#include "utils/Logging.h"
#include "utils/Memory.h"
#include "utils/MemoryMappedFile.h"
#include "utils/PathUtils.h"
#include "utils/StringView.h"
#include "utils/Runtime.h"
#include "vm-utils/DebugSymbolReader.h"
#include "vm/GlobalMetadata.h"
#include "vm/Method.h"
#include "vm/Reflection.h"
#include <string>

#if IL2CPP_TARGET_ARM64E
#include <ptrauth.h>
#endif

namespace il2cpp
{
namespace utils
{
    struct usymliteHeader
    {
        uint32_t magic;
        uint32_t version;
        uint32_t lineCount;
        uint32_t id; // executable's id, offset in string table
        uint32_t os;
        uint32_t arch;
    };

    struct usymliteLine
    {
        uint64_t address;
        uint32_t methodIndex;
        uint32_t fileName; // Reference to the managed source file name in the string table
        uint32_t line; // Managed line number
        uint32_t parent;
    };

    struct Reader
    {
        void* debugSymbolData;
        const usymliteLine* lines;
        const char* strings;
        usymliteHeader header;
        std::string uuid;
        std::string os;
        std::string arch;
        uint64_t firstLineAddress;
        uint64_t lastLineAddress;
        uint32_t maxStringIndex;
    };

    static Reader s_usym = { 0 };

    const int headerSize = 24;
    const int lineSize = 24;
    const uint32_t magicUsymlite = 0x2D6D7973; // "sym-"
    const uint32_t noLine = 0xFFFFFFFF;

    static std::string GetArchFolder()
    {
#if IL2CPP_TARGET_ARM64
        return PathUtils::Combine(utils::Runtime::GetDataDir(), std::string("arm64"));
#elif IL2CPP_TARGET_X64
        return PathUtils::Combine(utils::Runtime::GetDataDir(), std::string("x64"));
#else
        return std::string("<NotImplemented>");
#endif
    }

    // Do a binary search to find the line with the given address
    // This is looking for the line with the closest address without going over (price is right style)
    static usymliteLine FindLine(uint64_t address)
    {
        uint32_t head = 0;
        uint32_t tail = s_usym.header.lineCount - 1;

        while (head < tail)
        {
            uint32_t mid = (head + tail + 1) / 2;
            uint64_t midAddr = s_usym.lines[mid].address;

            if (address < midAddr)
            {
                tail = mid - 1;
            }
            else
            {
                head = mid;
            }
        }

        uint64_t foundAddr = s_usym.lines[head].address;

        // Find the last entry with this address
        while (head + 1 < s_usym.header.lineCount && s_usym.lines[head + 1].address == foundAddr)
        {
            head += 1;
        }

        return s_usym.lines[head];
    }

    static const char* GetString(uint32_t index)
    {
        IL2CPP_ASSERT(index < s_usym.maxStringIndex);
        return s_usym.strings + index;
    }

#define IL2CPP_DEBUG_DUMP_USYM_DATA 0

#if IL2CPP_DEBUG_DUMP_USYM_DATA
    static void DumpUsymData()
    {
        // You may want to change this to be a full path so it is easy to locate.
        FILE* dumpFile = fopen("usymData.txt", "w");
        uint64_t imageBase = (uint64_t)os::Image::GetImageBase();
        for (uint32_t i = 0; i < s_usym.header.lineCount; i++)
        {
            if (s_usym.lines[i].methodIndex != noLine)
            {
                uint64_t address = s_usym.lines[i].address;
                void* actualAddress = (void*)(s_usym.lines[i].address + imageBase);
                const MethodInfo* methodInfo = vm::GlobalMetadata::GetMethodInfoFromMethodDefinitionIndex(s_usym.lines[i].methodIndex);
                uint32_t methodIndex = s_usym.lines[i].methodIndex;
                const char* filePath = GetString(s_usym.lines[i].fileName);
                uint32_t sourceCodeLineNumber = s_usym.lines[i].line;
                uint32_t parent = s_usym.lines[i].parent;

                if (methodInfo != NULL)
                    fprintf(dumpFile, "%d [%p, %llu] Method Index: %d %s %s(%d) parent: %d\n", i, actualAddress, address, methodIndex, vm::Method::GetFullName(methodInfo).c_str(), filePath, sourceCodeLineNumber, parent);
            }
        }
        fclose(dumpFile);
    }

#endif

    bool DebugSymbolReader::LoadDebugSymbols()
    {
        int error = 0;
        std::string symbolsPath;
        const StringView<char> symbolFileName = "il2cpp.usym";

        // First, look for the symbol file next to the executable.
        std::string applicationFolder = os::Path::GetApplicationFolder();
        if (!applicationFolder.empty())
            symbolsPath = PathUtils::Combine(applicationFolder, symbolFileName);

        os::FileHandle* symbolsFileHandle = NULL;
        if (!symbolsPath.empty())
            symbolsFileHandle = os::File::Open(symbolsPath.c_str(), kFileModeOpen, kFileAccessRead, kFileShareRead, kFileOptionsNone, &error);

        // (MacOS only) - Handle cases where the il2cpp.usym file's been dropped under an architecture specific (x64 or arm64) directory
        if (symbolsPath.empty() || error != 0)
        {
            std::string archFolder = GetArchFolder();
            if (!archFolder.empty())
                symbolsPath = PathUtils::Combine(archFolder, symbolFileName);

            if (!symbolsPath.empty())
                symbolsFileHandle = os::File::Open(symbolsPath.c_str(), kFileModeOpen, kFileAccessRead, kFileShareRead, kFileOptionsNone, &error);
        }

        // If we don't have a symbol path yet or there was some error opening the file next to the executable, try to
        // look in the data directory. For some platforms, the packaging won't allow the file to live next to the
        // executable.
        if (symbolsPath.empty() || error != 0)
        {
            symbolsPath = PathUtils::Combine(utils::Runtime::GetDataDir(), symbolFileName);
            symbolsFileHandle = os::File::Open(symbolsPath.c_str(), kFileModeOpen, kFileAccessRead, kFileShareRead, kFileOptionsNone, &error);
            if (error != 0)
                return false;
        }

        s_usym.debugSymbolData = utils::MemoryMappedFile::Map(symbolsFileHandle);
        int64_t length = os::File::GetLength(symbolsFileHandle, &error);

        os::File::Close(symbolsFileHandle, &error);
        if (error != 0)
        {
            utils::MemoryMappedFile::Unmap(s_usym.debugSymbolData);
            s_usym.debugSymbolData = NULL;
            return false;
        }

        s_usym.header = *(usymliteHeader *)((char *)s_usym.debugSymbolData);

        if (s_usym.header.magic != magicUsymlite || s_usym.header.lineCount == 0)
        {
            utils::MemoryMappedFile::Unmap(s_usym.debugSymbolData);
            s_usym.debugSymbolData = NULL;
            return false;
        }

        int64_t lineOffset = headerSize;
        int64_t stringOffset = lineOffset + (s_usym.header.lineCount * lineSize);

        s_usym.maxStringIndex = (uint32_t)(length - stringOffset);
        s_usym.lines = (const usymliteLine*)((const char *)s_usym.debugSymbolData + lineOffset);
        s_usym.strings = ((const char *)s_usym.debugSymbolData + stringOffset);

#if IL2CPP_ENABLE_NATIVE_INSTRUCTION_POINTER_EMISSION
        char* our_uuid = os::Image::GetImageUUID();
        s_usym.uuid = std::string(GetString(s_usym.header.id));

        if (our_uuid == NULL || s_usym.uuid != our_uuid)
        {
            // UUID mismatch means this usymfile is not for this program
            il2cpp::utils::Logging::Write("Ignoring symbol file due to UUID mismatch. File contains %s but expected %s.", s_usym.uuid.c_str(), our_uuid);
            utils::MemoryMappedFile::Unmap(s_usym.debugSymbolData);
            s_usym.debugSymbolData = NULL;
            s_usym.lines = NULL;
            s_usym.strings = NULL;
            return false;
        }

        IL2CPP_FREE(our_uuid);
#endif

        s_usym.os = std::string(GetString(s_usym.header.os));
        s_usym.arch = std::string(GetString(s_usym.header.arch));

        s_usym.firstLineAddress = s_usym.lines[0].address;
        s_usym.lastLineAddress = s_usym.lines[s_usym.header.lineCount - 1].address;

#if IL2CPP_DEBUG_DUMP_USYM_DATA
        DumpUsymData();
#endif
        return true;
    }

    static void InsertStackFrame(usymliteLine line, std::vector<Il2CppStackFrameInfo>* stackFrames)
    {
        if (line.parent != noLine)
        {
            InsertStackFrame(s_usym.lines[line.parent], stackFrames);
        }

        const MethodInfo* methodInfo = vm::GlobalMetadata::GetMethodInfoFromMethodDefinitionIndex(line.methodIndex);

        Il2CppStackFrameInfo frameInfo = { 0 };
        frameInfo.method = methodInfo;
        frameInfo.raw_ip = (uintptr_t)line.address;
        frameInfo.filePath = GetString(line.fileName);
        frameInfo.sourceCodeLineNumber = line.line;
        stackFrames->push_back(frameInfo);
    }

    // Gets the line information for the given address
    static bool GetUsymLine(void* address, usymliteLine& line)
    {
        if (s_usym.debugSymbolData == NULL || address == NULL)
        {
            return false;
        }

        // The instruction pointer points to the next address, so to get the address we came from, we subtract 1.
        // findLine matches the address to the closest address <= the one we give, so it finds the one we need
        uint64_t adjustedAddress = ((uint64_t)address) - ((uint64_t)os::Image::GetImageBase()) - 1;

#if IL2CPP_TARGET_ANDROID
        // We don't seem to need to subtract by one for Android
        // https://github.com/Unity-Technologies/unity-services-crash/commit/50611fcf29a1d876689942ed1f1cdca23e32c522
        adjustedAddress += 1;
#endif

#if IL2CPP_TARGET_ARM64E
        adjustedAddress = (uint64_t)ptrauth_strip((void*)adjustedAddress, ptrauth_key_return_address);
#endif

        // Quick check to remove anything outside the range
        if (adjustedAddress < s_usym.firstLineAddress || s_usym.lastLineAddress < adjustedAddress)
        {
            return false;
        }

        line = FindLine(adjustedAddress);

        // End of symbol entries are placed to indicate that we're past the end of a C# function.
        // These EOS entries have their Line and FileName set to 0xFFFFFFFF
        if (line.line == noLine)
        {
            return false;
        }

        return true;
    }

    bool DebugSymbolReader::GetSourceLocation(void* nativeInstructionPointer, SourceLocation& sourceLocation)
    {
        usymliteLine line;
        if (!GetUsymLine(nativeInstructionPointer, line))
        {
            return false;
        }

        sourceLocation.filePath = GetString(line.fileName);
        sourceLocation.lineNumber = line.line;

        return true;
    }

    bool DebugSymbolReader::AddStackFrames(void* nativeInstructionPointer, std::vector<Il2CppStackFrameInfo>* stackFrames)
    {
        usymliteLine line;
        if (!GetUsymLine(nativeInstructionPointer, line))
        {
            return false;
        }

        InsertStackFrame(line, stackFrames);

        return true;
    }

    bool DebugSymbolReader::DebugSymbolsAvailable()
    {
        /*
        #if IL2CPP_MONO_DEBUGGER
        return true;
        #else
        return s_usym.debugSymbolData != NULL;
        #endif
        */
        return true;
    }
} /* namespace utils */
} /* namespace il2cpp */
