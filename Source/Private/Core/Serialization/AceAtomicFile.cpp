#include "ArhqenCognitionEngine/Core/Serialization/AceAtomicFile.h"
#include "ArhqenCognitionEngine/Core/Identity/AceGuid.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <utility>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace am::core::serialization
{
    namespace
    {
        void fail(std::string* error, std::string message) { if (error) *error = std::move(message); }
    }

    bool AtomicFile::write(const std::filesystem::path& destination,
                           std::span<const std::uint8_t> bytes,
                           std::string* error)
    {
        if (error) error->clear();
        if (destination.empty() || destination.filename().empty()) { fail(error, "Destination filename is empty"); return false; }
        std::error_code ec;
        if (!destination.parent_path().empty()) std::filesystem::create_directories(destination.parent_path(), ec);
        if (ec) { fail(error, "Could not create destination directory"); return false; }

        const std::filesystem::path temporary = destination.parent_path() /
            (destination.filename().wstring() + L".tmp." + std::filesystem::path(Guid::create().toString()).wstring());

#ifdef _WIN32
        HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                                  FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_WRITE_THROUGH, nullptr);
        if (file == INVALID_HANDLE_VALUE) { fail(error, "Could not create temporary file"); return false; }
        bool ok = true;
        std::size_t offset = 0;
        while (offset < bytes.size())
        {
            const DWORD chunk = static_cast<DWORD>(std::min<std::size_t>(bytes.size() - offset, 1u << 30));
            DWORD written = 0;
            if (!WriteFile(file, bytes.data() + offset, chunk, &written, nullptr) || written != chunk) { ok = false; break; }
            offset += written;
        }
        if (ok && !FlushFileBuffers(file)) ok = false;
        CloseHandle(file);
        if (ok) ok = MoveFileExW(temporary.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
        if (!ok) { DeleteFileW(temporary.c_str()); fail(error, "Atomic file replacement failed"); }
        return ok;
#else
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        stream.flush();
        if (!stream) { stream.close(); std::filesystem::remove(temporary, ec); fail(error, "Temporary file write failed"); return false; }
        stream.close();
        std::filesystem::rename(temporary, destination, ec);
        if (ec) { std::filesystem::remove(temporary, ec); fail(error, "Atomic file replacement failed"); return false; }
        return true;
#endif
    }

    bool AtomicFile::read(const std::filesystem::path& source, std::vector<std::uint8_t>& bytes,
                          std::size_t maximumBytes, std::string* error)
    {
        if (error) error->clear();
        std::error_code ec;
        const std::uintmax_t length = std::filesystem::file_size(source, ec);
        if (ec) { fail(error, "Could not query file size"); return false; }
        if (length > maximumBytes || length > std::numeric_limits<std::size_t>::max()) { fail(error, "File exceeds read limit"); return false; }
        std::ifstream stream(source, std::ios::binary);
        if (!stream) { fail(error, "Could not open file"); return false; }
        std::vector<std::uint8_t> candidate(static_cast<std::size_t>(length));
        if (!candidate.empty()) stream.read(reinterpret_cast<char*>(candidate.data()), static_cast<std::streamsize>(candidate.size()));
        if (!stream && !candidate.empty()) { fail(error, "File read was incomplete"); return false; }
        bytes = std::move(candidate);
        return true;
    }
}
