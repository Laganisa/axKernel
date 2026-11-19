#include "frm/kedt.h"
#include "global/_io.h"
#include "manage/_gm.h"
#include "manage/_wm.h"

// ! 지우기 만들기

#define EDT_CMD_YPOS 43 * 8
#define EDT_CMD_XPOS 2 * 8

// 기초적인 커널 내장 에디터
void kedt(fcb_t *file)
{
    enter("kedt");

    char prompt[64];
    char file_buf[64];

    uint32_t view_offset = 0;
    uint32_t edt_offset = 0;
    uint8_t mode = M_MODE;

    // 저장
    uint32_t now_console_y = wm_console_line;
    uint32_t now_console_x = wm_console_xpixel;

    // 파일 길이
    uint32_t file_len = file->lens;

    // 현재 보고 있는 페이지
    uint16_t now_page = 0;
    // 현재 보고 있는 라인
    uint32_t now_line = 0;

    /*
        처음에 어떤 파일을 편집 중인지 보여주기
    */

    // 화면 前단
    ltrs(0, 0, "file name : ");
    ltrs(100, 0, file->alias);

    gm_line(
        0, 7,
        FULL_WIDTH - 1, 7,
        0xFFFFFFFF,
        0, 7,
        FULL_WIDTH - 1, 7);

    // 화면 中단
    for (int i = 0; i < 40; i++)
    {
        deci(0, 8 + 8 * i, i + 1);
    }

    gm_line(
        15, 8,
        15, (FULL_HIGHT - 1) - 4 * 8,
        0xFFFFFFFF,
        15, 8,
        15, (FULL_HIGHT - 1) - 4 * 8);

    // 화면 後단

    gm_line(
        0, 41 * 8,
        FULL_WIDTH - 1, 41 * 8,
        0xFFFFFFFF,
        0, 41 * 8,
        FULL_WIDTH - 1, 41 * 8);

    // 상태 표시
    ltrs(16, 41 * 8 + 1, "line : ");
    deci(72, 41 * 8 + 1, now_line);

    ltrs(200, 41 * 8 + 1, "page : ");
    deci(256, 41 * 8 + 1, now_page);

    gm_line(
        0, 42 * 8,
        FULL_WIDTH - 1, 42 * 8,
        0xFFFFFFFF,
        0, 42 * 8,
        FULL_WIDTH - 1, 42 * 8);

    while (1)
    {
        /*
            화면 출력
        */

        // 버퍼에 복사하고 출력 하기
        for (int i = 0; i < 40; i++)
        {

            fm_read(
                fm_record,
                file,
                file_buf,
                63,
                now_page + 64 * i);

            ltrs(2 * 8, 8 + 8 * i, file_buf);
        }

        /*
            명령 입력
        */

        // 어디 위치인지 쓰고
        {
            // 입력 받고
            if (gtb(EDT_CMD_XPOS, EDT_CMD_YPOS, prompt, sizeof(prompt)) < 0)
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

                // edit page up
                // 다음 페이지를 봄
                case 'h':
                    puts("cmd edit page up\n");

                    gm_rect(256, 329, 56, 7, 0xFF000000,
                            256, 329, 56, 7);

                    now_page += 1;
                    deci(256, 329, now_page);
                    break;

                // edit line up
                case 'j':
                    puts("cmd edit line up\n");

                    if (now_line >= 40)
                        break;

                    gm_rect(72, 329, 56, 7, 0xFF000000,
                            72, 329, 56, 7);

                    now_line += 1;
                    deci(72, 329, now_line);
                    break;

                // edit line down
                case 'k':
                    puts("cmd edit line down\n");

                    if (now_line <= 0)
                        break;

                    gm_rect(72, 329, 56, 7, 0xFF000000,
                            72, 329, 56, 7);

                    now_line -= 1;
                    deci(72, 329, now_line);
                    break;

                // edit page down
                // 이전 페이지를 봄
                case 'l':
                    puts("cmd edit page down\n");

                    if (now_page <= 0)
                        break;

                    gm_rect(256, 329, 56, 7, 0xFF000000,
                            256, 329, 56, 7);

                    now_page -= 1;
                    deci(256, 329, now_page);
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

        gm_rect(16, 337 + 7, 608, 8, 0xFF000000,
                16, 337 + 7, 608, 8);
    }

quit:
    // 복구하기
    wm_console_line = now_console_y - 1;
    wm_console_xpixel = now_console_x;
}
