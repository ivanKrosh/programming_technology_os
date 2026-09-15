/* Демо 4. Власний приймач OutputDebugString - "міні-DebugView" на 60 рядків.
   Показує, що системний канал відлагодження - це не магія, а три об'єкти
   ядра: спільна пам'ять і дві події. Ті самі об'єкти будуть у Розділі II.

   Збірка: cl /utf-8 /W4 demo04_dbgmon.c
   Запуск: demo04_dbgmon.exe   (в іншому вікні - будь-яка програма,
                                що кличе OutputDebugString)            */
#include <windows.h>
#include <stdio.h>
#include <string.h>

#pragma pack(push, 1)
struct dbwin {              /* формат буфера, узгоджений із самою Windows */
    DWORD pid;              /* хто написав */
    char  text[4096 - sizeof(DWORD)];
};
#pragma pack(pop)

int main(void)
{
    SetConsoleOutputCP(CP_UTF8);

    HANDLE ready = CreateEventW(NULL, FALSE, FALSE, L"DBWIN_BUFFER_READY");
    HANDLE data = CreateEventW(NULL, FALSE, FALSE, L"DBWIN_DATA_READY");
    HANDLE mem = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
        0, 4096, L"DBWIN_BUFFER");
    if (!ready || !data || !mem) {
        printf("не вдалося створити об'єкти ядра, код %lu\n", GetLastError());
        printf("(найчастіша причина: вже запущено DebugView або інший приймач)\n");
        return 1;
    }

    struct dbwin* buf = (struct dbwin*)MapViewOfFile(mem, FILE_MAP_READ, 0, 0, 4096);
    if (!buf) {
        printf("MapViewOfFile не вдався, код %lu\n", GetLastError());
        return 1;
    }

    printf("слухаю OutputDebugString. Ctrl+C - вихід.\n");
    printf("---------------------------------------------\n");

    for (;;) {
        SetEvent(ready);                              /* буфер вільний */
        if (WaitForSingleObject(data, INFINITE) != WAIT_OBJECT_0) break;
        printf("[pid %5lu] %s", buf->pid, buf->text);
        fflush(stdout);
        if (buf->text[0] && buf->text[strlen(buf->text) - 1] != '\n') printf("\n");
    }

    UnmapViewOfFile(buf);
    CloseHandle(mem); CloseHandle(data); CloseHandle(ready);
    return 0;
}
