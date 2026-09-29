#include "tools/_asm.h"
#include "global/_alloc.h"
#include "tools/_hash.h"
#include "tools/_fs.h"

static void clear(bpt_node *node)
{
    uint8_t *bytes = (uint8_t *)node;
    for (uint32_t index = 0; index < sizeof(bpt_node); index++)
    {
        bytes[index] = 0;
    }
}

static bpt_node *parent_of(bpt_node *node)
{
    return (bpt_node *)node->parent;
}

static bpt_node *child_of(bpt_node *node, int index)
{
    return (bpt_node *)node->child[index];
}

static void set_parent(bpt_node *node, bpt_node *parent)
{
    node->parent = (struct node *)parent;
}

static void set_child(bpt_node *node, int index, bpt_node *child)
{
    node->child[index] = (struct node *)child;
}

bpt_node *create_node(uint8_t leaf)
{
    bpt_node *new_node = heap_alloc(sizeof(bpt_node));

    if (new_node == NULL)
    {
        return NULL;
    }

    clear(new_node);
    new_node->leaf = leaf;
    return new_node;
}

bpt_node *search_leaf(bpt_node *root, uint64_t key)
{
    if (root == NULL)
    {
        return NULL;
    }

    bpt_node *current = root;
    while (!current->leaf)
    {
        int index = 0;
        while (index < current->data_num && key >= current->fnv1a_hash_key[index])
        {
            index++;
        }

        current = child_of(current, index);
        if (current == NULL)
        {
            return NULL;
        }
    }

    return current;
}

static void put_leaf(bpt_node *leaf, const uint64_t *fnv_keys,
                     const uint64_t *djb_keys, const uint16_t *values,
                     int count)
{
    leaf->leaf = 1;
    leaf->data_num = count;
    for (int index = 0; index < count; index++)
    {
        leaf->fnv1a_hash_key[index] = fnv_keys[index];
        leaf->djb2_hash_Key[index] = djb_keys[index];
        leaf->data[index] = values[index];
    }
}

static void put_inner(bpt_node *internal, const uint64_t *keys,
                      bpt_node **children, int key_count)
{
    internal->leaf = 0;
    internal->data_num = key_count;
    for (int index = 0; index < key_count; index++)
    {
        internal->fnv1a_hash_key[index] = keys[index];
    }
    for (int index = 0; index <= key_count; index++)
    {
        set_child(internal, index, children[index]);
        set_parent(children[index], internal);
    }
}

static uint8_t add_parent(bpt_node *root, bpt_node *left,
                          uint64_t separator, bpt_node *right)
{
    bpt_node *parent = parent_of(left);
    if (parent == NULL)
    {
        return 0;
    }

    int child_index = 0;
    while (child_index <= parent->data_num && child_of(parent, child_index) != left)
    {
        child_index++;
    }
    if (child_index > parent->data_num)
    {
        return 0;
    }

    uint64_t keys[MAX_BPT_NODE_NUM];
    bpt_node *children[MAX_BPT_NODE_NUM + 1];
    int old_key_count = parent->data_num;
    int new_key_count = old_key_count + 1;

    for (int index = 0; index < child_index; index++)
    {
        keys[index] = parent->fnv1a_hash_key[index];
    }
    keys[child_index] = separator;
    for (int index = child_index; index < old_key_count; index++)
    {
        keys[index + 1] = parent->fnv1a_hash_key[index];
    }
    for (int index = 0; index <= child_index; index++)
    {
        children[index] = child_of(parent, index);
    }
    children[child_index + 1] = right;
    for (int index = child_index + 1; index <= old_key_count; index++)
    {
        children[index + 1] = child_of(parent, index);
    }

    if (new_key_count < MAX_BPT_NODE_NUM)
    {
        put_inner(parent, keys, children, new_key_count);
        return 1;
    }

    bpt_node *right_internal = create_node(0);
    if (right_internal == NULL)
    {
        return 0;
    }

    int middle = new_key_count / 2;
    int left_key_count = middle;
    int right_key_count = new_key_count - middle - 1;

    if (parent == root)
    {
        bpt_node *left_internal = create_node(0);
        if (left_internal == NULL)
        {
            return 0;
        }

        put_inner(left_internal, keys, children, left_key_count);
        put_inner(right_internal, keys + middle + 1,
                  children + middle + 1, right_key_count);

        clear(root);
        root->leaf = 0;
        root->data_num = 1;
        root->fnv1a_hash_key[0] = keys[middle];
        set_child(root, 0, left_internal);
        set_child(root, 1, right_internal);
        set_parent(left_internal, root);
        set_parent(right_internal, root);
        return 1;
    }

    bpt_node *grandparent = parent_of(parent);
    if (grandparent == NULL)
    {
        return 0;
    }

    put_inner(parent, keys, children, left_key_count);
    put_inner(right_internal, keys + middle + 1,
              children + middle + 1, right_key_count);
    set_parent(right_internal, grandparent);

    return add_parent(root, parent, keys[middle], right_internal);
}

