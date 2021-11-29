#ifndef __DOUBLY_LIST__
#define __DOUBLY_LIST__
#include "basic_header.h"

template<typename V>
struct DoublyListNode {
	V value;
	DoublyListNode<V>* prev, * next;

	DoublyListNode(const V& v, DoublyListNode<V>* p = nullptr, DoublyListNode<V>* n = nullptr)
		:value(v), prev(p), next(n)
	{}
};

template <typename V>
class DoublyList {
public:
	DoublyList(void);
	virtual ~DoublyList(void);

	class Iterator {
		friend class DoublyList;
	public:
		Iterator(void);
		~Iterator(void);

		V& operator*();
		Bool operator==(const Iterator& rhs);
		Bool operator!=(const Iterator& rhs);
		Iterator& operator=(const Iterator& rhs);

		Iterator& operator++();
		Iterator operator++(Int32); // 后置

		Iterator& operator--();
		Iterator operator--(Int32); // 前置

	private:
		const DoublyList* list_;
		DoublyListNode* node_;
	};

	Iterator begin(void);
	Iterator end(void);

	Int32 push_back(const V& value);
	Int32 insert_front(const Iterator& iter, const V& value);
	Int32 insert_back(const Iterator& iter, const V& value);
	Int32 erase(const Iterator& iter);
	Int32 clear(void);
	UInt32 size(void) const;

private:
	DoublyListNode<V>* get_node(const Iterator& iter);

private:
	UInt32 size_;
	DoublyListNode<V> head_;
	DoublyListNode<V> tail_;
};

template <typename V>
DoublyList<V>::DoublyList(void)
	:size_(0),
	head_(nullptr, &tail_),
	tail_(&head_, nullptr)
{
}

template <typename V>
DoublyList<V>::~DoublyList(void)
{
	this->clear();
}

template <typename V>
UInt32 DoublyList<V>::size(void) const
{
	return size_;
}

template <typename V>
Int32 DoublyList<V>::clear(void)
{
	DoublyList<V>::Iterator iter = this->begin();
	while (iter != this->end()) {
		this->erase(iter);
		iter = this->begin();
	}

	return 0;
}

template <typename V>
DoublyList<V>::Iterator DoublyList<V>::begin(void)
{
	Iterator iter(this);
	iter.node_ = head_->next;

	return iter;
}

template <typename V>
DoublyList<V>::Iterator DoublyList<V>::end(void)
{
	Iterator iter(this);
	iter.node_ = &tail_;

	return iter;
}

template <typename V>
DoublyListNode<V>* DoublyList<V>::get_node(const Iterator& iter)
{
	if (iter.list_ != this) {
		return nullptr;
	}

	return iter.node_;
}

template <typename V>
Int32 DoublyList<V>::push_back(const V& value)
{
	DoublyListNode<V>* new_node = new DoublyListNode<V>(value, tail_.prev, &tail_);
	tail_.prev->next = new_node;
	tail_.prev = new_node;
	++size_;

	return 0;
}

template <typename V>
Int32 DoublyList<V>::insert_front(const Iterator& iter, const V& value)
{
	DoublyListNode<V>* curr_node = this->get_node(iter);
	if (curr_node == nullptr || curr_node == &head_) {
		throw std::runtime_error("DoublyList<V>::insert_front(): Insert value in the wrong postion.");
	}

	DoublyListNode<V>* new_node = new DoublyListNode<V>(value, curr_node->prev, curr_node);
	curr_node->prev->next = new_node;
	curr_node->prev = new_node;
	++size_;

	return 0;
}

template <typename V>
Int32 DoublyList<V>::insert_back(const Iterator& iter, const V& value)
{
	DoublyListNode<V>* curr_node = this->get_node(iter);
	if (curr_node == nullptr || curr_node == &tail_) {
		throw std::runtime_error("DoublyList<V>::insert_back(): Insert value in the wrong postion.");
	}

	DoublyListNode<V>* new_node = new DoublyListNode<V>(value, curr_node, curr_node->next);
	curr_node->next->prev = new_node;
	curr_node->next = new_node;
	++size_;

	return 0;
}

