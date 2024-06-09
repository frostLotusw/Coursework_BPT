#ifndef BPT_B_PLUS_TREE_H
#define BPT_B_PLUS_TREE_H

#include <iostream>
#include <cstring>
#include "page_manager.h"
#include "utils/hash.h"
#include "utils/binary_search.h"

class BPlusTree {

public:

    static constexpr int leaf_size = 48, internal_size = 336;
    static constexpr int leaf_merge_size = 16, internal_merge_size = 112;
    static constexpr char data_path[] = "data.bin", info_path[] = "info.bin", root_path[] = "root.bin";

    struct Data {
        char str[65];
        long long index; // hash << 32 + value

        bool operator<(const Data &other) const {
            return index < other.index;
        }
    };

    typedef int FilePos;

    class InternalNode;

    class LeafNode;

    class StorageInterface;

    class Node {
    public:
        virtual ~Node() = default;

        static void serialize(std::fstream &out, Node *obj_ptr) {
            obj_ptr->serialize(out);
        }

        static void deserialize(std::fstream &in, char *ptr) {
            int node_type;
            in.read(reinterpret_cast<char *>(&node_type), sizeof(int));

            Node *obj_ptr;

            switch (node_type) {
                case 0:
                    obj_ptr = new(ptr) LeafNode;
                    break;
                case 1:
                    obj_ptr = new(ptr) InternalNode;
                    break;
            }

            obj_ptr->deserialize(in);
        }

        virtual void serialize(std::fstream &out) = 0;

        virtual void deserialize(std::fstream &in) = 0;
    };

    class InternalNode : public Node {

    public:

        long long index[internal_size - 1];
        FilePos child[internal_size];
        int size;


        InternalNode() {
            memset(index, 0, sizeof(long long) * (internal_size - 1));
            memset(child, 0, sizeof(int) * internal_size);
            size = 0;
        }

        void serialize(std::fstream &out) override {
            int node_type = 1;
            out.write(reinterpret_cast<char *>(&node_type), sizeof(int));
            out.write(reinterpret_cast<char *>(index), sizeof(long long) * (internal_size - 1));
            out.write(reinterpret_cast<char *>(child), sizeof(int) * internal_size);
            out.write(reinterpret_cast<char *>(&size), sizeof(int));
        }

        void deserialize(std::fstream &in) override {
            in.read(reinterpret_cast<char *>(index), sizeof(long long) * (internal_size - 1));
            in.read(reinterpret_cast<char *>(child), sizeof(int) * internal_size);
            in.read(reinterpret_cast<char *>(&size), sizeof(int));
        }

        void insert(long long new_index, FilePos new_child, int cursor) {
            if (cursor < size) {
                memmove(index + cursor, index + cursor - 1, sizeof(long long) * (size - cursor));
                memmove(child + cursor + 1, child + cursor, sizeof(FilePos) * (size - cursor));
            }
            index[cursor - 1] = new_index;
            child[cursor] = new_child;
            ++size;
        }

        void remove(int cursor) {
            if (cursor < size - 1) {
                memmove(index + cursor - 1, index + cursor, sizeof(long long) * (size - cursor - 1));
                memmove(child + cursor, child + cursor + 1, sizeof(FilePos) * (size - cursor - 1));
            }
            --size;
        }

        void insert_head(long long new_index, FilePos new_child) {
            memmove(index + 1, index, sizeof(long long) * (size - 1));
            memmove(child + 1, child, sizeof(FilePos) * size);
            index[0] = new_index;
            child[0] = new_child;
            ++size;
        }

        void remove_head() {
            memmove(index, index + 1, sizeof(long long) * (size - 2));
            memmove(child, child + 1, sizeof(FilePos) * (size - 1));
            --size;
        }
    };

    class LeafNode : public Node {

    public:

        Data data[leaf_size];
        FilePos next;
        int size;

        LeafNode() {
            memset(data, 0, sizeof(Data) * leaf_size);
            next = -1;
            size = 0;
        }

        void serialize(std::fstream &out) override {
            int node_type = 0;
            out.write(reinterpret_cast<char *>(&node_type), sizeof(int));
            out.write(reinterpret_cast<char *>(data), sizeof(Data) * leaf_size);
            out.write(reinterpret_cast<char *>(&next), sizeof(int));
            out.write(reinterpret_cast<char *>(&size), sizeof(int));
        }

