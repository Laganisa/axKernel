#include "global/_in_proc.h"
#include "global/_io.h"
#include "tools/_asm.h"
#include "global/_debug.h"
#include "tools/_font.h"
#include "_dstruc.h"

/*static void say(uint64_t line, char *str)
{
    puts(str);
    ltrs(8, 8 * line, str);
}
*/

// 현재 작업 경로를 나타내는 로직
static int kpath()
{
}

// 프롬포트를 분리하는 로직
static int ksplit()
{
}

// 기초적인 커널 내장 쉘
void kshell(void)
{

    // 경로 설정
    stack dir_path;
    uint8_t path_buf[10];
    stack_init(&dir_path, path_buf, 9);

    enter("kernel in shell");

    char prompt[64];

    uint64_t line_y = 0;

    while (1)
    {
        if (line_y < 45)
        {
            // 복사하고 붙여넣기
        }

        puts("ax:");
        // 경로 출력하고
        kpath();

        // 입력 받고
        gets(prompt, 63);

        // 입력을 정규화
        ksplit(prompt);

        if (strcmp(prompt, "exit") == 0)
        {
            puts("exit shell\n");
            break;
        }
        else if (strcmp(prompt, "help") == 0)
        {
            puts("help: show this message\n");
            puts("exit: exit shell\n");
        }
        else
        {
            puts("Unknown command: ");
            puts(prompt);
            puts("\n");
        }
        line_y++;
    }
}