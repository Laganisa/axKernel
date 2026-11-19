#include "frm/kedt.h"
#include "global/_io.h"
#include "manage/_gm.h"
#include "manage/_wm.h"

// ! 지우기 만들기

#define EDT_CMD_YPOS 20 * 8
#define EDT_CMD_XPOS 8 * 4

// 기초적인 커널 내장 에디터
void kedt(fcb_t *file)
{
    enter("kedt");

    char prompt[64];
    char file_buf[64];

    uint32_t view_offset;
    uint32_t edt_offset;
    uint8_t mode = M_MODE;

    // 저장
    // ! 왜 필요한가
    uint32_t now_console_y = wm_console_line;
    uint32_t now_console_x = wm_console_xpixel;

    // 파일 길이
    uint32_t file_len = file->lens;

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
            wputs(file_buf);
            view_offset += up;
            wputs("\n");
        }

        // 읽은 위치에 edt_offset을 넣기
        ltr(wm_console_xpixel, wm_console_line, '_');

        /*
            명령 입력
        */

        // 명령 입력 위치 선정
        wm_console_line = EDT_CMD_YPOS;
        wm_console_xpixel = EDT_CMD_XPOS;

        // 어디 위치인지 쓰고

        {
            // 입력 받고
            if (wget(prompt, sizeof(prompt)) < 0)
            {
                return;
            }

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

                case '`':
                    mode = E_MODE;
                    puts("mode edt switch!\n");
                    break;

                default:
                    break;
                }
            }
            // 입력 받은 문자 그대로 파일에 넣어주기
            else
            {
                // 전환 로직
                if (prompt[0] == '`')
                {
                    mode = M_MODE;
                    puts("mode mov switch!\n");
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

        gm_rect(0, EDT_CMD_YPOS, 360, 8, 0xFF000000, 0, EDT_CMD_YPOS, 360, 8);
    }

quit:
    // 복구하기
    wm_console_line = now_console_y - 1;
    wm_console_xpixel = now_console_x;
}
