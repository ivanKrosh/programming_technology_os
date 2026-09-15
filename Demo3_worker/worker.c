/* Демо 3. Об'єкт спостереження: програма, яку ми будемо моніторити.
   Робить видиме навантаження - відкриває дескриптори, виділяє пам'ять,
   читає файл - і сама реєструє кожен крок у трьох каналах:
      1) власний журнал demo03.log (працює завжди);
      2) OutputDebugString - системний канал відлагодження;
      3) консоль.

   Збірка: cl /utf-8 /W4 demo03_worker.c
   Запуск: demo03_worker.exe [секунд]     (типово 20)                  */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <TraceLoggingProvider.h>

TRACELOGGING_DEFINE_PROVIDER(
    g_provider,
    "Demo03.Worker",
    (0x7a4e1c21, 0x2d7a, 0x4d45, 0x9a, 0x11, 0x3b, 0x68, 0x4f, 0x0d, 0x7e, 0x21)
);

static FILE* g_log;

static void log_line(const char* level, const char* fmt, ...)
{
    char msg[512], line[640];
    va_list ap;
    SYSTEMTIME t;

    va_start(ap, fmt);
    _vsnprintf_s(msg, sizeof(msg), _TRUNCATE, fmt, ap);
    va_end(ap);

    GetLocalTime(&t);
    _snprintf_s(line, sizeof(line), _TRUNCATE,
        "%02d:%02d:%02d.%03d [%s] pid=%lu %s\n",
        t.wHour, t.wMinute, t.wSecond, t.wMilliseconds,
        level, GetCurrentProcessId(), msg);

    /*if (g_log) { fputs(line, g_log); fflush(g_log); }*/  /* канал 1: файл  */

    TraceLoggingWrite(
        g_provider,
        "LogLine",
        TraceLoggingString(level, "Level"),
        TraceLoggingString(msg, "Message")
    );                                                     /* ETW */

    OutputDebugStringA(line);                          /* канал 2: дебагер */
    fputs(line, stdout); fflush(stdout);               /* канал 3: консоль */
}

int main(int argc, char** argv)
{
    SetConsoleOutputCP(CP_UTF8);
    TraceLoggingRegister(g_provider);

    int seconds = (argc > 1) ? atoi(argv[1]) : 20;

    g_log = NULL;
    fopen_s(&g_log, "demo03.log", "a");

    HANDLE probe = CreateFileW(L"demo03_probe.tmp", GENERIC_WRITE, FILE_SHARE_READ,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
    if (probe != INVALID_HANDLE_VALUE) CloseHandle(probe);

    log_line("INFO", "старт, план роботи %d с", seconds);

    HANDLE  held[512];
    int     n_held = 0;
    void* blocks[64];
    int     n_blocks = 0;

    for (int step = 0; step < seconds; step++) {
        /* 1. відкриваємо дескриптори і НЕ закриваємо частину з них */
        for (int i = 0; i < 20; i++) {
            HANDLE h = CreateFileW(L"demo03_probe.tmp", GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL, OPEN_EXISTING, 0, NULL);
            if (h == INVALID_HANDLE_VALUE) {
                log_line("ERROR", "CreateFileW не вдався, код %lu", GetLastError());
                break;
            }
            if (i % 4 == 0 && n_held < 512) held[n_held++] = h;   /* лишаємо жити */
            else CloseHandle(h);
        }

        /* 2. виділяємо пам'ять і теж не всю повертаємо */
        if (n_blocks < 64) {
            void* p = VirtualAlloc(NULL, 1024 * 1024, MEM_COMMIT | MEM_RESERVE,
                PAGE_READWRITE);
            if (p) {
                memset(p, step & 0xFF, 1024 * 1024);   /* торкаємось сторінок */
                blocks[n_blocks++] = p;
            }
        }

        DWORD handles = 0;
        GetProcessHandleCount(GetCurrentProcess(), &handles);
        log_line("INFO", "крок %d: дескрипторів %lu, блоків пам'яті %d",
            step + 1, handles, n_blocks);

        Sleep(1000);
    }

    log_line("WARN", "завершення: не закрито %d дескрипторів, не звільнено %d МБ",
        n_held, n_blocks);

    for (int i = 0; i < n_held; i++)  CloseHandle(held[i]);
    for (int i = 0; i < n_blocks; i++) VirtualFree(blocks[i], 0, MEM_RELEASE);

    log_line("INFO", "прибрано, вихід");
    if (g_log) fclose(g_log);
    return 0;
}
