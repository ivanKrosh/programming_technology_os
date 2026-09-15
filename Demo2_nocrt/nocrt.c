/* Демо 2. Та сама задача без бібліотеки періоду виконання.
   Немає printf, немає argv, немає ініціалізації - лише kernel32.

   Збірка:
     cl /utf-8 /O1 /GS- /c demo02_nocrt.c
     link /nologo /nodefaultlib /entry:start /subsystem:console ^
          demo02_nocrt.obj kernel32.lib

   Ціна відмови від CRT видна одразу: командний рядок доводиться
   розбирати самому, а число - переводити в текст руками.            */
#include <windows.h>

static void print(const wchar_t* s)
{
    DWORD  n = 0, len = 0;
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    while (s[len]) len++;

    /* WriteConsoleW пише лише в справжню консоль. Якщо вивід
       перенаправлено у файл, доводиться перекодувати вручну -
       ще одна робота, яку в звичайній програмі зробила б CRT. */
    if (!WriteConsoleW(out, s, len, &n, NULL)) {
        char utf8[1024];
        int  bytes = WideCharToMultiByte(CP_UTF8, 0, s, (int)len,
            utf8, sizeof(utf8), NULL, NULL);
        if (bytes > 0) WriteFile(out, utf8, (DWORD)bytes, &n, NULL);
    }
}

void start(void)                 /* точка входу замість mainCRTStartup */
{
    print(L"Hello, world\n");

    /* argv немає - є лише сирий командний рядок від системи */
    print(L"командний рядок: ");
    print(GetCommandLineW());
    print(L"\n");

    ExitProcess(0);              /* return нікуди не веде: CRT нас не викликала */
}
