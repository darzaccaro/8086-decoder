#pragma once
struct File {
    u8* data;
    u32 size;
    File(cstring path) {
        HANDLE fileHandle = CreateFile(path, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (fileHandle == INVALID_HANDLE_VALUE) {
            OutputDebugString("Failed to create file\n");
            Win32::ReportLastError();
        }
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
        success = CloseHandle(fileHandle);
        if (!success) {
            OutputDebugString("Failed to close file handle\n");
            Win32::ReportLastError();
            ASSERT(false);
        }
    }
    ~File() {
        delete data;
    }
    static void write(cstring path, std::string data) {
        HANDLE fileHandle = CreateFile(path, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (fileHandle == INVALID_HANDLE_VALUE) {
            OutputDebugString("Failed to create file\n");
            Win32::ReportLastError();
        }
        BOOL success = WriteFile(fileHandle, data.data(),(DWORD)data.size(), NULL, NULL);
        if (!success) {
            OutputDebugString("Failed to write file\n");
            Win32::ReportLastError();
            ASSERT(false);
        }
        success = CloseHandle(fileHandle);
        if (!success) {
            OutputDebugString("Failed to close file handle\n");
            Win32::ReportLastError();
            ASSERT(false);
        }
    }
};