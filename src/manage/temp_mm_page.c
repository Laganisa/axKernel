#include "manage/temp_mm.h"
#include "global/_alloc.h"
#include "global/_io.h"

#define TEMP_MM_USER_LIMIT (1ULL << 39)

static page_t *temp_mm_page_node_create(uint8_t is_leaf)
{
    page_t *node = (page_t *)heap_alloc(sizeof(*node));
    if (node == NULL)
    {
        return NULL;
    }

    memset(node, 0, sizeof(*node));
    node->is_leaf = is_leaf;
    return node;
}

static void temp_mm_page_rollback(
    page_t **parents,
    page_t **created,
    uint16_t *slots,
    uint8_t count)
{
    while (count > 0)
    {
        count--;
        parents[count]->pages[slots[count]] = NULL;
        parents[count]->num--;
        parents[count]->is_full = 0;
        heap_free(created[count]);
    }
}

uint8_t temp_mm_page_map(page_t *root, uint64_t vaddr, uint64_t frame_addr)
{
    if (root == NULL || root->is_leaf || frame_addr == 0 ||
        (vaddr & (TEMP_MM_PAGE_SIZE - 1)) != 0 ||
        (frame_addr & (TEMP_MM_PAGE_SIZE - 1)) != 0 ||
        vaddr >= TEMP_MM_USER_LIMIT)
    {
        return FALSE;
    }

    uint16_t indexes[3] = {
        (uint16_t)((vaddr >> 30) & 0x1FF),
        (uint16_t)((vaddr >> 21) & 0x1FF),
        (uint16_t)((vaddr >> 12) & 0x1FF)};
    page_t *parents[2];
    page_t *created[2];
    uint16_t slots[2];
    uint8_t created_count = 0;
    page_t *node = root;

    for (uint8_t level = 0; level < 2; level++)
    {
        if (node->is_leaf)
        {
            temp_mm_page_rollback(parents, created, slots, created_count);
            return FALSE;
        }

        uint16_t slot = indexes[level];
        page_t *child = node->pages[slot];
        uint8_t child_is_leaf = (level == 1);

        if (child == NULL)
        {
            child = temp_mm_page_node_create(child_is_leaf);
            if (child == NULL)
            {
                temp_mm_page_rollback(parents, created, slots, created_count);
                return FALSE;
            }

            node->pages[slot] = child;
            node->num++;
            node->is_full = (node->num == 512);
            parents[created_count] = node;
            created[created_count] = child;
            slots[created_count] = slot;
            created_count++;
        }
        else if (child->is_leaf != child_is_leaf)
        {
            temp_mm_page_rollback(parents, created, slots, created_count);
            return FALSE;
        }

        node = child;
    }

    uint16_t leaf_slot = indexes[2];
    if (!node->is_leaf || node->frame[leaf_slot] != 0)
    {
        temp_mm_page_rollback(parents, created, slots, created_count);
        return FALSE;
    }

    node->frame[leaf_slot] = frame_addr;
    node->num++;
    node->is_full = (node->num == 512);
    return TRUE;
}

uint8_t temp_mm_page_translate(
    page_t *root,
    uint64_t vaddr,
    uint64_t *paddr_out)
{
    if (root == NULL || paddr_out == NULL || root->is_leaf ||
        vaddr >= TEMP_MM_USER_LIMIT)
    {
        return FALSE;
    }

    uint16_t indexes[3] = {
        (uint16_t)((vaddr >> 30) & 0x1FF),
        (uint16_t)((vaddr >> 21) & 0x1FF),
        (uint16_t)((vaddr >> 12) & 0x1FF)};
    page_t *node = root;

    for (uint8_t level = 0; level < 2; level++)
    {
        if (node->is_leaf)
        {
            return FALSE;
        }

        node = node->pages[indexes[level]];
        if (node == NULL)
        {
            return FALSE;
        }
    }

    uint64_t frame_addr = node->frame[indexes[2]];
    if (!node->is_leaf || frame_addr == 0)
    {
        return FALSE;
    }

    *paddr_out = frame_addr + (vaddr & (TEMP_MM_PAGE_SIZE - 1));
    return TRUE;
}

static void temp_mm_page_free_children(page_t *node)
{
    if (node == NULL || node->is_leaf)
    {
        return;
    }

    for (uint16_t i = 0; i < 512; i++)
    {
        if (node->pages[i] != NULL)
        {
            temp_mm_page_free_children(node->pages[i]);
            heap_free(node->pages[i]);
            node->pages[i] = NULL;
        }
    }
}

void temp_mm_page_reset(page_t *root)
{
    if (root == NULL || root->is_leaf)
    {
        return;
    }

    temp_mm_page_free_children(root);
    memset(root, 0, sizeof(*root));
}