uint8_t bpt_insert(bpt_node *root, char *name, uint16_t value)
{
    if (root == NULL || name == NULL)
    {
        return 0;
    }

    uint64_t fnv_key = fnv1a_hash_64(name);
    uint64_t djb_key = djb2_hash_64(name);
    bpt_node *leaf = search_leaf(root, fnv_key);
    if (leaf == NULL)
    {
        return 0;
    }

    int position = 0;
    while (position < leaf->data_num && leaf->fnv1a_hash_key[position] < fnv_key)
    {
        position++;
    }
    if (position < leaf->data_num && leaf->fnv1a_hash_key[position] == fnv_key)
    {
        return 0;
    }

    if (leaf->data_num < MAX_BPT_NODE_NUM - 1)
    {
        for (int index = leaf->data_num; index > position; index--)
        {
            leaf->fnv1a_hash_key[index] = leaf->fnv1a_hash_key[index - 1];
            leaf->djb2_hash_Key[index] = leaf->djb2_hash_Key[index - 1];
            leaf->data[index] = leaf->data[index - 1];
        }

        leaf->fnv1a_hash_key[position] = fnv_key;
        leaf->djb2_hash_Key[position] = djb_key;
        leaf->data[position] = value;
        leaf->data_num++;
        return 1;
    }

    uint64_t fnv_keys[MAX_BPT_NODE_NUM];
    uint64_t djb_keys[MAX_BPT_NODE_NUM];
    uint16_t values[MAX_BPT_NODE_NUM];
    for (int index = 0; index < position; index++)
    {
        fnv_keys[index] = leaf->fnv1a_hash_key[index];
        djb_keys[index] = leaf->djb2_hash_Key[index];
        values[index] = leaf->data[index];
    }
    fnv_keys[position] = fnv_key;
    djb_keys[position] = djb_key;
    values[position] = value;
    for (int index = position; index < leaf->data_num; index++)
    {
        fnv_keys[index + 1] = leaf->fnv1a_hash_key[index];
        djb_keys[index + 1] = leaf->djb2_hash_Key[index];
        values[index + 1] = leaf->data[index];
    }

    bpt_node *right_leaf = create_node(1);
    if (right_leaf == NULL)
    {
        return 0;
    }

    int left_count = (MAX_BPT_NODE_NUM + 1) / 2;
    int right_count = MAX_BPT_NODE_NUM - left_count;
    bpt_node *parent = parent_of(leaf);

    if (leaf == root)
    {
        bpt_node *left_leaf = create_node(1);
        if (left_leaf == NULL)
        {
            return 0;
        }

        put_leaf(left_leaf, fnv_keys, djb_keys, values, left_count);
        put_leaf(right_leaf, fnv_keys + left_count,
                 djb_keys + left_count, values + left_count, right_count);
        left_leaf->next = (struct node *)right_leaf;

        clear(root);
        root->leaf = 0;
        root->data_num = 1;
        root->fnv1a_hash_key[0] = right_leaf->fnv1a_hash_key[0];
        set_child(root, 0, left_leaf);
        set_child(root, 1, right_leaf);
        set_parent(left_leaf, root);
        set_parent(right_leaf, root);
        return 1;
    }

    if (parent == NULL)
    {
        return 0;
    }

    struct node *old_next = leaf->next;
    put_leaf(leaf, fnv_keys, djb_keys, values, left_count);
    put_leaf(right_leaf, fnv_keys + left_count,
             djb_keys + left_count, values + left_count, right_count);
    leaf->next = (struct node *)right_leaf;
    right_leaf->next = old_next;
    set_parent(right_leaf, parent);

    return add_parent(root, leaf, right_leaf->fnv1a_hash_key[0], right_leaf);
}

static uint64_t min_key(bpt_node *node)
{
    while (!node->leaf)
    {
        node = child_of(node, 0);
    }
    return node->fnv1a_hash_key[0];
}

static void fix_keys(bpt_node *node)
{
    for (int index = 0; index < node->data_num; index++)
    {
        node->fnv1a_hash_key[index] = min_key(child_of(node, index + 1));
    }
}

static void fix_up(bpt_node *node)
{
    while (node != NULL)
    {
        if (!node->leaf)
        {
            fix_keys(node);
        }
        node = parent_of(node);
    }
}

