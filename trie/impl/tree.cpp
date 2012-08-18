#pragma once
#include <vector>
#include <algorithm>
#include <utility>
#include <functional>
#include <cstddef>
#include "base.h"

template<class T, class K>
struct BinaryTree {
    typedef T* pointer;
    typedef T& reference;
    typedef BinaryTree link;
    pointer next;
    pointer sib[2];
    K key;

    pointer left()  const { return sib[0]; }
    pointer right() const { return sib[1]; }

    BinaryTree() : key(), next(), sib() { }

    pointer find_node(const char* str, size_t& ofs, bool opt=true) 
    {
	const char ch = str[ofs];
	pointer cur = next;
	while(cur) {
	    if(cur->key == ch)
		return key_traits<K>::match_key(cur->key, str, ofs)? cur : 0;
	    cur = cur->sib[ch > cur->key];
	}
	return 0;
    }

    // boilerplate class to avoid unncessary over-templatizing 
    struct args_base {
	virtual void operator()(T*&) const = 0;
	virtual operator T*()        const = 0;
    };
    struct args : args_base {
	operator T*() const 
	{ 
	    size_t ofs = args::ofs;
	    K key = key_traits<K>::extract_key(str, ofs);
	    T* tmp;
	    result = &T::create(tmp,str,ofs);
	    tmp->key = key;
	    return tmp; 
	}

	void operator()(T*& node) const 
	{
	    size_t ofs = args::ofs;
	    if(key_traits<K>::match_key(node->key, str, ofs))
		return result = &node->insert(str,ofs), (void)0;

//printf("%d>split '%s' @%d\n", (int)args::ofs, node->key.c_str(), int(ofs-args::ofs));
//printf("%d>extra '%s' @%d\n", (int)args::ofs, str, (int)ofs);
	    typename T::link link = *node;
	    result = key_traits<K>::split_key(node, link.key, ofs-args::ofs, str, ofs);
	    link.next = node->next;
	    node->T::link::operator=(link);
	}

	const char* const str;
	size_t const ofs;
	mutable T* result;

	args(const char* str, size_t ofs=0)
	: str(str), ofs(ofs) { }
    };

    struct args_ptr : args_base {
	operator T*() const        { return node; }
	void operator()(T*&) const { assert(!"logical error in *Tree<>::attach_node"); }

	args_ptr(T* const node) : node(node) { }
	T* const node;
    };

    void attach_node(K key, pointer p)
    {
	p->sib[0] = p->sib[1] = 0;
	p->key = key;
	T::insert_node(next, key, args_ptr(p));
    }

    static void insert_node(T*& node, char ch, const args_base& payload)
    {
	if(!node)
	    node = payload;
	else if(node->key == ch)
	    payload(node);
	else
	    insert_node(node->sib[ch > node->key], ch, payload);
    }

    reference select_node(const char* str, size_t ofs=0)
    {
	// type prefix to enable us to 'override' this method in derived classes
	args payload(str,ofs);
	T::insert_node(next, str[ofs], payload);
	return *payload.result;
    }

    static void rotate(T*& root, bool dir)
    {
        T* tmp = root;
        root = tmp->sib[!dir];
        tmp->sib[!dir] = root->sib[dir];
        root->sib[dir] = tmp;
    }

    void reserve(size_t) const { }

    bool empty() const
    {
	return !next;
    }

    static size_t count_nodes(const BinaryTree* node)
    {
	if(!node) return 0;
	return 1 + count_nodes(node->sib[0]) + count_nodes(node->sib[1]);
    }

    size_t arity() const
    {
	return count_nodes(next);
    }

    std::pair<K,pointer> successor() const
    {
	return next&&!next->sib[0]&&!next->sib[1]? std::make_pair(next->key, next) : std::make_pair(K(),0);
    }

    /* utilities */
    T* this_T() 
    {
	return static_cast<T*>(this);
    }


