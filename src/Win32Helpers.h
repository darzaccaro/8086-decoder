#pragma once
namespace Win32 {
    void ReportLastError() {
        DWORD errorCode = GetLastError();
        char errorString[256] = { 0 };
        DWORD errorStringSize = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errorCode, LANG_USER_DEFAULT, errorString, 256, NULL);
        if (errorStringSize <= 0) {
            OutputDebugString("Failed to get message for system error code");
        } else {
            OutputDebugString(errorString);
        }
    }
}