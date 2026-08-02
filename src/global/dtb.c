#include "_types.h"

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

// DTB 파싱 메인 진입 함수
void parse_dtb(uint64_t dtb_addr)
{
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
    dump("DTB_Magic", magic);
    dump("DTB_TotalSize", totalsize);
    dump("DTB_StructOffset", off_struct);
    dump("DTB_StringOffset", off_strings);

    // TODO: 다음 단계인 구조체 블록 순회 준비
}
