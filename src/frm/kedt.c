#include "frm/kedt.h"
#include "global/_io.h"
#include "manage/_gm.h"

static uint16_t kedtputs(const char *s)
{
    return io_puts_color(s, 0xFFFFFFFF);
}

// ! 지우기 만들기

// 기초적인 커널 내장 에디터
void kedt(fcb_t *file)
{
    char prompt[64];
    char file_buf[64];

    uint32_t view_offset;
    uint32_t edt_offset;
    uint8_t mode = M_MODE;

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
            uint32_t up = fm_read(fm_record, file, file_buf, 63, view_offset);
            kedtputs(file_buf);
            view_offset += up;
        }

        // 읽은 위치에 edt_offset을 넣기
        ltr(io_console_xpixel, io_console_line, '_');

        /*
            명령 입력
        */

        // 명령 입력 위치 선정
        io_console_line = 10;
        io_console_xpixel = 5;

        // 어디 위치인지 쓰고

        // 입력 받고
        gets(prompt, 63);

        // 모드에 따른 스위칭
        if (mode == M_MODE)
        {
            switch ((int8_t)prompt[0])
            {
            // 나가기 표시
            case 'q':
                goto quit;

            case 'i':
                edt_offset -= 64;
                break;

            case 'j':
                edt_offset -= 1;
                break;

            case 'k':
                edt_offset += 1;
                break;

            case 'l':
                edt_offset += 64;
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
                uint32_t len = strlen(prompt);
                fm_write(fm_record, file, prompt, len, edt_offset);
                edt_offset += len;
            }
        }
    }

quit:
    // 복구하기
    io_console_line = now_console_y;
    io_console_xpixel = now_console_x;
}
