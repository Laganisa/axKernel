#include "frm/kedt.h"
#include "global/_io.h"
#include "manage/_gm.h"
#include "manage/_wm.h"

#define EDT_CMD_YPOS 43 * 8
#define EDT_CMD_XPOS 2 * 8

/*
    기초적인 커널 내장 에디터
*/

void kedt(fcb_t *file)
{

    char prompt[65];
    char file_buf[65];

    uint32_t view_offset = 0;
    uint32_t edt_offset = 0;
    uint8_t mode = M_MODE;

    // 저장
    uint32_t now_console_y = wm_console_line;
    uint32_t now_console_x = wm_console_xpixel;

    // 파일 길이
    uint32_t file_len = file->lens;

    // 현재 보고 있는 페이지
    uint16_t now_page = 1;
    // 현재 보고 있는 라인
    uint32_t now_line = 1;

    /*
        처음에 어떤 파일을 편집 중인지 보여주기
    */

    // 화면 前단
    ltrs(0, 0, "file name : ");
    ltrs(100, 0, (const char *)file->alias);

    gm_line(
        0, 7,
        FULL_WIDTH - 1, 7,
        0xFFFFFFFF,
        0, 7,
        FULL_WIDTH - 1, 7);

    ltrs(35 * 8, 0, "edit mode : ");
    ltrs(35 * 8 + 100, 0, "move");

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
        file_len = file->lens;
        uint32_t page_offset = 0;
        uint32_t first_line = ((uint32_t)now_page - 1) * 40;
        for (uint32_t line = 0;
             line < first_line && page_offset < file_len;
             ++line)
        {
            uint32_t probe_len = file_len - page_offset;
            if (probe_len > 65)
            {
                probe_len = 65;
            }
            uint32_t ret = fm_read(
                fm_record,
                file,
                file_buf,
                probe_len,
                page_offset);
            if (ret == 0)
            {
                break;
            }

            uint32_t consumed = ret > 64 ? 64 : ret;
            for (uint32_t i = 0; i < ret && i <= 64; ++i)
            {
                if (file_buf[i] == '\n')
                {
                    consumed = i + 1;
                    break;
                }
            }
            page_offset += consumed;
        }

        edt_offset = page_offset;
        for (uint32_t i = 0; i < 40; ++i)
        {
            uint32_t y = 8 + 8 * i;
            uint32_t color = i + 1 == now_line ? 0xFF202020 : 0xFF000000;
            gm_rect(
                16, y, FULL_WIDTH - 16, 8, color,
                16, y, FULL_WIDTH - 16, 8);

            uint32_t line_offset = page_offset;
            if (i + 1 == now_line)
            {
                edt_offset = line_offset;
            }

            if (page_offset >= file_len)
            {
                continue;
            }

            uint32_t probe_len = file_len - page_offset;
            if (probe_len > 65)
            {
                probe_len = 65;
            }
            uint32_t ret = fm_read(
                fm_record,
                file,
                file_buf,
                probe_len,
                page_offset);
            if (ret == 0)
            {
                continue;
            }

            uint32_t content_len = ret > 64 ? 64 : ret;
            uint32_t consumed = content_len;
            for (uint32_t j = 0; j < ret && j <= 64; ++j)
            {
                if (file_buf[j] == '\n')
                {
                    content_len = j;
                    consumed = j + 1;
                    break;
                }
            }
            if (content_len > 0 && file_buf[content_len - 1] == '\r')
            {
                --content_len;
            }
            file_buf[content_len] = '\0';
            if (content_len != 0)
            {
                ltrs(16, y, file_buf);
            }
            page_offset += consumed;
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
                    gm_rect(256, 329, 56, 7, 0xFF000000,
                            256, 329, 56, 7);

                    now_page += 1;
                    deci(256, 329, now_page);
                    break;

                // edit line up
                case 'j':
                    if (now_line >= 40)
                        break;

                    gm_rect(72, 329, 56, 7, 0xFF000000,
                            72, 329, 56, 7);

                    now_line += 1;
                    deci(72, 329, now_line);
                    break;

                // edit line down
                case 'k':
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
                    if (now_page <= 0)
                        break;

                    gm_rect(256, 329, 56, 7, 0xFF000000,
                            256, 329, 56, 7);

                    now_page -= 1;
                    deci(256, 329, now_page);
                    break;

                case '`':
                    mode = E_MODE;
                    gm_rect(
                        35 * 8 + 100, 0,
                        6 * 8, 7,
                        0xFF000000,
                        35 * 8 + 100, 0,
                        6 * 8, 7);

                    ltrs(35 * 8 + 100, 0, "insert");
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
                    gm_rect(
                        35 * 8 + 100, 0,
                        6 * 8, 7,
                        0xFF000000,
                        35 * 8 + 100, 0,
                        6 * 8, 7);

                    ltrs(35 * 8 + 100, 0, "move");
                }
                // 파일에 넣어주기
                else
                {
                    uint32_t line_len = 0;
                    uint32_t line_bytes = 0;
                    uint8_t has_newline = FALSE;
                    uint32_t original_file_len = file->lens;
                    if (edt_offset < file->lens)
                    {
                        uint32_t probe_len = file->lens - edt_offset;
                        if (probe_len > 65)
                        {
                            probe_len = 65;
                        }
                        uint32_t ret = fm_read(
                            fm_record,
                            file,
                            file_buf,
                            probe_len,
                            edt_offset);
                        line_len = ret > 64 ? 64 : ret;
                        line_bytes = line_len;
                        for (uint32_t i = 0; i < ret && i <= 64; ++i)
                        {
                            if (file_buf[i] == '\n')
                            {
                                line_len = i;
                                line_bytes = i + 1;
                                has_newline = TRUE;
                                break;
                            }
                        }
                        if (line_len > 0 && file_buf[line_len - 1] == '\r')
                        {
                            --line_len;
                        }
                    }

                    uint32_t input_len = strlen(prompt);
                    uint8_t write_ok = TRUE;
                    if (line_len == 0)
                    {
                        if (edt_offset == original_file_len && input_len != 0 &&
                            fm_write(
                                fm_record,
                                file,
                                prompt,
                                input_len,
                                edt_offset) != input_len)
                        {
                            write_ok = FALSE;
                            puts((const int8_t *)"Editor line write failed\n");
                        }
                    }
                    else
                    {
                        if (input_len > line_len)
                        {
                            input_len = line_len;
                        }

                        char write_buf[64];
                        for (uint32_t i = 0; i < line_len; ++i)
                        {
                            write_buf[i] = i < input_len ? prompt[i] : ' ';
                        }
                        if (fm_write(
                                fm_record,
                                file,
                                write_buf,
                                line_len,
                                edt_offset) != line_len)
                        {
                            write_ok = FALSE;
                            puts((const int8_t *)"Editor line write failed\n");
                        }
                    }

                    uint8_t append_newline =
                        write_ok &&
                        !has_newline &&
                        edt_offset + line_bytes == original_file_len;
                    if (append_newline)
                    {
                        char newline = '\n';
                        if (fm_write(
                                fm_record,
                                file,
                                &newline,
                                1,
                                file->lens) != 1)
                        {
                            puts((const int8_t *)"Editor line write failed\n");
                        }
                    }

                    if (now_line >= 40)
                    {
                        ++now_page;
                        now_line = 1;
                    }
                    else
                    {
                        ++now_line;
                    }

                    gm_rect(72, 329, 56, 7, 0xFF000000,
                            72, 329, 56, 7);
                    gm_rect(256, 329, 56, 7, 0xFF000000,
                            256, 329, 56, 7);
                    deci(72, 329, now_line);
                    deci(256, 329, now_page);
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
