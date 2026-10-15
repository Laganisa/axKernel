#include "frm/kedt.h"

// 기초적인 커널 내장 에디터
void kedt(fcb_t *file)
{
    char prompt[64];

    while (1)
    {
        // 입력 받고
        gets(prompt, 63);

        // 입력을 정규화 및 적용
        // ksplit(prompt);
    }
}