static int child_idx(bpt_node *parent, bpt_node *child)
{
    for (int index = 0; index <= parent->data_num; index++)
    {
        if (child_of(parent, index) == child)
        {
            return index;
        }
    }
    return -1;
}

static void drop_child(bpt_node *parent, int child_index)
{
    int old_key_count = parent->data_num;
    int separator_index = 0;
    if (child_index > 0)
    {
        separator_index = child_index - 1;
    }

    for (int index = separator_index; index < old_key_count - 1; index++)
    {
        parent->fnv1a_hash_key[index] = parent->fnv1a_hash_key[index + 1];
    }
    for (int index = child_index; index < old_key_count; index++)
    {
        set_child(parent, index, child_of(parent, index + 1));
    }

    parent->data_num--;
    set_child(parent, old_key_count, NULL);
}

static void balance_inner(bpt_node *root, bpt_node *node)
{
    int minimum_keys = (MAX_BPT_NODE_NUM + 1) / 2 - 1;

    if (node == root)
    {
        if (!root->leaf && root->data_num == 0)
        {
            bpt_node *only_child = child_of(root, 0);
            if (only_child == NULL)
            {
                return;
            }

            *root = *only_child;
            set_parent(root, NULL);
            if (!root->leaf)
            {
                for (int index = 0; index <= root->data_num; index++)
                {
                    set_parent(child_of(root, index), root);
                }
            }
            heap_free(only_child);
            return;
        }

        fix_keys(root);
        return;
    }

    if (node->data_num >= minimum_keys)
    {
        fix_up(node);
        return;
    }

    bpt_node *parent = parent_of(node);
    if (parent == NULL)
    {
        return;
    }

    int index = child_idx(parent, node);
    if (index < 0)
    {
        return;
    }

    bpt_node *left = NULL;
    bpt_node *right = NULL;
    if (index > 0)
    {
        left = child_of(parent, index - 1);
    }
    if (index < parent->data_num)
    {
        right = child_of(parent, index + 1);
    }

    if (left != NULL && left->data_num > minimum_keys)
    {
        int left_child_count = left->data_num + 1;
        for (int child_index = node->data_num + 1; child_index > 0; child_index--)
        {
            set_child(node, child_index, child_of(node, child_index - 1));
        }
        set_child(node, 0, child_of(left, left_child_count - 1));
        set_parent(child_of(node, 0), node);
        left->data_num--;
        node->data_num++;
        set_child(left, left_child_count - 1, NULL);
        fix_keys(left);
        fix_keys(node);
        fix_up(parent);
        return;
    }

    if (right != NULL && right->data_num > minimum_keys)
    {
        int right_key_count = right->data_num;
        set_child(node, node->data_num + 1, child_of(right, 0));
        set_parent(child_of(node, node->data_num + 1), node);
        node->data_num++;
        for (int child_index = 0; child_index < right_key_count; child_index++)
        {
            set_child(right, child_index, child_of(right, child_index + 1));
        }
        right->data_num--;
        set_child(right, right->data_num + 1, NULL);
        fix_keys(node);
        fix_keys(right);
        fix_up(parent);
        return;
    }

    if (left != NULL)
    {
        int left_key_count = left->data_num;
        int node_key_count = node->data_num;
        for (int child_index = 0; child_index <= node_key_count; child_index++)
        {
            bpt_node *child = child_of(node, child_index);
            set_child(left, left_key_count + 1 + child_index, child);
            set_parent(child, left);
        }
        left->data_num = left_key_count + node_key_count + 1;
        fix_keys(left);
        drop_child(parent, index);
        heap_free(node);
        balance_inner(root, parent);
        return;
    }

    if (right != NULL)
    {
        int node_key_count = node->data_num;
        int right_key_count = right->data_num;
        for (int child_index = 0; child_index <= right_key_count; child_index++)
        {
            bpt_node *child = child_of(right, child_index);
            set_child(node, node_key_count + 1 + child_index, child);
            set_parent(child, node);
        }
        node->data_num = node_key_count + right_key_count + 1;
        fix_keys(node);
        drop_child(parent, index + 1);
        heap_free(right);
        balance_inner(root, parent);
    }
}

