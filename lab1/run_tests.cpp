#include <windows.h>
#include <psapi.h>
#include <cstdlib>
#include <cstring>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc == 1) {
        return std::system(
            "powershell.exe -NoProfile -NonInteractive -Command \""
            "$ErrorActionPreference = 'Stop'; "
            "$times = @(); $peaks = @(); "
            "$rows = foreach ($i in 1..7) { "
                "$elapsed = Measure-Command { $bytes = .\\run_tests.exe --once }; "
                "if ($LASTEXITCODE -ne 0) { exit 1 }; "
                "$ms = $elapsed.TotalMilliseconds; $mb = [long]$bytes / 1000000; "
                "$times += $ms; $peaks += $mb; "
                "'Run {0}: {1:F3} ms, {2:F3} MB' -f $i,$ms,$mb "
            "}; "
            "$rows += 'Median: {0:F3} ms, {1:F3} MB' -f "
                "($times | Sort-Object)[3],($peaks | Sort-Object)[3]; "
            "$rows | Set-Content -Encoding ASCII benchmark_result.txt; $rows\""
        ) != 0;
    }

    if (argc != 2 || std::strcmp(argv[1], "--once") != 0) {
        std::cerr << "Usage: run_tests.exe\n";
        return 1;
    }
    SECURITY_ATTRIBUTES inherit = {sizeof(inherit), nullptr, TRUE};
    HANDLE input = CreateFileA("tests_short.txt", GENERIC_READ, FILE_SHARE_READ,
                               &inherit, OPEN_EXISTING, 0, nullptr);
    HANDLE output = CreateFileA("NUL", GENERIC_WRITE, FILE_SHARE_WRITE,
                                &inherit, OPEN_EXISTING, 0, nullptr);
    STARTUPINFOA startup = {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = input;
    startup.hStdOutput = output;
    startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    PROCESS_INFORMATION process = {};
    bool started = input != INVALID_HANDLE_VALUE && output != INVALID_HANDLE_VALUE &&
        CreateProcessA(".\\main.exe", nullptr, nullptr, nullptr, TRUE, 0,
                       nullptr, nullptr, &startup, &process);
    if (input != INVALID_HANDLE_VALUE) CloseHandle(input);
    if (output != INVALID_HANDLE_VALUE) CloseHandle(output);
    if (!started) {
        std::cerr << "Cannot run main.exe with tests_short.txt\n";
        return 1;
    }

    CloseHandle(process.hThread);
    DWORD code = 0;
    PROCESS_MEMORY_COUNTERS memory = {};
    bool ok = WaitForSingleObject(process.hProcess, INFINITE) == WAIT_OBJECT_0 &&
              GetExitCodeProcess(process.hProcess, &code) && code == 0 &&
              K32GetProcessMemoryInfo(process.hProcess, &memory, sizeof(memory));
    CloseHandle(process.hProcess);
    if (!ok) {
        std::cerr << "Dictionary failed or memory measurement failed\n";
        return 1;
    }
    std::cout << memory.PeakWorkingSetSize << '\n';
    return 0;
}


/*
g++ -std=c++11 -O2 main.cpp -o main.exe
g++ -std=c++11 -O2 run_tests.cpp -o run_tests.exe
.\run_tests.exe
*/