/* Демо 1. Той самий вихідний код - дві бібліотеки періоду виконання.
   Збірка:  cl /utf-8 /MT hello... -> hello_mt.exe (CRT усередині програми)
            cl /utf-8 /MD hello... -> hello_md.exe (CRT у зовнішніх DLL)
   Різниця - у розмірі файлу й у таблиці імпорту, а не в коді.          */
#include <stdio.h>

int main(int argc, char** argv)
{
    printf("Hello, world\n");
    printf("argc = %d, argv[0] = %s\n", argc, argv[0]);
    return 0;
}