    template<class F>
    static void explore_nodes(T* tree, F fun, bool in_order)
    {
	if(tree) {
	    if(!in_order) fun(tree->key, *tree);
	    explore_nodes<F>(tree->sib[0], fun, in_order);
	    if( in_order) fun(tree->key, *tree);
	    explore_nodes<F>(tree->sib[1], fun, in_order);
	}
    }

    template<class F>
    void explore(F fun, bool in_order)
    {
	explore_nodes<F>(next, fun, in_order);
    }

    template<class F>
    void explore(F fun)
    {   
	std::vector<T*> open;
	if(next) open.push_back(next);
	for(int i=0; i < open.size(); ++i) {
	    T* const tree = open[i];
	    fun(tree->key, *tree);
	    if(tree->sib[0]) open.push_back(tree->sib[0]);
	    if(tree->sib[1]) open.push_back(tree->sib[1]);
	}
    } 

    template<class F>
    struct walk_t {
	F            fun;
	size_t const ofs;
	walk_t(F fun, size_t ofs) : fun(fun), ofs(ofs) { }
	void operator()(K k, T& node) const
	{ node.walk<F>(fun, ofs); }
    };

    template<class F>
    void walk(F fun, const size_t ofs=0) 
    {
	if(fun(key,*this_T(),ofs)) 
	    explore(walk_t<F>(fun,ofs+1), false);
    }

    static size_t optimize(T*& p)
    {
	if(!p) return 0;

	size_t const w[2] = {
	    optimize(p->sib[0]),
	    optimize(p->sib[1])
	};
	size_t const n = optimize(p->next)+!!p->search_key;
	size_t diff = w[0]<w[1]? w[1]-w[0] : w[0]-w[1];

	if(diff > n)
	    rotate(p, w[0]>w[1]);

	return n+w[0]+w[1];
    }

    size_t optimize() 
    {
	return optimize(next);
    }
};

template<class T,class K>
struct RotateTree : BinaryTree<T,K> {
    typedef typename BinaryTree<T,K>::args_base args_base;
    typedef RotateTree link;

    using BinaryTree<T,K>::rotate;

    T* find_node(const char* str, size_t& ofs, bool opt=true)
    {
	if(T* cur = sift_up(BinaryTree<T,K>::next, str[ofs], opt)) {
	    return key_traits<K>::match_key(cur->key, str, ofs)? cur : 0;
	} else
	    return 0;
    }

    static T* sift_up(T*& p, char k, bool opt)
    {
	if(!p || k == p->key) return p;

        bool dir = k > p->key;
        if(!p->sib[dir]) return 0;

        if(T* q = sift_up(p->sib[dir], k, opt))
            return opt? rotate(p, !dir), p : q;
        else
            return 0;
    }

    static void insert_node(T*& node, char ch, const args_base& payload)
    {
	if(!node)
	    node = payload;
	else if(node->key == ch)
	    payload(node);
	else {
	    bool const dir = ch > node->key;
	    insert_node(node->sib[dir], ch, payload);
	    rotate(node, !dir);
	}
    }
};

template<class T,class K>
struct BinaryTreeOpt : BinaryTree<T,K> {
    typedef BinaryTreeOpt link;

    using BinaryTree<T,K>::rotate;
    using BinaryTree<T,K>::next;
    using BinaryTree<T,K>::sib;

    T*& seek_node(char k, bool opt=true)
    {
	if(!next || next->key == k) return next;

	T** g;
	T** cur;
	bool dir;

        for(cur = &next; *cur; cur = &(*cur)->sib[dir]) {
            const char key = (*cur)->key;
	    if(k == key) 
	  	return opt? rotate(*g, !dir), *g : *cur;
	    dir = k > key;
	    g = cur;
        }
        return *cur;
    }

    T* find_node(const char* str, size_t& ofs, bool opt=true)
    {
	if(T* node = seek_node(str[ofs], opt)) 
	    return key_traits<K>::match_key(node->key, str, ofs)? node : 0;
	else
	    return 0;
    }

