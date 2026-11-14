#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <cstdio>
#include <cstring>
#include <string>

namespace {

constexpr char kOriginalPayload[] = "Winamp file-sharing integration test";

bool WriteTextFile(const std::wstring &path, const char *payload) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;

    const DWORD length = static_cast<DWORD>(std::strlen(payload));
    DWORD written = 0;
    const BOOL ok = WriteFile(file, payload, length, &written, nullptr);
    CloseHandle(file);
    return ok && written == length;
}

bool CheckReadableHandle(HANDLE file, const char *expected) {
    const size_t expectedLength = std::strlen(expected);
    std::string buffer(expectedLength, '\0');
    LARGE_INTEGER start{};
    if (!SetFilePointerEx(file, start, nullptr, FILE_BEGIN)) return false;
    DWORD read = 0;
    if (!ReadFile(file, buffer.data(), static_cast<DWORD>(expectedLength), &read, nullptr))
        return false;
    return read == expectedLength && buffer == expected;
}

bool PathCannotBeOpenedForRead(const std::wstring &path) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return true;
    CloseHandle(file);
    return false;
}

bool PathDoesNotExist(const std::wstring &path) {
    SetLastError(ERROR_SUCCESS);
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes == INVALID_FILE_ATTRIBUTES && GetLastError() == ERROR_FILE_NOT_FOUND;
}

} // namespace

