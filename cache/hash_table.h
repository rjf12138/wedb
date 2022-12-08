#ifndef __HASH_TABLE_H__
#define __HASH_TABLE_H__

#include <cstdarg>

template <typename V>
class Buckets {
public:
    Buckets(void);
    ~Buckets(void);

    
private:
    V * array_;
};

template <typename V>
class HashTable {
public:
    HashTable(void);
    ~HashTable(void);

    bool exist(int key);
    bool insert(int key, const V& value);
    bool remove(int key);
    int32_t size(void);

private:
    int32_t resize(bool grow);
    void when_resize_hash_table(void);

private:
    int32_t size_;
};

#endif