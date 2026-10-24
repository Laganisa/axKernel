#include "frm/kedt.h"
#include "global/_io.h"

static uint16_t kedtputs(const char *s)
{
    return io_puts_color(s, 0xFFFFFFFF);
}

// 기초적인 커널 내장 에디터
void kedt(fcb_t *file)
{
    char prompt[64];
    char file_buf[64];

    char read_offset;
    uint8_t mode;

    uint32_t now_console_y = io_console_line;
    uint32_t now_console_x = io_console_xpixel;

    while (1)
    {
        /*
            화면 출력
        */

        // 버퍼에 복사하고 출력 하기
        // ! 반복 횟수 정하기
        // ! 파일 길이를 받아서 길이랑 화면 전체 공간을 생각하며 하면 될 듯
        for (int i = 0; i < 10; i++)
        {
            uint32_t up = fm_read(fm_record, file, file_buf, 63, read_offset);
            kedtputs(file_buf);
            read_offset += up;
        }

        /*
            입력
        */

        // 입력 위치 선정
        io_console_line = 10;
        io_console_xpixel = 5;

        // 입력 받고
        gets(prompt, 63);

        // 모드에 따른 스위칭
        if (mode == M_MODE)
        {
            switch (prompt[0])
            {
            // 나가기 표시
            case 'q':
                goto quit;
            case 'i':
                break;
            case 'j':
                break;
            case 'k':
                break;
            case 'l':
                break;
            case IO_KEY_ESC:
                mode = E_MODE;
                break;
            default:
                break;
            }
        }
        // 입력 받은 문자 그대로 파일에 넣어주기
        else
        {
            // 전환 로직
            if (prompt[0] == IO_KEY_ESC)
            {
                mode = M_MODE;
            }
            // 파일에 넣어주기
            else
            {
            }
        }
    }

quit:
    // 복구하기
    io_console_line = now_console_y;
    io_console_xpixel = now_console_x;
}