template <typename V>
Int32 DoublyList<V>::erase(const Iterator& iter)
{
	DoublyListNode<V>* curr_node = this->get_node(iter);
	if (curr_node == nullptr || curr_node == &head_ || curr_node == &tail_) {
		throw std::runtime_error("DoublyList<V>::erase(): Erase the value in the wrong postion.");
	}

	curr_node->prev->next = curr_node->next;
	curr_node->next->prev = curr_node->prev;

	--size_;
	delete curr_node;

	return 0;
}

////////////////////////////////////////////////////

template <typename V>
DoublyList<V>::Iterator::Iterator(void)
	:list_(nullptr), node_(nullptr)
{

}


template <typename V>
DoublyList<V>::Iterator::~Iterator(void)
{

}

template <typename V>
V& DoublyList<V>::Iterator::operator*()
{
	if (list_ == nullptr || node_ == nullptr) {
		throw std::runtime_error("DoublyList<V>::Iterator::operator*(): Iterator not pointer any DoublyList node.");
	}

	if (node_ == &list_->head_ || node_ == &list->tail_) {
		throw std::runtime_error("DoublyList<V>::Iterator::operator*(): Iterator not pointer any DoublyList node.");
	}

	return node_->value;
}

template <typename V>
Bool DoublyList<V>::Iterator::operator==(const DoublyList<V>::Iterator& rhs)
{
	if (list_ == nullptr || node_ == nullptr) {
		return False;
	}

	if (list_ == rhs.list_ && node_ == rhs.node_) {
		return True;
	}

	return False;
}

template <typename V>
Bool DoublyList<V>::Iterator::operator!=(const DoublyList<V>::Iterator& rhs)
{
	return !(*this == rhs);
}

template <typename V>
DoublyList<V>::Iterator& DoublyList<V>::Iterator::operator=(const DoublyList<V>::Iterator& rhs)
{
	list_ = rhs.list_;
	node_ = rhs.node_;

	return *this;
}

template <typename V>
DoublyList<V>::Iterator& DoublyList<V>::Iterator::operator++()
{
	if (list_ == nullptr || node_ == nullptr) {
		throw std::runtime_error("DoublyList<V>::Iterator::operator++(): Iterator not pointer any DoublyList node.");
	}

	if (node_ == &list_->head_ || node_ == &list->tail_) {
		throw std::runtime_error("DoublyList<V>::Iterator::operator++(): Iterator not pointer any DoublyList node.");
	}

	node_ = node_->next;
	return *this;
}

template <typename V>
DoublyList<V>::Iterator DoublyList<V>::Iterator::operator++(Int32)
{
	if (list_ == nullptr || node_ == nullptr) {
		throw std::runtime_error("DoublyList<V>::Iterator::operator++(Int32): Iterator not pointer any DoublyList node.");
	}

	if (node_ == &list_->head_ || node_ == &list->tail_) {
		throw std::runtime_error("DoublyList<V>::Iterator::operator++(Int32): Iterator not pointer any DoublyList node.");
	}

	Iterator tmp = *this;
	node_ = node_->next;

	return tmp;
}

template <typename V>
DoublyList<V>::Iterator& DoublyList<V>::Iterator::operator--()
{
	if (list_ == nullptr || node_ == nullptr) {
		throw std::runtime_error("DoublyList<V>::Iterator::operator--(): Iterator not pointer any DoublyList node.");
	}

	if (node_ == &list_->head_ || node_ == &list->tail_) {
		throw std::runtime_error("DoublyList<V>::Iterator::operator--(): Iterator not pointer any DoublyList node.");
	}

	node_ = node_->prev;
	return *this;
}

template <typename V>
DoublyList<V>::Iterator DoublyList<V>::Iterator::operator--(Int32)
{
	if (list_ == nullptr || node_ == nullptr) {
		throw std::runtime_error("DoublyList<V>::Iterator::operator--(Int32): Iterator not pointer any DoublyList node.");
	}

	if (node_ == &list_->head_ || node_ == &list->tail_) {
		throw std::runtime_error("DoublyList<V>::Iterator::operator--(Int32): Iterator not pointer any DoublyList node.");
	}

	Iterator tmp = *this;
	node_ = node_->prev;

	return tmp;
}

#endif