int wmain() {
    wchar_t tempDirectory[MAX_PATH]{};
    if (!GetTempPathW(MAX_PATH, tempDirectory)) {
        std::fwprintf(stderr, L"FAIL: GetTempPathW (error %lu)\n", GetLastError());
        return 1;
    }

    wchar_t tempFile[MAX_PATH]{};
    if (!GetTempFileNameW(tempDirectory, L"wfs", 0, tempFile)) {
        std::fwprintf(stderr, L"FAIL: GetTempFileNameW (error %lu)\n", GetLastError());
        return 1;
    }

    const std::wstring source = tempFile;
    const std::wstring renamed = source + L".renamed";
    const std::wstring moveDirectory = source + L".dir";
    const std::wstring moved = moveDirectory + L"\\moved.mp3";
    DeleteFileW(source.c_str());

    if (!CreateDirectoryW(moveDirectory.c_str(), nullptr)) {
        std::fwprintf(stderr, L"FAIL: CreateDirectoryW (error %lu)\n", GetLastError());
        return 1;
    }
    if (!WriteTextFile(source, kOriginalPayload)) {
        std::fwprintf(stderr, L"FAIL: could not create test file (error %lu)\n", GetLastError());
        RemoveDirectoryW(moveDirectory.c_str());
        return 1;
    }

    // Without FILE_SHARE_DELETE, both rename and deletion must be rejected
    // while the read handle is open.
    HANDLE restrictive = CreateFileW(source.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (restrictive == INVALID_HANDLE_VALUE) {
        std::fwprintf(stderr, L"FAIL: restrictive CreateFileW (error %lu)\n", GetLastError());
        DeleteFileW(source.c_str());
        RemoveDirectoryW(moveDirectory.c_str());
        return 1;
    }

    const BOOL renameWithoutShareDelete = MoveFileExW(source.c_str(), renamed.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    const BOOL deleteWithoutShareDelete = DeleteFileW(source.c_str());
    CloseHandle(restrictive);
    if (renameWithoutShareDelete || deleteWithoutShareDelete) {
        std::fwprintf(stderr,
            L"FAIL: rename/delete unexpectedly succeeded without FILE_SHARE_DELETE\n");
        DeleteFileW(source.c_str());
        DeleteFileW(renamed.c_str());
        RemoveDirectoryW(moveDirectory.c_str());
        return 1;
    }

    // With FILE_SHARE_DELETE, rename and a same-volume directory move should
    // both work while the original playback handle remains readable.
    HANDLE permissive = CreateFileW(source.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);
    if (permissive == INVALID_HANDLE_VALUE) {
        std::fwprintf(stderr, L"FAIL: permissive CreateFileW (error %lu)\n", GetLastError());
        DeleteFileW(source.c_str());
        RemoveDirectoryW(moveDirectory.c_str());
        return 1;
    }

    const BOOL renamedWhileOpen = MoveFileExW(source.c_str(), renamed.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    const bool readableAfterRename = renamedWhileOpen &&
        CheckReadableHandle(permissive, kOriginalPayload);

    const BOOL movedWhileOpen = renamedWhileOpen &&
        MoveFileExW(renamed.c_str(), moved.c_str(), MOVEFILE_WRITE_THROUGH);
    const bool readableAfterMove = movedWhileOpen &&
        CheckReadableHandle(permissive, kOriginalPayload);

    // DeleteFile marks the file for deletion. The path must no longer be
    // openable, but the already-open playback handle must remain usable.
    const BOOL deletedWhileOpen = movedWhileOpen && DeleteFileW(moved.c_str());
    const bool readableAfterDelete = deletedWhileOpen &&
        CheckReadableHandle(permissive, kOriginalPayload);
    const bool cannotReopenDeletedPath = deletedWhileOpen && PathCannotBeOpenedForRead(moved);
    CloseHandle(permissive);
    const bool deletedAfterLastClose = deletedWhileOpen && PathDoesNotExist(moved);

    if (!readableAfterRename || !readableAfterMove || !readableAfterDelete ||
        !cannotReopenDeletedPath || !deletedAfterLastClose) {
        std::fwprintf(stderr,
            L"FAIL: rename/move/delete semantics failed (error %lu)\n", GetLastError());
        DeleteFileW(source.c_str());
        DeleteFileW(renamed.c_str());
        DeleteFileW(moved.c_str());
        RemoveDirectoryW(moveDirectory.c_str());
        return 1;
    }

    // Simulate a tag editor that writes a temporary file and atomically replaces
    // a path that is still open for playback. The old handle must keep reading
    // the old file object, while new opens at that same pathname see new bytes.
    const std::wstring atomicSource = source + L".atomic";
    const std::wstring replacementCandidate = atomicSource + L".new";
    constexpr char replacementPayload[] = "Updated tags and metadata";
    const bool atomicSourceCreated = WriteTextFile(atomicSource, kOriginalPayload);
    HANDLE atomicPlaybackHandle = atomicSourceCreated
        ? CreateFileW(atomicSource.c_str(), GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)
        : INVALID_HANDLE_VALUE;
    const bool replacementCreated = WriteTextFile(replacementCandidate, replacementPayload);
    const BOOL replacementInstalled = atomicPlaybackHandle != INVALID_HANDLE_VALUE &&
        replacementCreated && MoveFileExW(replacementCandidate.c_str(), atomicSource.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);

    HANDLE newPathHandle = replacementInstalled
        ? CreateFileW(atomicSource.c_str(), GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)
        : INVALID_HANDLE_VALUE;
    const bool newPathHasReplacement = newPathHandle != INVALID_HANDLE_VALUE &&
        CheckReadableHandle(newPathHandle, replacementPayload);
    const bool oldHandleStillHasOriginal = atomicPlaybackHandle != INVALID_HANDLE_VALUE &&
        CheckReadableHandle(atomicPlaybackHandle, kOriginalPayload);

    if (newPathHandle != INVALID_HANDLE_VALUE) CloseHandle(newPathHandle);
    if (atomicPlaybackHandle != INVALID_HANDLE_VALUE) CloseHandle(atomicPlaybackHandle);

    if (!replacementInstalled || !newPathHasReplacement || !oldHandleStillHasOriginal) {
        std::fwprintf(stderr, L"FAIL: atomic replacement semantics failed (error %lu)\n",
                      GetLastError());
        DeleteFileW(atomicSource.c_str());
        DeleteFileW(replacementCandidate.c_str());
        RemoveDirectoryW(moveDirectory.c_str());
        return 1;
    }

    DeleteFileW(atomicSource.c_str());
    DeleteFileW(replacementCandidate.c_str());
    RemoveDirectoryW(moveDirectory.c_str());

    std::wprintf(L"PASS: without FILE_SHARE_DELETE rename and delete were blocked; with it, "
                 L"rename, same-volume move and delete succeeded while the old handle "
                 L"remained readable. The delete became final after the last close. "
                 L"Atomic replacement also preserved old-handle and new-path contents.\n");
    return 0;
}