        void deserialize(std::fstream &in) override {
            in.read(reinterpret_cast<char *>(data), sizeof(Data) * leaf_size);
            in.read(reinterpret_cast<char *>(&next), sizeof(int));
            in.read(reinterpret_cast<char *>(&size), sizeof(int));
        }

        void insert(Data &new_data, int cursor) {
            if (cursor < size)
                memmove(data + cursor + 1, data + cursor, sizeof(Data) * (size - cursor));
            data[cursor] = new_data;
            ++size;
        }

        void remove(int cursor) {
            if (cursor < size - 1)
                memmove(data + cursor, data + cursor + 1, sizeof(Data) * (size - cursor - 1));
            --size;
        }
    };

    class StorageInterface {

        PageManager<Node, 4096, 4096> pages;

    public:

        StorageInterface() :
                pages(data_path, info_path) {}

        Node *operator[](FilePos index) {
            char *page = pages[index];
            return reinterpret_cast<Node *>(page);
        }

        FilePos new_leaf() {
            return pages.alloc_page<LeafNode>();
        }

        FilePos new_internal() {
            return pages.alloc_page<InternalNode>();
        }

        void free(FilePos index) {
            pages.free_page(index);
        }
    };

private:

    StorageInterface storage;

    FilePos root_pos;

    Vector<FilePos> recursive_par;
    Vector<int> recursive_cursor;

    Data data_in_operation;

    void maintain_index_recursive(long long new_index, int recursive_layer) {

        // called when leaf->data[0] or internal->child[0] modified

        --recursive_layer;
        while (recursive_layer >= 0) {
            if (recursive_cursor[recursive_layer]) {
                InternalNode *internal = dynamic_cast<InternalNode *>(storage[recursive_par[recursive_layer]]);
                internal->index[recursive_cursor[recursive_layer] - 1] = new_index;
                return;
            }
            --recursive_layer;
        }
    }

    void insert_recursive(FilePos file_pos, int recursive_layer = 0) {

        Node *node = storage[file_pos];

        if (LeafNode *leaf = dynamic_cast<LeafNode *>(node)) {

            int insert_cursor = binary_search(leaf->data, leaf->size, data_in_operation);
            leaf->insert(data_in_operation, insert_cursor);

            // insert_cursor == 0 && recursive_layer: iff left-most leaf

            if (leaf->size == leaf_size) { // split

                FilePos next_pos = storage.new_leaf();
                LeafNode *next = dynamic_cast<LeafNode *>(storage[next_pos]);

                leaf->size = leaf_size / 2;
                next->size = leaf_size / 2;
                next->next = leaf->next;
                leaf->next = next_pos;
                memcpy(
                        next->data,
                        leaf->data + leaf_size / 2,
                        sizeof(Data) * leaf_size / 2
                );

                if (!recursive_layer) { // root
                    root_pos = storage.new_internal();
                    InternalNode *root = dynamic_cast<InternalNode *>(storage[root_pos]);

                    root->index[0] = next->data[0].index;
                    root->child[0] = file_pos;
                    root->child[1] = next_pos;
                    root->size = 2;
                }
                else {
                    InternalNode *par = dynamic_cast<InternalNode *>(storage[recursive_par[recursive_layer - 1]]);
                    par->insert(next->data[0].index, next_pos, recursive_cursor[recursive_layer - 1] + 1);
                }
            }
        }

        else if (InternalNode *internal = dynamic_cast<InternalNode *>(node)) {

            recursive_cursor[recursive_layer] = binary_search(internal->index, internal->size - 1,
                                                              data_in_operation.index);
            recursive_par[recursive_layer] = file_pos;

            insert_recursive(internal->child[recursive_cursor[recursive_layer]], recursive_layer + 1);

            internal = dynamic_cast<InternalNode *>(storage[file_pos]); // previous cache might be evicted

            if (internal->size == internal_size) {

                FilePos next_pos = storage.new_internal();
                InternalNode *next = dynamic_cast<InternalNode *>(storage[next_pos]);

                long long up_move_index = internal->index[internal_size / 2 - 1];
                internal->size = internal_size / 2;
                next->size = internal_size / 2;
                memcpy(
                        next->index,
                        internal->index + internal_size / 2,
                        sizeof(long long) * (internal_size / 2 - 1)
                );
                memcpy(
                        next->child,
                        internal->child + internal_size / 2,
                        sizeof(int) * internal_size / 2
                );

                if (!recursive_layer) { // root
                    root_pos = storage.new_internal();
                    InternalNode *root = dynamic_cast<InternalNode *>(storage[root_pos]);

                    root->index[0] = up_move_index;
                    root->child[0] = file_pos;
                    root->child[1] = next_pos;
                    root->size = 2;
                }
                else {
                    InternalNode *par = dynamic_cast<InternalNode *>(storage[recursive_par[recursive_layer - 1]]);
                    par->insert(up_move_index, next_pos, recursive_cursor[recursive_layer - 1] + 1);
                }
            }
        }
    }


