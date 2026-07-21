#ifndef __KERNEL_FS_H__
#define __KERNEL_FS_H__

#include "_defs.h"
#include "_types.h"

typedef struct bpt_node
{
    uint8_t data_num : 7;
    uint8_t leaf : 1;
    uint64_t fnv1a_hash_key[MAX_BPT_NODE_NUM - 1]; // 검색에 주된 키
    uint64_t djb2_hash_Key[MAX_BPT_NODE_NUM - 1];  // 다시 확인용도
    uint16_t data[MAX_BPT_NODE_NUM - 1];           // 3개
    struct node *parent;                           // 부모 포인터
    struct node *child[MAX_BPT_NODE_NUM];
    struct node *next;
} bpt_node;

bpt_node *search_leaf(bpt_node *root, uint64_t key);
bpt_node *create_node(uint8_t leaf);
uint8_t insert(bpt_node *root, char *name, uint16_t value);
uint16_t *search(bpt_node *root, char *name);

#endif