static void balance_leaf(bpt_node *root, bpt_node *leaf)
{
    int minimum_keys = MAX_BPT_NODE_NUM / 2;
    if (leaf == root)
    {
        return;
    }

    bpt_node *parent = parent_of(leaf);
    if (parent == NULL)
    {
        return;
    }

    if (leaf->data_num >= minimum_keys)
    {
        fix_up(parent);
        return;
    }

    int index = child_idx(parent, leaf);
    if (index < 0)
    {
        return;
    }

    bpt_node *left = NULL;
    bpt_node *right = NULL;
    if (index > 0)
    {
        left = child_of(parent, index - 1);
    }
    if (index < parent->data_num)
    {
        right = child_of(parent, index + 1);
    }

    if (left != NULL && left->data_num > minimum_keys)
    {
        for (int item = leaf->data_num; item > 0; item--)
        {
            leaf->fnv1a_hash_key[item] = leaf->fnv1a_hash_key[item - 1];
            leaf->djb2_hash_Key[item] = leaf->djb2_hash_Key[item - 1];
            leaf->data[item] = leaf->data[item - 1];
        }
        int last = left->data_num - 1;
        leaf->fnv1a_hash_key[0] = left->fnv1a_hash_key[last];
        leaf->djb2_hash_Key[0] = left->djb2_hash_Key[last];
        leaf->data[0] = left->data[last];
        left->data_num--;
        leaf->data_num++;
        fix_up(parent);
        return;
    }

    if (right != NULL && right->data_num > minimum_keys)
    {
        int append_index = leaf->data_num;
        leaf->fnv1a_hash_key[append_index] = right->fnv1a_hash_key[0];
        leaf->djb2_hash_Key[append_index] = right->djb2_hash_Key[0];
        leaf->data[append_index] = right->data[0];
        leaf->data_num++;
        for (int item = 0; item < right->data_num - 1; item++)
        {
            right->fnv1a_hash_key[item] = right->fnv1a_hash_key[item + 1];
            right->djb2_hash_Key[item] = right->djb2_hash_Key[item + 1];
            right->data[item] = right->data[item + 1];
        }
        right->data_num--;
        fix_up(parent);
        return;
    }

    if (left != NULL)
    {
        for (int item = 0; item < leaf->data_num; item++)
        {
            int target = left->data_num + item;
            left->fnv1a_hash_key[target] = leaf->fnv1a_hash_key[item];
            left->djb2_hash_Key[target] = leaf->djb2_hash_Key[item];
            left->data[target] = leaf->data[item];
        }
        left->data_num += leaf->data_num;
        left->next = leaf->next;
        drop_child(parent, index);
        heap_free(leaf);
        balance_inner(root, parent);
        return;
    }

    if (right != NULL)
    {
        for (int item = 0; item < right->data_num; item++)
        {
            int target = leaf->data_num + item;
            leaf->fnv1a_hash_key[target] = right->fnv1a_hash_key[item];
            leaf->djb2_hash_Key[target] = right->djb2_hash_Key[item];
            leaf->data[target] = right->data[item];
        }
        leaf->data_num += right->data_num;
        leaf->next = right->next;
        drop_child(parent, index + 1);
        heap_free(right);
        balance_inner(root, parent);
    }
}

uint8_t bpt_delete(bpt_node *root, char *name)
{
    if (root == NULL || name == NULL)
    {
        return 0;
    }

    uint64_t fnv_key = fnv1a_hash_64(name);
    uint64_t djb_key = djb2_hash_64(name);
    bpt_node *leaf = search_leaf(root, fnv_key);

    if (leaf == NULL)
    {
        return 0;
    }

    int position = 0;
    while (position < leaf->data_num &&
           (leaf->fnv1a_hash_key[position] != fnv_key ||
            leaf->djb2_hash_Key[position] != djb_key))
    {
        position++;
    }
    if (position == leaf->data_num)
    {
        return 0;
    }

    for (int index = position; index < leaf->data_num - 1; index++)
    {
        leaf->fnv1a_hash_key[index] = leaf->fnv1a_hash_key[index + 1];
        leaf->djb2_hash_Key[index] = leaf->djb2_hash_Key[index + 1];
        leaf->data[index] = leaf->data[index + 1];
    }
    leaf->data_num--;
    balance_leaf(root, leaf);
    return 1;
}

uint16_t *bpt_search(bpt_node *root, char *name)
{
    if (root == NULL || name == NULL)
    {
        return NULL;
    }

    uint64_t fnv_key = fnv1a_hash_64(name);
    uint64_t djb_key = djb2_hash_64(name);
    bpt_node *leaf = search_leaf(root, fnv_key);
    if (leaf == NULL)
    {
        return NULL;
    }

    for (int index = 0; index < leaf->data_num; index++)
    {
        if (leaf->fnv1a_hash_key[index] == fnv_key &&
            leaf->djb2_hash_Key[index] == djb_key)
        {
            return &leaf->data[index];
        }
    }

    return NULL;
}