    bool remove_recursive(FilePos file_pos, int recursive_layer = 0) {

        Node *node = storage[file_pos];

        if (LeafNode *leaf = dynamic_cast<LeafNode *>(node)) {

            int remove_cursor = binary_search(leaf->data, leaf->size, data_in_operation);

            while (true) {
                if (remove_cursor == leaf->size)
                    return false;
                if (data_in_operation.index < leaf->data[remove_cursor].index)
                    return false;

                if (strcmp(data_in_operation.str, leaf->data[remove_cursor].str) == 0) {
                    leaf->remove(remove_cursor);
                    if (remove_cursor == 0 && recursive_layer) {
                        maintain_index_recursive(leaf->data[0].index, recursive_layer);
                        leaf = dynamic_cast<LeafNode *>(storage[file_pos]);
                    }
                    break;
                }
                ++remove_cursor;
            }

            if (leaf->size < leaf_merge_size && recursive_layer) {

                InternalNode *par = dynamic_cast<InternalNode *>(storage[recursive_par[recursive_layer - 1]]);
                int par_insert_cursor = recursive_cursor[recursive_layer - 1];
                LeafNode *left_bro = nullptr, *right_bro = nullptr;

                if (par_insert_cursor > 0)
                    left_bro = dynamic_cast<LeafNode *>(storage[par->child[par_insert_cursor - 1]]);
                if (par_insert_cursor < par->size - 1)
                    right_bro = dynamic_cast<LeafNode *>(storage[par->child[par_insert_cursor + 1]]);

                if (left_bro && left_bro->size > leaf_merge_size) {
                    leaf->insert(left_bro->data[left_bro->size - 1], 0);
                    --left_bro->size;
                    par->index[par_insert_cursor - 1] = leaf->data[0].index;
                }
                else if (right_bro && right_bro->size > leaf_merge_size) {
                    leaf->insert(right_bro->data[0], leaf->size);
                    right_bro->remove(0);
                    par->index[par_insert_cursor] = right_bro->data[0].index;
                }
                else if (left_bro) {
                    memcpy(
                            left_bro->data + left_bro->size,
                            leaf->data,
                            sizeof(Data) * leaf->size
                    );
                    left_bro->size += leaf->size;
                    left_bro->next = leaf->next;
                    storage.free(file_pos);
                    par->remove(par_insert_cursor);
                }
                else if (right_bro) {
                    memcpy(
                            leaf->data + leaf->size,
                            right_bro->data,
                            sizeof(Data) * right_bro->size
                    );
                    leaf->size += right_bro->size;
                    leaf->next = right_bro->next;
                    storage.free(par->child[par_insert_cursor + 1]);
                    par->remove(par_insert_cursor + 1);
                }
            }
        }

        else if (InternalNode *internal = dynamic_cast<InternalNode *>(node)) {

            recursive_cursor[recursive_layer] = binary_search(internal->index, internal->size - 1,
                                                              data_in_operation.index);
            recursive_par[recursive_layer] = file_pos;

            while (true) {
                if (remove_recursive(internal->child[recursive_cursor[recursive_layer]], recursive_layer + 1))
                    break;

                internal = dynamic_cast<InternalNode *>(storage[file_pos]); // previous cache might be evicted

                if (recursive_cursor[recursive_layer] == internal->size - 1)
                    return false;
                if (data_in_operation.index < internal->index[recursive_cursor[recursive_layer]])
                    return false;

                ++recursive_cursor[recursive_layer];
            }

            internal = dynamic_cast<InternalNode *>(storage[file_pos]);

            if (internal->size < internal_merge_size && recursive_layer) {

                InternalNode *par = dynamic_cast<InternalNode *>(storage[recursive_par[recursive_layer - 1]]);
                int par_insert_cursor = recursive_cursor[recursive_layer - 1];
                InternalNode *left_bro = nullptr, *right_bro = nullptr;

                if (par_insert_cursor > 0)
                    left_bro = dynamic_cast<InternalNode *>(storage[par->child[par_insert_cursor - 1]]);
                if (par_insert_cursor < par->size - 1)
                    right_bro = dynamic_cast<InternalNode *>(storage[par->child[par_insert_cursor + 1]]);

                if (left_bro && left_bro->size > internal_merge_size) {
                    internal->insert_head(par->index[par_insert_cursor - 1], left_bro->child[left_bro->size - 1]);
                    par->index[par_insert_cursor - 1] = left_bro->index[left_bro->size - 2];
                    --left_bro->size;
                }
                else if (right_bro && right_bro->size > internal_merge_size) {
                    internal->insert(par->index[par_insert_cursor], right_bro->child[0], internal->size);
                    par->index[par_insert_cursor] = right_bro->index[0];
                    right_bro->remove_head();
                }
                else if (left_bro) {
                    memcpy(
                            left_bro->index + left_bro->size,
                            internal->index,
                            sizeof(long long) * (internal->size - 1)
                    );
                    left_bro->index[left_bro->size - 1] = par->index[par_insert_cursor - 1];
                    memcpy(
                            left_bro->child + left_bro->size,
                            internal->child,
                            sizeof(FilePos) * internal->size
                    );
                    left_bro->size += internal->size;
                    storage.free(file_pos);
                    par->remove(par_insert_cursor);
                }
                else if (right_bro) {
                    memcpy(
                            internal->index + internal->size,
                            right_bro->index,
                            sizeof(long long) * (right_bro->size - 1)
                    );
                    internal->index[internal->size - 1] = par->index[par_insert_cursor];
                    memcpy(
                            internal->child + internal->size,
                            right_bro->child,
                            sizeof(FilePos) * right_bro->size
                    );
                    internal->size += right_bro->size;
                    storage.free(par->child[par_insert_cursor + 1]);
                    par->remove(par_insert_cursor + 1);
                }
            }

            else if (internal->size == 1 && !recursive_layer) {
                root_pos = internal->child[0];
                storage.free(file_pos);
            }
        }

        return true;
    }

public:

