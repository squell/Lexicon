#include "trie/basis.cpp"

 // some common names for different tries

template<class Link, class Data>
struct using_impl {
    typedef simple_trie<Data,Link, char>           simple_trie;
    typedef simple_trie<Data,Link, char_ptr>       patricia_trie;
    typedef simple_trie<Data,Link, char_store<8> > fast_patricia_trie;

    typedef trie<Data,Link,char> compact_trie;
};
