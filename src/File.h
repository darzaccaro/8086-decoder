#pragma once
struct File {
    u8* data;
    u32 size;
    File(cstring path) {
        HANDLE fileHandle = CreateFile(path, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        LARGE_INTEGER large_int = {0};
        BOOL success = GetFileSizeEx(fileHandle, &large_int);
        DWORD fileSizeHigh = 0;
        size = GetFileSize(fileHandle, &fileSizeHigh);
        if (!size) {
            OutputDebugString("Failed to get file size\n");
            Win32::ReportLastError();
            ASSERT(false);
        } else if (fileSizeHigh) {
            OutputDebugString("64 bit file sizes are not currently supported\n");
            ASSERT(false);
        }
        data = new u8[size];
        success = ReadFile(fileHandle, (void*)data, size, NULL, NULL);
        if (!success) {
            OutputDebugString("Failed to read file\n");
            Win32::ReportLastError();
            ASSERT(false);
        }
    }
    ~File() {
        delete data;
    }
};