    T& select_node(const char* str, size_t ofs=0)
    {
	typename BinaryTree<T,K>::args payload(str,ofs);
	T*& node = seek_node(str[ofs]);
	if(node)
	    payload(node);
	else 
	    node = payload;
	return payload.result;
    }
};

template<class T, class K>
struct AATree : BinaryTree<T,K> {
    unsigned char level;

    typedef typename BinaryTree<T,K>::args_base args_base;
    typedef AATree link;

    using BinaryTree<T,K>::rotate;

    enum { left = false, right = true };

    AATree() : level() { }

    void attach_node(K key, T* p)
    {
	p->level = 0;
	BinaryTree<T,K>::attach_node(key, p);
    }

    static void insert_node(T*& p, char val, const args_base& payload)
    {
	if(!p)
	    return void(p = payload);

        if(val < p->key) {
            insert_node(p->sib[left], val, payload);
            if(p->level == p->sib[left]->level)
                rotate(p,right);
        } else if(val > p->key) {
            insert_node(p->sib[right], val, payload);
        } else
            return payload(p);

        if(p->sib[right] && p->sib[right]->sib[right] && p->level == p->sib[right]->sib[right]->level)
            rotate(p,left), ++p->level;
    }

private:
    void optimize();
};

template<class T, class K>
struct AVLTree : BinaryTree<T,K> {
    signed char bal;

    typedef typename BinaryTree<T,K>::args_base args_base;
    typedef AVLTree link;

    using BinaryTree<T,K>::rotate;

    AVLTree() : bal() { }

    void attach_node(K key, T* p)
    {
	p->bal = 0;
	BinaryTree<T,K>::attach_node(key, p);
    }

    static void insert_node(T*& p, char val, const args_base& payload)
    {
        if(!p) return void(p = payload);
	
	if(val == p->key) 
	    return payload(p);

	bool dir = val > p->key;
        int delta = 2*dir-1;
	int old_bal;

	if(!p->sib[dir])
	    p->bal += delta;
	else
	    old_bal = p->sib[dir]->bal;

        insert_node(p->sib[dir], val, payload);

	if(p->sib[dir]->bal != 0 && old_bal == 0)
	    p->bal += delta;

        if(p->bal == 2*delta) {
            if(p->sib[dir]->bal == delta) {
                p->bal = 0;
                rotate(p, !dir);
            } else {
                rotate(p->sib[dir], dir);
                rotate(p, !dir);
                p->sib[!dir]->bal = -(p->bal+delta)/2;
                p->sib[ dir]->bal = -(p->bal-delta)/2;
            }
            p->bal = 0;
        }
    }

private:
    void optimize();
};

template<class T, class K>
struct RBTree : BinaryTree<T,K> {
    enum { red, black } color;

    typedef typename BinaryTree<T,K>::args_base args_base;
    typedef RBTree link;

    using BinaryTree<T,K>::rotate;

    RBTree() : color(red) { }

    void attach_node(K key, T* p)
    {
	p->color = red;
	BinaryTree<T,K>::attach_node(key, p);
    }

    static void insert_node(T*& p, char val, const args_base& payload)
    {
	T* ignore = 0;
	assert(!p || p->color == black);
	insert_node(p, val, ignore, 0, payload);
	p->color = black;
    }

    static void insert_node(T*& p, char val, T*& g, bool uncle, const args_base& payload)
    {
        if(!p) return void(p = payload);
	
	if(val == p->key)
	    return payload(p);

        bool dir = val > p->key;

        insert_node(p->sib[dir], val, p, !dir, payload);

        if(p->color == black || p->sib[dir]->color == black)
            return;

	assert(g);
        if(g->sib[uncle] && g->sib[uncle]->color == red) {
            g->color = red;
            p->color = g->sib[uncle]->color = black;
            return;
        }

        if(uncle == dir) rotate(p, !dir);
        p->color = black;
        g->color = red;
        rotate(g, uncle);
    }

private:
    void optimize();
};
