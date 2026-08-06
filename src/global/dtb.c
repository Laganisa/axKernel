#include "_types.h"
#include "global/_io.h"
#include "global/_debug.h"

/*
    dtb 파서 함수
*/
#define FDT_MAGIC 0xd00dfeed

struct fdt_header
{
    uint32_t magic;             // 0xd00dfeed (빅 엔디안)
    uint32_t totalsize;         // DTB 전체 크기
    uint32_t off_dt_struct;     // 구조체 블록 오프셋
    uint32_t off_dt_strings;    // 문자열 블록 오프셋
    uint32_t off_mem_rsvmap;    // 메모리 예약 지도 오프셋
    uint32_t version;           // 버전
    uint32_t last_comp_version; // 호환되는 최소 버전
    uint32_t boot_cpuid_phys;   // 부팅 CPU 물리 ID
    uint32_t size_dt_strings;   // 문자열 블록 크기
    uint32_t size_dt_struct;    // 구조체 블록 크기
};

// 엔디안 변환 함수 (빅 엔디안 -> 리틀 엔디안)
static inline uint32_t fdt32_to_cpu(uint32_t big_endian)
{
    return __builtin_bswap32(big_endian);
}

static uint64_t fdt64_to_cpu(uint64_t val)
{
    // 32비트짜리 fdt32_to_cpu를 활용해 상/하위 32비를 뒤집어 조합
    uint32_t high = (uint32_t)(val >> 32);
    uint32_t low = (uint32_t)(val & 0xFFFFFFFF);
    return ((uint64_t)fdt32_to_cpu(low) << 32) | fdt32_to_cpu(high);
}

// DTB 파싱 메인 진입 함수
void parse_dtb(uint64_t dtb_addr)
{
    if (dtb_addr == 0)
    {
        dump("DTB_FallbackAddr", 0x40000000);
        dtb_addr = 0x40000000;
    }

    struct fdt_header *header = (struct fdt_header *)dtb_addr;

    // 1. 매직 넘버 검증
    uint32_t magic = fdt32_to_cpu(header->magic);

    if (magic != FDT_MAGIC)
    {
        dump("DTB_Error", magic); // 잘못된 값이 들어왔을 때 확인용
        return;
    }

    // 정상적으로 매직 넘버를 읽었다면 기본 정보 추출
    uint32_t totalsize = fdt32_to_cpu(header->totalsize);
    uint32_t off_struct = fdt32_to_cpu(header->off_dt_struct);
    uint32_t off_strings = fdt32_to_cpu(header->off_dt_strings);
    uint32_t off_mem_rsvmap = fdt32_to_cpu(header->off_mem_rsvmap);

    // 디버그 출력으로 헤더 정보가 잘 읽히는지 확인
    /*
    dump("DTB_Magic", magic);
    dump("DTB_TotalSize", totalsize);
    dump("DTB_StructOffset", off_struct);
    dump("DTB_StringOffset", off_strings);
    */

    // TODO: 다음 단계인 구조체 블록 순회 준비
    parse_dtb_tokens(dtb_addr);
}

// FDT 토큰 정의
#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE 0x00000002
#define FDT_PROP 0x00000003
#define FDT_NOP 0x00000004
#define FDT_END 0x00000009

// 4바이트 정렬을 맞추기 위한 헬퍼 함수
static inline uint32_t fdt_align(uint32_t offset)
{
    return (offset + 3) & ~3;
}

// 문자열 크기 계산 (Null-terminated string)
static uint32_t fdt_strnlen(const char *s, uint32_t maxlen)
{
    uint32_t len = 0;
    while (len < maxlen && s[len] != '\0')
    {
        len++;
    }
    return len;
}

char current_node_name[256] = {0};

uint64_t g_virtio_net_base = 0;
uint64_t g_virtio_gpu_base = 0;

void parse_dtb_tokens(uint64_t dtb_addr)
{
    struct fdt_header *header = (struct fdt_header *)dtb_addr;

    if (fdt32_to_cpu(header->magic) != FDT_MAGIC)
    {
        return;
    }

    uint32_t off_struct = fdt32_to_cpu(header->off_dt_struct);
    uint32_t off_strings = fdt32_to_cpu(header->off_dt_strings);

    // 구조체 블록과 문자열 블록의 시작 포인터 계산
    uint8_t *struct_ptr = (uint8_t *)dtb_addr + off_struct;
    char *strings_ptr = (char *)dtb_addr + off_strings;

    uint32_t *p = (uint32_t *)struct_ptr;

    // dump("Start_Token_Parsing", (uint64_t)p);

    while (1)
    {
        uint32_t token = fdt32_to_cpu(*p++);

        if (token == FDT_BEGIN_NODE)
        {
            char *node_name = (char *)p;

            // dump("NODE", (uint64_t)node_name);

            uint32_t len = fdt_strnlen(node_name, 256) + 1;
            p = (uint32_t *)((uint8_t *)p + fdt_align(len));
        }
        else if (token == FDT_END_NODE)
        {
            // 노드 종료
        }
        else if (token == FDT_PROP)
        {
            uint32_t prop_len = fdt32_to_cpu(*p++);
            uint32_t name_off = fdt32_to_cpu(*p++);

            char *prop_name = strings_ptr + name_off;
            void *prop_val = (void *)p;

            if (strcmp(prop_name, "compatible") == 0)
            {
                char *compat_str = (char *)prop_val;
                if (strstr(compat_str, "virtio") != 0)
                {
                    // 여기에 걸리면 Virtio 계열 장치(네트워크, 디스크, GPU 등)입니다!
                    // dump("Found_Virtio_Compatible", compat_str);
                }
            }

            if (strcmp(prop_name, "reg") == 0)
            {
                uint64_t *reg_data = (uint64_t *)prop_val;

                // 1. 빅 엔디안 값을 현재 CPU(ARM64)에 맞게 변환
                uint64_t mmio_addr = fdt64_to_cpu(reg_data[0]);
                uint64_t mmio_size = fdt64_to_cpu(reg_data[1]);

                //  dump("Virtio_MMIO_Addr", mmio_addr);

                // 2. 이 주소에 진짜 Virtio 장치가 있는지, 그리고 ID가 뭔지 확인!
                // (주의: 너무 낮은 주소나 유효하지 않은 주소면 패닉이 날 수 있으니 QEMU virtio 영역인지 체크)
                if (mmio_addr >= 0x0a000000 && mmio_addr < 0x0a200000)
                {
                    volatile uint32_t *device_id_reg = (volatile uint32_t *)(mmio_addr + 0x008);
                    uint32_t device_id = *device_id_reg;

                    if (device_id == 1)
                    {
                        dump("Found_Virtio_Net_At", mmio_addr);
                        g_virtio_net_base = mmio_addr;
                    }
                    else if (device_id == 16)
                    {
                        dump("Found_Virtio_GPU_At", mmio_addr);
                        g_virtio_gpu_base = mmio_addr;
                    }
                }
            }

            // 값의 길이만큼 포인터 이동 후 4바이트 정렬
            p = (uint32_t *)((uint8_t *)p + fdt_align(prop_len));
        }
        else if (token == FDT_NOP)
        {
            continue;
        }
        else if (token == FDT_END)
        {
            dump("DTB_Parse", 0x99999999); // 파싱 완료 표시
            break;
        }
        else
        {
            // 알 수 없는 토큰 예외 처리
            dump("Unknown_Token", token);
            break;
        }
    }
}
