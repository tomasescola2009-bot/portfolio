#include <windows.h>
#include <iostream>

const wchar_t* MAPPING_NAME = L"Local\\OverlayCoords";

int wmain(int argc, wchar_t** argv)
{
    if (argc < 3) {
        std::wcout << L"Usage: write_coords <x> <y>\n";
        return 1;
    }

    float x = static_cast<float>(_wtof(argv[1]));
    float y = static_cast<float>(_wtof(argv[2]));

    HANDLE hMap = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(float)*2, MAPPING_NAME);
    if (!hMap) {
        std::wcerr << L"CreateFileMapping failed: " << GetLastError() << L"\n";
        return 1;
    }

    void* p = MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, sizeof(float)*2);
    if (!p) {
        std::wcerr << L"MapViewOfFile failed: " << GetLastError() << L"\n";
        CloseHandle(hMap);
        return 1;
    }

    float* arr = (float*)p;
    arr[0] = x;
    arr[1] = y;

    std::wcout << L"Wrote coords: " << x << L", " << y << L"\n";

    UnmapViewOfFile(p);
    CloseHandle(hMap);
    return 0;
}
