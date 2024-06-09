#ifndef BPT_PAGE_MANAGER_H
#define BPT_PAGE_MANAGER_H

#include <fstream>
#include <cstring>
#include <utility>
#include "utils/vector.h"
#include "utils/qsort.h"
#include "utils/pair.h"
#include "utils/heap.h"

template<typename data_type, int page_size, int cache_limit>
class PageManager {

    typedef int MemoryPos;
    typedef int FilePos;

    struct CacheElement {
        MemoryPos mem_pos;
        int priority;

        CacheElement() = default;

        CacheElement(MemoryPos mem_pos, int priority) :
                mem_pos(mem_pos), priority(priority) {}

        template<typename U>
        bool operator<(U &&other) {
            return priority < other.priority;
        }
    };

    class CacheHeap : public Heap<CacheElement, true> {

        /*
         * later I realized this can be implemented through a list...
         */

        using Heap<CacheElement, true>::data;
        using Heap<CacheElement, true>::pos;
        using Heap<CacheElement, true>::key_map;
        using Heap<CacheElement, true>::push;

        int max_priority;

        static constexpr int PRIORITY_LIMIT = 1073741824;

        static bool comp_priority(const Pair<int, CacheElement> &a, const Pair<int, CacheElement> &b) {
            return a.second.priority < b.second.priority;
        }

        void discretize_priority() {
            qsort(data, data + pos, comp_priority);
            for (int i = 0; i < pos; ++i)
                data[i].second.priority = i;
            max_priority = pos - 1;
        }

    public:

        CacheHeap() : Heap<CacheElement, true>() {
            max_priority = 0;
        }

        MemoryPos operator[](FilePos key) {
            if (key >= key_map.size())
                return -1;

            int index = key_map[key];
            if (index == -1)
                return -1;
            return data[index].second.mem_pos;
        }

        void reset_priority(FilePos key) {
            int index = key_map[key];
            if (index == -1)
                return;

            if (max_priority >= PRIORITY_LIMIT)
                discretize_priority();

            data[index].second.priority = ++max_priority;
        }

        void insert(FilePos key, MemoryPos mem_pos) {
            if (max_priority >= PRIORITY_LIMIT)
                discretize_priority();

            push(key, CacheElement(mem_pos, ++max_priority));
        }

    };

    CacheHeap cache_heap;

    Heap<FilePos> recycle_heap;

    FilePos file_size;

    std::fstream data_file;

    std::string data_path, info_path;

    char *pages;

public:

    PageManager(const std::string &data_path, const std::string &info_path) :
            data_path(data_path), info_path(info_path) {

        std::fstream info_file(
                info_path,
                std::fstream::in | std::fstream::binary
        );

        if (info_file.is_open()) {

            info_file.read(reinterpret_cast<char *>(&file_size), sizeof(int));

            int recycle_size;
            info_file.read(reinterpret_cast<char *>(&recycle_size), sizeof(int));
            int *recycle_arr = new int[recycle_size];
            info_file.read(reinterpret_cast<char *>(recycle_arr), (long long) sizeof(int) * recycle_size);
            for (int i = 0; i < recycle_size; ++i)
                recycle_heap.push(recycle_arr[i]);
            delete[] recycle_arr;

            info_file.close();
        }
        else {
            file_size = 0;
        }

        pages = new char[page_size * cache_limit];
        data_file.open(
                data_path,
                std::fstream::in | std::fstream::out | std::fstream::binary
        );

        if (!data_file.is_open()) {
            data_file.clear();
            data_file.open(
                    data_path,
                    std::fstream::out | std::fstream::binary
            );
            data_file.close();
            data_file.open(
                    data_path,
                    std::fstream::in | std::fstream::out | std::fstream::binary
            );
        }
    }

    ~PageManager() {

        int recycle_size = recycle_heap.size();
        int *recycle_arr = new int[recycle_size];
        memcpy(recycle_arr, recycle_heap.raw(), sizeof(int) * recycle_size);
        qsort(recycle_arr, recycle_arr + recycle_size);
        while (recycle_size) {
            if (recycle_arr[recycle_size - 1] == file_size - 1) {
                --recycle_size;
                --file_size;
            }
            else
                break;
        }

        std::fstream info_file(
                info_path,
                std::fstream::out | std::fstream::trunc | std::fstream::binary
        );

        info_file.write(reinterpret_cast<char *>(&file_size), sizeof(int));
        info_file.write(reinterpret_cast<char *>(&recycle_size), sizeof(int));
        info_file.write(reinterpret_cast<char *>(recycle_arr), (long long) sizeof(int) * recycle_size);

        delete[] recycle_arr;
        info_file.close();

        Pair<FilePos, CacheElement> top;
        while (cache_heap.size()) {
            top = cache_heap.top();
            cache_heap.pop();
            data_file.seekp(page_size * top.first);
            data_type::serialize(data_file, reinterpret_cast<data_type *>(pages + page_size * top.second.mem_pos));
        }

        delete[] pages;
        data_file.close();
    }

    char *operator[](FilePos file_pos) {

        MemoryPos mem_pos = cache_heap[file_pos];

        if (mem_pos != -1) {
            cache_heap.reset_priority(file_pos);
        }
        else {
            if (cache_heap.size() < cache_limit) {
                mem_pos = cache_heap.size();
            }
            else {
                Pair<FilePos, CacheElement> top = cache_heap.top();
                mem_pos = top.second.mem_pos;
                cache_heap.pop();
                data_file.seekp(page_size * top.first);
                data_type::serialize(data_file, reinterpret_cast<data_type *>(pages + page_size * mem_pos));
            }

            data_file.seekg(page_size * file_pos);
            data_type::deserialize(data_file, pages + page_size * mem_pos);
            if (data_file.eof())
                data_file.clear();
            cache_heap.insert(file_pos, mem_pos);
        }

        return pages + page_size * mem_pos;
    }

    template<typename alloc_type>
    FilePos alloc_page() {

        FilePos alloc_pos;
        MemoryPos mem_pos;

        if (recycle_heap.size()) {
            FilePos top = recycle_heap.top();
            recycle_heap.pop();
            alloc_pos = top;
        }
        else
            alloc_pos = file_size++;

        if (cache_heap.size() < cache_limit) {
            mem_pos = cache_heap.size();
        }
        else {
            Pair<FilePos, CacheElement> top = cache_heap.top();
            mem_pos = top.second.mem_pos;
            cache_heap.pop();
            data_file.seekp(page_size * top.first);
            data_type::serialize(data_file, reinterpret_cast<data_type *>(pages + page_size * mem_pos));
        }

        new(pages + page_size * mem_pos) alloc_type;
        cache_heap.insert(alloc_pos, mem_pos);
        return alloc_pos;
    }

    void free_page(FilePos file_pos) {
        recycle_heap.push(file_pos);
    }

    FilePos size() {
        return file_size;
    }

};

#endif
