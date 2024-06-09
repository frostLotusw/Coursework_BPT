#ifndef UTILS_HEAP_H
#define UTILS_HEAP_H

#include <utility>
#include "vector.h"
#include "pair.h"

template<typename T, bool using_key = false>
class Heap;

template<typename T>
class Heap<T, false> : protected Vector<T> {

protected:

    using Vector<T>::data;
    using Vector<T>::pos;
    using Vector<T>::push_back;

    void swap(int index_a, int index_b) {
        T tmp = data[index_a];
        data[index_a] = data[index_b];
        data[index_b] = tmp;
    }

    void move_up(int index) {
        int par;
        while (index) {
            par = (index - 1) / 2;
            if (!(data[index] < data[par]))
                break;

            swap(index, par);
            index = par;
        }
    }

    void move_down(int index) {
        int left, right, smallest;
        while (true) {
            left = index * 2 + 1;
            right = index * 2 + 2;
            smallest = index;

            if (left < pos && data[left] < data[smallest]) {
                smallest = left;
            }
            if (right < pos && data[right] < data[smallest]) {
                smallest = right;
            }

            if (smallest != index) {
                swap(index, smallest);
                index = smallest;
            }
            else
                break;
        }
    }

public:

    Heap() : Vector<T>() {}

    ~Heap() = default;

    template<typename U>
    void push(U &&val) {
        push_back(std::forward<U>(val));
        move_up(pos - 1);
    }

    T &top() {
        return data[0];
    }

    const T *raw() {
        return data;
    }

    void pop() {
        if (pos <= 0)
            return;
        if (pos == 1) {
            --pos;
            return;
        }

        data[0] = data[--pos];
        move_down(0);
    }

    int size() {
        return pos;
    }
};

template<typename T>
class Heap<T, true> : protected Vector<Pair<int, T>> {

protected:

    using Vector<Pair<int, T>>::data;
    using Vector<Pair<int, T>>::pos;
    using Vector<Pair<int, T>>::push_back;

    Vector<int> key_map;

    void swap(int index_a, int index_b) {
        Pair<int, T> tmp_T = data[index_a];
        data[index_a] = data[index_b];
        data[index_b] = tmp_T;

        key_map[data[index_a].first] = index_a;
        key_map[data[index_b].first] = index_b;
    }

    void move_up(int index) {
        int par;
        while (index) {
            par = (index - 1) / 2;
            if (!(data[index].second < data[par].second))
                break;

            swap(index, par);
            index = par;
        }
    }

    void move_down(int index) {
        int left, right, smallest;
        while (true) {
            left = index * 2 + 1;
            right = index * 2 + 2;
            smallest = index;

            if (left < pos && data[left].second < data[smallest].second) {
                smallest = left;
            }
            if (right < pos && data[right].second < data[smallest].second) {
                smallest = right;
            }

            if (smallest != index) {
                swap(index, smallest);
                index = smallest;
            }
            else
                break;
        }
    }

public:

    Heap() : Vector<Pair<int, T>>() {}

    ~Heap() = default;

    template<typename U>
    void push(int key, U &&val) {
        int map_size = key_map.size();
        if (key >= map_size) {
            key_map.resize(key + 1);
            for (int i = map_size; i < key; ++i)
                key_map[i] = -1;

            key_map[key] = pos;
            push_back(Pair<int, T>(key, std::forward<U>(val)));
            move_up(pos - 1);
        }
        else {
            int index = key_map[key];
            if (index == -1) {
                key_map[key] = pos;
                push_back(Pair<int, T>(key, std::forward<U>(val)));
                move_up(pos - 1);
            }
            else {
                if (std::forward<U>(val) < data[index].second) {
                    data[index].second = std::forward<U>(val);
                    move_up(index);
                }
                else if (data[index].second < std::forward<U>(val)) {
                    data[index].second = std::forward<U>(val);
                    move_down(index);
                }
            }
        }
    }

    const T &at(int key) {
        return data[key_map[key]].second;
    }

    Pair<int, T> top() {
        return data[0];
    }

    const Pair<int, T> *raw() {
        return data;
    }

    void pop() {
        if (pos <= 0)
            return;
        if (pos == 1) {
            key_map[data[0].first] = -1;
            --pos;
            return;
        }

        key_map[data[0].first] = -1;
        data[0] = data[--pos];
        key_map[data[0].first] = 0;
        move_down(0);
    }

    int size() {
        return pos;
    }

};

#endif
