#include "fm.h"
#include "io.h"
#include "mm.h"
#include "asm.h"
#include "debug.h"

/*
    FAT 시스템을 정리하는 파일
    fm.c, exfm.c의 파일 크기를 줄이고자
*/

/*
   글자 8자 - 구문자 - 글자 8자 - 구문자 - 글자 8자
   해시함수에 넣기 좋게 자르면
   3글자 - 구문자 - 3글자 - 구문자
   3/3/3으로 쪼개고 차이를 저장하기
   매핑 테이블도 필요함
*/

// ! 수정 필요
uint16_t token(int8_t segment[8])
{
    // 1. 첫 글자 보정 (모음 -> 자음)
    int8_t first = segment[0];
    if (first == 65 || first == 69 || first == 73 || first == 79 || first == 85)
    {
        first -= 1;
    }

    // 차이값 계산 (첫 번째 차이: 1글자-2글자, 두 번째 차이: 2글자-3글자)
    // 공백(0x20)이나 점('.')이 오면 차이를 0으로 처리하거나 예외처리
    int8_t diff1 = 0;
    int8_t diff2 = 0;

    if (segment[1] != 0x20 && segment[1] != '.')
    {
        diff1 = abs(segment[1] - segment[0]);
    }
    if (segment[2] != 0x20 && segment[2] != '.')
    {
        diff2 = abs(segment[2] - segment[1]);
    }

    // 4비트 패킹 (하위 4비트만 추출하여 데이터 손실 방지)
    // 0xF(1111)와 AND 연산하여 4비트 범위를 강제함
    uint8_t packed_diffs = ((diff1 & 0x0F) << 4) | (diff2 & 0x0F);

    // 최종 16비트 결과 생성 (첫 글자 8bit + 차이들 8bit)
    uint16_t result = (uint16_t)(first << 8) | packed_diffs;

    return result;
}

