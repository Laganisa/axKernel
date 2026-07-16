// #include "_fm.h"
// #include "_io.h"
// #include "_mm.h"
#include "_asm.h"
#include "_debug.h"
#include "_alloc.h"
#include "_hash.h"
#include "_fs.h"

bpt_node *create_node(uint8_t leaf)
{
    bpt_node *new_node = heap_alloc(sizeof(bpt_node));

    if (new_node == NULL)
    {
        return NULL;
    }

    memset(new_node, 0, sizeof(bpt_node));

    new_node->leaf = leaf;
    new_node->data_num = 0;
    new_node->next = NULL;

    return new_node;
}

bpt_node *search_leaf(bpt_node *root, uint64_t key)
{
    if (root == NULL)
        return NULL; // 최상위 노드 자체가 없으면 실패

    bpt_node *pre = root;
    while (!pre->leaf)
    {
        int i;
        for (i = 0; i < pre->data_num; i++)
        {
            if (key < pre->fnv1a_hash_key[i])
                break;
        }

        // [추가] 자식 노드가 존재하는지 확인!
        if (pre->child[i] == NULL)
        {
            return NULL; // 트리가 제대로 형성되지 않았거나 데이터가 없음
        }

        pre = pre->child[i];
    }

    return pre;
}

uint8_t insert(bpt_node *root, char *name, uint16_t value)
{
    uint64_t key1 = fnv1a_hash_64(name);

    uint64_t key2 = djb2_hash_64(name);
    bpt_node *leaf = search_leaf(root, key1);

    if (leaf->data_num < MAX_BPT_NODE_NUM - 1)
    {
        // 그냥 삽입
        int pos = 0;

        while (pos < leaf->data_num && leaf->fnv1a_hash_key[pos] < key1)
        {
            pos++;
        }
        for (int i = leaf->data_num; i > pos; i--)
        {
            leaf->fnv1a_hash_key[i] = leaf->fnv1a_hash_key[i - 1];
            leaf->djb2_hash_Key[i] = leaf->djb2_hash_Key[i - 1];
            leaf->data[i] = leaf->data[i - 1];
        }
        leaf->fnv1a_hash_key[pos] = key1;
        leaf->djb2_hash_Key[pos] = key2;
        leaf->data[pos] = value;

        leaf->data_num++;
        return 1;
    }
    else
    {
        // Split
    }

    return 0;
}

uint16_t *search(bpt_node *root, char *name)
{
    uint64_t key = fnv1a_hash_64(name);

    bpt_node *leaf = search_leaf(root, key);

    if (leaf == NULL)
    {
        return NULL;
    }

    for (int i = 0; i < leaf->data_num; i++)
    {

        if (leaf->fnv1a_hash_key[i] == key)
        {
            return leaf->data[i];
        }
    }
    return NULL;
}