    explicit BPlusTree(bool reset = false) : storage() {

        if (reset) {
            std::remove(data_path);
            std::remove(info_path);
            std::remove(root_path);
            root_pos = storage.new_leaf();
        }
        else {
            std::fstream root_file;

            root_file.open(
                    root_path,
                    std::fstream::in | std::fstream::binary
            );

            if (root_file.is_open()) {
                root_file.seekg(0);
                root_file.read(reinterpret_cast<char *>(&root_pos), sizeof(int));
            }
            else
                root_pos = storage.new_leaf();

            root_file.close();
        }

        recursive_par.resize(64);
        recursive_cursor.resize(64);
    }

    ~BPlusTree() {

        std::fstream root_file;

        root_file.open(
                root_path,
                std::fstream::out | std::fstream::binary
        );

        root_file.write(reinterpret_cast<char *>(&root_pos), sizeof(int));
        root_file.close();
    }

    void insert(const char *key, int value) {
        strcpy(data_in_operation.str, key);
        data_in_operation.index = ((long long) hash(key) << 32) + value;
        insert_recursive(root_pos);
    }

    void remove(const char *key, int value) {
        strcpy(data_in_operation.str, key);
        data_in_operation.index = ((long long) hash(key) << 32) + value;
        remove_recursive(root_pos);
    }

    void print_value(const char *key) {

        long long index = (long long) hash(key) << 32;
        FilePos cur_pos = root_pos;
        Node *cur = storage[cur_pos];

        while (InternalNode *internal = dynamic_cast<InternalNode *>(cur)) {
            cur_pos = internal->child[binary_search(internal->index, internal->size - 1, index)];
            cur = storage[cur_pos];
        }

        LeafNode *leaf = dynamic_cast<LeafNode *>(cur);
        data_in_operation.index = index;
        int find_cursor = binary_search(leaf->data, leaf->size, data_in_operation);

        bool found = false;

        while (true) {
            if (find_cursor == leaf->size) {
                if (leaf->next == -1)
                    break;
                leaf = dynamic_cast<LeafNode *>(storage[leaf->next]);
                find_cursor = 0;
            }

            if (leaf->data[find_cursor].index - index >= (1ll << 32))
                break;

            if (strcmp(key, leaf->data[find_cursor].str) == 0) {
                found = true;
                std::cout << (int) leaf->data[find_cursor].index << ' ';
            }
            ++find_cursor;
        }

        if (!found)
            std::cout << "null\n";
        else std::cout << '\n';
    }
};

#endif