//  경로 유효성 검사
/*
    규칙에 따라 경로가 유효한지 확인
    cmd == 0
    1. 구문자가 고정된 위치가 맞는지 검사
    2. 각 마디에 '.' 의 위치가 맞는지 검사
    3. 공백 후 문자가 나오는지 검사
    cmd == 1
    1. 경로에서 최근 디렉토리를 알아낸다
    2. 디랙토리를 알아냈다면 mapping 테이블에서
    현재 주소를 변환한 값(저기 위에 token 함수를 통과한 값)이랑
    같은 문자가 있는지 확인
*/
bool fm_check(FMv2_record *reco, uint8_t cmd, int8_t path[27])
{
    // 경로길이가 짧으면 늘려주는 함수
    // 얼마나 짧은지 확인
    // 확인 후 구분자 및 널 문자를 넣어줌
    uint8_t i = 0;

    // 문자열 끝 찾기
    while (i < 26 && path[i] != '\0')
    {
        i++;
    }

    // 끝까지 공백 채우기
    while (i < 26)
    {
        path[i++] = 0x20;
    }

    // 마지막 널 문자
    path[26] = '\0';
    // while 문으로 길이를 확인
    for (int i = 0; i < 27; i++)
    {
        // 다음 문자가 널 문자면
        if (path[i] == '\0')
        {
            // 늘려주고
            break;
        }
    }

    // 경로 유호성 검사
    if (cmd == 0)
    {
        bool is_ok = TRUE;

        // 1. 구문자 및 널 문자 고정 위치 검사
        if (path[8] != '/' || path[17] != '/' || path[26] != '\0')
        {
            return FALSE;
        }
        if (path[22] != '.')
        {
            return FALSE; // 파일명 영역의 '.' 위치 고정
        }
        // 2. 이중 루프로 각 마디(8글자씩) 검사
        int segments[3] = {0, 9, 18}; // 각 마디의 시작 인덱스

        for (int s = 0; s < 3; s++)
        {
            int start = segments[s];
            bool space_started = FALSE;

            for (int i = 0; i < 8; i++)
            {
                int8_t current_char = path[start + i];

                // 파일명 마디(s==2)에서 '.' 위치(인덱스 22)는 건너뜀
                if (s == 2 && (start + i) == 22)
                {
                    continue;
                }

                if (current_char == 0x20)
                {
                    space_started = TRUE; // 이제부터는 계속 공백이어야 함
                }
                else
                {
                    // 공백이 이미 시작됐는데 문자가 나타나면 규칙 위반
                    if (space_started)
                    {
                        is_ok = FALSE;

                        break;
                    }
                    // ! 특수 문자 로직 넣는 자리
                    // 보통은 특수 문자를 허용하지 않음
                }
            }
            if (!is_ok)
            {

                break;
            }
        }

        return is_ok;
    }
    // 디랙토리에 중복 있는지 검사
    else if (cmd == 1)
    {
        // 검사하는 법
        /*
            경로에서 최근 디렉토리를 알아낸다
            디랙토리를 알아냈다면 mapping 테이블에서
            현재 주소를 변환한 값(저기 위에 token 함수를 통과한 값)이랑
            같은 문자가 있는지 확인
            확인은 일단 저 mapping 테이블이 8비트여서 문자 8개씩 비교할 수 있음
            또한 한 디렉토리당 파일 수가 16개 여서 2번 반복하면 됨
        */
        // 그럼 일단 현재 경로에서 위에 디렉토리가 몇개인지 확인하는 로직
        // 경로에서 앞의 문자가 공백이 아니면 디렉토리가 있다 판단
        // 항상 경로를 만들때 앞에서 부터 채우기 때문
        // 8번쨰는 구문자여서
        // 9번째와 18번째 글자가 공백이 아니면 각각 디렉토리가 있다고 판단

        uint8_t dir_count = 0;
        uint8_t pos_dir1 = 0;
        uint8_t pos_dir2 = 0;

        if (path[9] != 0x20)
        {
            // 그 앞을 pos_dir1로 저장
            pos_dir1 = token(&path[0]);
            dir_count++;
        }
        if (path[18] != 0x20)
        {
            // 그 앞을 pos_dir2로 저장
            pos_dir2 = token(&path[9]);
            dir_count++;
        }

        bool is_ok = TRUE;

        // 디렉토리는 다음에 나오는 위치가 16임
        if (dir_count == 0)
        {
            // 루트 디렉토리인 경우
            // 매핑 테이블에서 루트 디렉토리에 해당하는 값이 있는지 확인
            // ! 간단한 구현으로 대체
            for (int i = 0; i < 16; i++)
            {
                if (reco->mapping[i][16][16] == token(path))
                {
                    // 값이 존재함
                    is_ok = FALSE;
                    break;
                }
            }
        }
        else if (dir_count == 1)
        {
            // 첫번째 디렉토리가 있는 경우
            // 매핑 테이블에서 첫번째 디렉토리에 해당하는 값이 있는지 확인

            // ! 간단한 구현으로 대체
            for (int i = 0; i < 16; i++)
            {
                if (reco->mapping[pos_dir1][i][16] == token(path))
                {
                    // 값이 존재함
                    is_ok = FALSE;
                    break;
                }
            }
        }
        else if (dir_count == 2)
        {
            // 두번째 디렉토리가 있는 경우
            // 매핑 테이블에서 두번째 디렉토리에 해당하는 값이 있는지 확인

            // ! 간단한 구현으로 대체
            for (int i = 0; i < 16; i++)
            {
                if (reco->mapping[pos_dir1][pos_dir2][i] == token(path))
                {
                    // 값이 존재함
                    is_ok = FALSE;
                    break;
                }
            }
        }

        if (is_ok)
        {
            // 만약 중복이 없다면
            return TRUE;
        }
        else
        {
            // 중복이 있다면
            return FALSE;
        }
    }
    // 맞지 않는 명령
    else
    {
        return FALSE;
    }
}