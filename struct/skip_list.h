#pragma once
#include "basic_header.h"
#define MAX_LEVEL   32

template<typename K>
struct Compare {
	Int32 operator()(K key1, K key2) {
		if (key1 > key2) {
			return 1;
		}
		else if (key1 < key2) {
			return -1;
		}

		return 0;
	}
};

template<typename K, typename V>
struct SkipListNode {
	K key;
	V value;
	Int8 level;
	SkipListNode<K, V>* next[MAX_LEVEL];
};

template<typename K, typename V, typename Comparator = Compare<K>>
class SkipList {
public:
	SkipList(void);
	virtual ~SkipList(void);

	Int32 get(K key, V& value);
	Int32 erase(K key);

	Int32 put(K key, V value);
	Int32 size(void);

private:
	Int8 random_height(void);

private:
	Int32 size_;
	Comparator compare_;
	SkipListNode<K, V> header_;
	SkipListNode<K, V> tail_;
};

template<typename K, typename V, class Comparator>
SkipList<K, V, Comparator>::SkipList(void)
	:size_(0)
{
	header_.level = MAX_LEVEL;
	for (Int32 i = 0; i <= header_.level; ++i) {
		header_.next[i] = &tail_;
	}
}

template<typename K, typename V, class Comparator>
SkipList<K, V, Comparator>::~SkipList(void)
{
	SkipListNode<K, V>* next_ptr = header_.next[0];
	while (next_ptr != &tail_) {
		SkipListNode<K, V>* remove_ptr = next_ptr;
		next_ptr = remove_ptr->next[0];
		delete remove_ptr;
	}
}

template<typename K, typename V, class Comparator>
Int32 SkipList<K, V, Comparator>::get(K key, V& value)
{
	Int8 level = header_.level;
	SkipListNode<K, V>* ptr = &header_;
	while (level >= 0) {
		if (ptr->next[level] == &tail_ || compare_(ptr->next[level]->key, key) == 1) {
			if (level > 0) {
				--level;
			}
			else {
				break;
			}
		}
		else if (compare_(ptr->next[level]->key, key) == -1) {
			ptr = ptr->next[level];
		}
		else if (compare_(ptr->next[level]->key, key) == 0) {
			value = ptr->next[level]->value;
			return 0;
		}
	}

	return -1;
}

template<typename K, typename V, class Comparator>
Int32 SkipList<K, V, Comparator>::put(K key, V value)
{
	Int8 level = header_.level;
	SkipListNode<K, V>* ptr = &header_;
	SkipListNode<K, V>* new_node = new SkipListNode<K, V>;
	new_node->key = key;
	new_node->value = value;
	new_node->level = this->random_height();

	while (level >= 0) {
		if (ptr->next[level] == &tail_ || compare_(ptr->next[level]->key, key) == 1) {
			if (level <= new_node->level) {
				new_node->next[level] = ptr;
			}

			if (level > 0) {
				--level;
			}
			else {
				break;
			}
		}
		else if (compare_(ptr->next[level]->key, key) == -1) {
			ptr = ptr->next[level];
		}
		else if (compare_(ptr->next[level]->key, key) == 0) {
			delete new_node;
			return -1;
		}
	}

	for (Int32 i = new_node->level; i >= 0; --i) {
		SkipListNode<K, V>* tmp_ptr = new_node->next[i];
		if (tmp_ptr != &tail_) {
			new_node->next[i] = tmp_ptr->next[i];
			tmp_ptr->next[i] = new_node;
		}
		else {
			new_node->next[i] = &tail_;
		}
	}
	++size_;

	return 0;
}

template<typename K, typename V, class Comparator>
Int32 SkipList<K, V, Comparator>::erase(K key)
{
	Int8 level = header_.level;
	SkipListNode<K, V>* ptr = &header_, * remove_ptr = nullptr;
	while (level >= 0) {
		if (ptr->next[level] == &tail_ || compare_(ptr->next[level]->key, key) == 1) {
			if (level > 0) {
				--level;
			}
			else {
				break;
			}
		}
		else if (compare_(ptr->next[level]->key, key) == -1) {
			ptr = ptr->next[level];
		}
		else if (compare_(ptr->next[level]->key, key) == 0) {
			remove_ptr = ptr->next[level];
			ptr->next[level] = remove_ptr->next[level];
			--level;
		}
	}

	if (remove_ptr != nullptr) {
		--size_;
		delete remove_ptr;

		return 0;
	}


	return -1;
}

template<typename K, typename V, class Comparator>
Int32 SkipList<K, V, Comparator>::size(void)
{
	return size_;
}

template<typename K, typename V, class Comparator>
Int8 SkipList<K, V, Comparator>::random_height(void)
{
	Int8 height = 0;
	for (Int32 i = 0; i < MAX_LEVEL; ++i) {
		if (rand() % 2 == 1) {
			++height;
		}
	}

	return height;
}

//template<typename K, typename V, class Comparator>
//void SkipList<K, V,Comparator>::print(void)
//{
//    SkipListNode<K, V>* ptr = &header_;
//
//    while (ptr->next[0] != &tail_) {
//        std::cout << "key: " << ptr->next[0]->key << ", value: " << ptr->next[0]->value << std::endl;
//        ptr = ptr->next[0];
//    }
//}
//
//template<typename K, typename V, class Comparator>
//void SkipList<K, V,Comparator>::print_rand()
//{
//    for (int i = 0; i < MAX_LEVEL; ++i) {
//        std::cout << i << ": " << (rand_num[i]) << std::endl;
//    }
//}
