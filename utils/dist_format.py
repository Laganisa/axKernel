# dist의 파일 시스템 포맷을 담당

#B+트리와 나의 fs를 보고 수정하기

# 해시 함수

def fnv1a_64(s):
    fnv_prime = 0x100000001b3
    hash_val = 0xcbf29ce484222325
    for x in s:
        hash_val ^= x
        hash_val = (hash_val * fnv_prime) & 0xFFFFFFFFFFFFFFFF
    return hash_val

def hash_djb2(s):
    hash = 5381
    for x in s:
        hash = ((hash << 5) + hash) + ord(x)
    return hash & 0xFFFFFFFF

