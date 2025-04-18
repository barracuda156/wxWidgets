/////////////////////////////////////////////////////////////////////////////
// Name:        wx/hashmap.h
// Purpose:     wxHashMap class
// Author:      Mattia Barbon
// Created:     29/01/2002
// Copyright:   (c) Mattia Barbon
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_HASHMAP_H_
#define _WX_HASHMAP_H_

#include "wx/string.h"
#include "wx/wxcrt.h"

// wxUSE_STD_CONTAINERS can't be used with gcc 4.8 due to a bug in its standard
// library (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=56278) which was fixed
// a very long time ago but is still present in the latest available versions
// of this compiler in Ubuntu and RHEL, so don't use the standard class with it.
#if !wxUSE_STD_CONTAINERS || defined(wxGCC_4_8)
    #define wxNEEDS_WX_HASH_MAP
#endif

#ifndef wxNEEDS_WX_HASH_MAP

#include <unordered_map>

// Use inheritance instead of simple typedef to allow existing forward
// declarations of the hash map classes in the applications using wx to work.
#define _WX_DECLARE_HASH_MAP( KEY_T, VALUE_T, HASH_T, KEY_EQ_T, CLASSNAME, CLASSEXP ) \
    class CLASSNAME : public std::unordered_map< KEY_T, VALUE_T, HASH_T, KEY_EQ_T > \
    { \
    public: \
        using std::unordered_map< KEY_T, VALUE_T, HASH_T, KEY_EQ_T >::unordered_map; \
    }

#else // wxNEEDS_WX_HASH_MAP

#include <stddef.h>             // for ptrdiff_t

// private
struct WXDLLIMPEXP_BASE _wxHashTable_NodeBase
{
    _wxHashTable_NodeBase() : m_next(nullptr) {}

    _wxHashTable_NodeBase* m_next;

// Cannot do this:
//  wxDECLARE_NO_COPY_CLASS(_wxHashTable_NodeBase);
// without rewriting the macros, which require a public copy constructor.
};

// private
class WXDLLIMPEXP_BASE _wxHashTableBase2
{
public:
    typedef void (*NodeDtor)(_wxHashTable_NodeBase*);
    typedef size_t (*BucketFromNode)(_wxHashTableBase2*,_wxHashTable_NodeBase*);
    typedef _wxHashTable_NodeBase* (*ProcessNode)(_wxHashTable_NodeBase*);
protected:
    static _wxHashTable_NodeBase* DummyProcessNode(_wxHashTable_NodeBase* node);
    static void DeleteNodes( size_t buckets, _wxHashTable_NodeBase** table,
                             NodeDtor dtor );
    static _wxHashTable_NodeBase* GetFirstNode( size_t buckets,
                                                _wxHashTable_NodeBase** table )
    {
        for( size_t i = 0; i < buckets; ++i )
            if( table[i] )
                return table[i];
        return nullptr;
    }

    // as static const unsigned prime_count = 31 but works with all compilers
    enum { prime_count = 31 };
    static const unsigned long ms_primes[prime_count];

    // returns the first prime in ms_primes greater than n
    static unsigned long GetNextPrime( unsigned long n );

    // returns the first prime in ms_primes smaller than n
    // ( or ms_primes[0] if n is very small )
    static unsigned long GetPreviousPrime( unsigned long n );

    static void CopyHashTable( _wxHashTable_NodeBase** srcTable,
                               size_t srcBuckets, _wxHashTableBase2* dst,
                               _wxHashTable_NodeBase** dstTable,
                               BucketFromNode func, ProcessNode proc );

    static void** AllocTable( size_t sz )
    {
        return (void **)calloc(sz, sizeof(void*));
    }
    static void FreeTable(void *table)
    {
        free(table);
    }
};

#define _WX_DECLARE_HASHTABLE( VALUE_T, KEY_T, HASH_T, KEY_EX_T, KEY_EQ_T,\
                               PTROPERATOR, CLASSNAME, CLASSEXP, \
                               SHOULD_GROW, SHOULD_SHRINK ) \
CLASSEXP CLASSNAME : protected _wxHashTableBase2 \
{ \
public: \
    typedef KEY_T key_type; \
    typedef VALUE_T value_type; \
    typedef HASH_T hasher; \
    typedef KEY_EQ_T key_equal; \
 \
    typedef size_t size_type; \
    typedef ptrdiff_t difference_type; \
    typedef value_type* pointer; \
    typedef const value_type* const_pointer; \
    typedef value_type& reference; \
    typedef const value_type& const_reference; \
    /* should these be protected? */ \
    typedef const KEY_T const_key_type; \
    typedef const VALUE_T const_mapped_type; \
public: \
    typedef KEY_EX_T key_extractor; \
    typedef CLASSNAME Self; \
protected: \
    _wxHashTable_NodeBase** m_table; \
    size_t m_tableBuckets; \
    size_t m_items; \
    hasher m_hasher; \
    key_equal m_equals; \
    key_extractor m_getKey; \
public: \
    struct Node:public _wxHashTable_NodeBase \
    { \
    public: \
        Node( const value_type& value ) \
            : m_value( value ) {} \
        Node* next() { return static_cast<Node*>(m_next); } \
 \
        value_type m_value; \
    }; \
 \
protected: \
    static void DeleteNode( _wxHashTable_NodeBase* node ) \
    { \
        delete static_cast<Node*>(node); \
    } \
public: \
    /*                  */ \
    /* forward iterator */ \
    /*                  */ \
    CLASSEXP Iterator \
    { \
    public: \
        Node* m_node; \
        Self* m_ht; \
 \
        Iterator() : m_node(nullptr), m_ht(nullptr) {} \
        Iterator( Node* node, const Self* ht ) \
            : m_node(node), m_ht(const_cast<Self*>(ht)) {} \
        bool operator ==( const Iterator& it ) const \
            { return m_node == it.m_node; } \
        bool operator !=( const Iterator& it ) const \
            { return m_node != it.m_node; } \
    protected: \
        Node* GetNextNode() \
        { \
            size_type bucket = GetBucketForNode(m_ht,m_node); \
            for( size_type i = bucket + 1; i < m_ht->m_tableBuckets; ++i ) \
            { \
                if( m_ht->m_table[i] ) \
                    return static_cast<Node*>(m_ht->m_table[i]); \
            } \
            return nullptr; \
        } \
 \
        void PlusPlus() \
        { \
            Node* next = m_node->next(); \
            m_node = next ? next : GetNextNode(); \
        } \
    }; \
    friend class Iterator; \
 \
public: \
    CLASSEXP iterator : public Iterator \
    { \
    public: \
        iterator() : Iterator() {} \
        iterator( Node* node, Self* ht ) : Iterator( node, ht ) {} \
        iterator& operator++() { PlusPlus(); return *this; } \
        iterator operator++(int) { iterator it=*this;PlusPlus();return it; } \
        reference operator *() const { return m_node->m_value; } \
        PTROPERATOR(pointer) \
    }; \
 \
    CLASSEXP const_iterator : public Iterator \
    { \
    public: \
        const_iterator() : Iterator() {} \
        const_iterator(iterator i) : Iterator(i) {} \
        const_iterator( Node* node, const Self* ht ) \
            : Iterator(node, const_cast<Self*>(ht)) {} \
        const_iterator& operator++() { PlusPlus();return *this; } \
        const_iterator operator++(int) { const_iterator it=*this;PlusPlus();return it; } \
        const_reference operator *() const { return m_node->m_value; } \
        PTROPERATOR(const_pointer) \
    }; \
 \
    CLASSNAME( size_type sz = 10, const hasher& hfun = hasher(), \
               const key_equal& k_eq = key_equal(), \
               const key_extractor& k_ex = key_extractor() ) \
        : m_tableBuckets( GetNextPrime( (unsigned long) sz ) ), \
          m_items( 0 ), \
          m_hasher( hfun ), \
          m_equals( k_eq ), \
          m_getKey( k_ex ) \
    { \
        m_table = (_wxHashTable_NodeBase**)AllocTable(m_tableBuckets); \
    } \
 \
    CLASSNAME( const Self& ht ) \
        : m_table(nullptr), \
          m_tableBuckets( 0 ), \
          m_items( ht.m_items ), \
          m_hasher( ht.m_hasher ), \
          m_equals( ht.m_equals ), \
          m_getKey( ht.m_getKey ) \
    { \
        HashCopy( ht ); \
    } \
 \
    const Self& operator=( const Self& ht ) \
    { \
         if (&ht != this) \
         { \
             clear(); \
             m_hasher = ht.m_hasher; \
             m_equals = ht.m_equals; \
             m_getKey = ht.m_getKey; \
             m_items = ht.m_items; \
             HashCopy( ht ); \
         } \
         return *this; \
    } \
 \
    ~CLASSNAME() \
    { \
        clear(); \
 \
        FreeTable(m_table); \
    } \
 \
    hasher hash_funct() { return m_hasher; } \
    key_equal key_eq() { return m_equals; } \
 \
    /* removes all elements from the hash table, but does not */ \
    /* shrink it ( perhaps it should ) */ \
    void clear() \
    { \
        DeleteNodes(m_tableBuckets, m_table, DeleteNode); \
        m_items = 0; \
    } \
 \
    size_type size() const { return m_items; } \
    size_type max_size() const { return size_type(-1); } \
    bool empty() const { return size() == 0; } \
 \
    const_iterator end() const { return const_iterator(nullptr, this); } \
    iterator end() { return iterator(nullptr, this); } \
    const_iterator begin() const \
        { return const_iterator(static_cast<Node*>(GetFirstNode(m_tableBuckets, m_table)), this); } \
    iterator begin() \
        { return iterator(static_cast<Node*>(GetFirstNode(m_tableBuckets, m_table)), this); } \
 \
    size_type erase( const const_key_type& key ) \
    { \
        _wxHashTable_NodeBase** node = GetNodePtr(key); \
 \
        if( !node ) \
            return 0; \
 \
        --m_items; \
        _wxHashTable_NodeBase* temp = (*node)->m_next; \
        delete static_cast<Node*>(*node); \
        (*node) = temp; \
        if( SHOULD_SHRINK( m_tableBuckets, m_items ) ) \
            ResizeTable( GetPreviousPrime( (unsigned long) m_tableBuckets ) - 1 ); \
        return 1; \
    } \
 \
protected: \
    static size_type GetBucketForNode( Self* ht, Node* node ) \
    { \
        return ht->m_hasher( ht->m_getKey( node->m_value ) ) \
            % ht->m_tableBuckets; \
    } \
    static Node* CopyNode( Node* node ) { return new Node( *node ); } \
 \
    Node* GetOrCreateNode( const value_type& value, bool& created ) \
    { \
        const const_key_type& key = m_getKey( value ); \
        size_t bucket = m_hasher( key ) % m_tableBuckets; \
        Node* node = static_cast<Node*>(m_table[bucket]); \
 \
        while( node ) \
        { \
            if( m_equals( m_getKey( node->m_value ), key ) ) \
            { \
                created = false; \
                return node; \
            } \
            node = node->next(); \
        } \
        created = true; \
        return CreateNode( value, bucket); \
    }\
    Node * CreateNode( const value_type& value, size_t bucket ) \
    {\
        Node* node = new Node( value ); \
        node->m_next = m_table[bucket]; \
        m_table[bucket] = node; \
 \
        /* must be after the node is inserted */ \
        ++m_items; \
        if( SHOULD_GROW( m_tableBuckets, m_items ) ) \
            ResizeTable( m_tableBuckets ); \
 \
        return node; \
    } \
    void CreateNode( const value_type& value ) \
    {\
        CreateNode(value, m_hasher( m_getKey(value) ) % m_tableBuckets ); \
    }\
 \
    /* returns nullptr if not found */ \
    _wxHashTable_NodeBase** GetNodePtr(const const_key_type& key) const \
    { \
        size_t bucket = m_hasher( key ) % m_tableBuckets; \
        _wxHashTable_NodeBase** node = &m_table[bucket]; \
 \
        while( *node ) \
        { \
            if (m_equals(m_getKey(static_cast<Node*>(*node)->m_value), key)) \
                return node; \
            node = &(*node)->m_next; \
        } \
 \
        return nullptr; \
    } \
 \
    /* returns nullptr if not found */ \
    /* expressing it in terms of GetNodePtr is 5-8% slower :-( */ \
    Node* GetNode( const const_key_type& key ) const \
    { \
        size_t bucket = m_hasher( key ) % m_tableBuckets; \
        Node* node = static_cast<Node*>(m_table[bucket]); \
 \
        while( node ) \
        { \
            if( m_equals( m_getKey( node->m_value ), key ) ) \
                return node; \
            node = node->next(); \
        } \
 \
        return nullptr; \
    } \
 \
    void ResizeTable( size_t newSize ) \
    { \
        newSize = GetNextPrime( (unsigned long)newSize ); \
        _wxHashTable_NodeBase** srcTable = m_table; \
        size_t srcBuckets = m_tableBuckets; \
        m_table = (_wxHashTable_NodeBase**)AllocTable( newSize ); \
        m_tableBuckets = newSize; \
 \
        CopyHashTable( srcTable, srcBuckets, \
                       this, m_table, \
                       (BucketFromNode)GetBucketForNode,\
                       (ProcessNode)&DummyProcessNode ); \
        FreeTable(srcTable); \
    } \
 \
    /* this must be called _after_ m_table has been cleaned */ \
    void HashCopy( const Self& ht ) \
    { \
        ResizeTable( ht.size() ); \
        CopyHashTable( ht.m_table, ht.m_tableBuckets, \
                       (_wxHashTableBase2*)this, \
                       m_table, \
                       (BucketFromNode)GetBucketForNode, \
                       (ProcessNode)CopyNode ); \
    } \
};

// defines an STL-like pair class CLASSNAME storing two fields: first of type
// KEY_T and second of type VALUE_T
#define _WX_DECLARE_PAIR( KEY_T, VALUE_T, CLASSNAME, CLASSEXP ) \
CLASSEXP CLASSNAME \
{ \
public: \
    typedef KEY_T first_type; \
    typedef VALUE_T second_type; \
    typedef KEY_T t1; \
    typedef VALUE_T t2; \
    typedef const KEY_T const_t1; \
    typedef const VALUE_T const_t2; \
 \
    CLASSNAME(const const_t1& f, const const_t2& s) \
        : first(const_cast<t1&>(f)), second(const_cast<t2&>(s)) {} \
 \
    t1 first; \
    t2 second; \
};

// defines the class CLASSNAME returning the key part (of type KEY_T) from a
// pair of type PAIR_T
#define _WX_DECLARE_HASH_MAP_KEY_EX( KEY_T, PAIR_T, CLASSNAME, CLASSEXP ) \
CLASSEXP CLASSNAME \
{ \
    typedef KEY_T key_type; \
    typedef PAIR_T pair_type; \
    typedef const key_type const_key_type; \
    typedef const pair_type const_pair_type; \
    typedef const_key_type& const_key_reference; \
    typedef const_pair_type& const_pair_reference; \
public: \
    CLASSNAME() = default; \
    const_key_reference operator()( const_pair_reference pair ) const { return pair.first; }\
};

// grow/shrink predicates
inline bool never_grow( size_t, size_t ) { return false; }
inline bool never_shrink( size_t, size_t ) { return false; }
inline bool grow_lf70( size_t buckets, size_t items )
{
    return float(items)/float(buckets) >= 0.85f;
}

#endif // various hash map implementations

// ----------------------------------------------------------------------------
// hashing and comparison functors
// ----------------------------------------------------------------------------

#ifndef wxNEEDS_WX_HASH_MAP

// integer types
struct WXDLLIMPEXP_BASE wxIntegerHash
{
private:
    std::hash<long> longHash;
    std::hash<unsigned long> ulongHash;
    std::hash<int> intHash;
    std::hash<unsigned int> uintHash;
    std::hash<short> shortHash;
    std::hash<unsigned short> ushortHash;

#ifdef wxHAS_LONG_LONG_T_DIFFERENT_FROM_LONG
    // hash<wxLongLong_t> ought to work but doesn't on some compilers
    #if (!defined SIZEOF_LONG_LONG && SIZEOF_LONG == 4) \
        || (defined SIZEOF_LONG_LONG && SIZEOF_LONG_LONG == SIZEOF_LONG * 2)
    size_t longlongHash( wxLongLong_t x ) const
    {
        return longHash( wx_truncate_cast(long, x) ) ^
               longHash( wx_truncate_cast(long, x >> (sizeof(long) * 8)) );
    }
    #elif defined SIZEOF_LONG_LONG && SIZEOF_LONG_LONG == SIZEOF_LONG
    std::hash<long> longlongHash;
    #else
    std::hash<wxLongLong_t> longlongHash;
    #endif
#endif // wxHAS_LONG_LONG_T_DIFFERENT_FROM_LONG

public:
    wxIntegerHash() noexcept = default;
    size_t operator()( long x ) const noexcept { return longHash( x ); }
    size_t operator()( unsigned long x ) const noexcept { return ulongHash( x ); }
    size_t operator()( int x ) const noexcept { return intHash( x ); }
    size_t operator()( unsigned int x ) const noexcept { return uintHash( x ); }
    size_t operator()( short x ) const noexcept { return shortHash( x ); }
    size_t operator()( unsigned short x ) const noexcept { return ushortHash( x ); }
#ifdef wxHAS_LONG_LONG_T_DIFFERENT_FROM_LONG
    size_t operator()( wxLongLong_t x ) const noexcept { return longlongHash(x); }
    size_t operator()( wxULongLong_t x ) const noexcept { return longlongHash(x); }
#endif // wxHAS_LONG_LONG_T_DIFFERENT_FROM_LONG
};

#else // wxNEEDS_WX_HASH_MAP

// integer types
struct WXDLLIMPEXP_BASE wxIntegerHash
{
    wxIntegerHash() noexcept = default;
    unsigned long operator()( long x ) const noexcept { return (unsigned long)x; }
    unsigned long operator()( unsigned long x ) const noexcept { return x; }
    unsigned long operator()( int x ) const noexcept { return (unsigned long)x; }
    unsigned long operator()( unsigned int x ) const noexcept { return x; }
    unsigned long operator()( short x ) const noexcept { return (unsigned long)x; }
    unsigned long operator()( unsigned short x ) const noexcept { return x; }
#ifdef wxHAS_LONG_LONG_T_DIFFERENT_FROM_LONG
    wxULongLong_t operator()( wxLongLong_t x ) const noexcept { return static_cast<wxULongLong_t>(x); }
    wxULongLong_t operator()( wxULongLong_t x ) const noexcept { return x; }
#endif // wxHAS_LONG_LONG_T_DIFFERENT_FROM_LONG
};

#endif // !wxNEEDS_WX_HASH_MAP/wxNEEDS_WX_HASH_MAP

struct WXDLLIMPEXP_BASE wxIntegerEqual
{
    wxIntegerEqual() noexcept = default;
    bool operator()( long a, long b ) const noexcept { return a == b; }
    bool operator()( unsigned long a, unsigned long b ) const noexcept { return a == b; }
    bool operator()( int a, int b ) const noexcept { return a == b; }
    bool operator()( unsigned int a, unsigned int b ) const noexcept { return a == b; }
    bool operator()( short a, short b ) const noexcept { return a == b; }
    bool operator()( unsigned short a, unsigned short b ) const noexcept { return a == b; }
#ifdef wxHAS_LONG_LONG_T_DIFFERENT_FROM_LONG
    bool operator()( wxLongLong_t a, wxLongLong_t b ) const noexcept { return a == b; }
    bool operator()( wxULongLong_t a, wxULongLong_t b ) const noexcept { return a == b; }
#endif // wxHAS_LONG_LONG_T_DIFFERENT_FROM_LONG
};

// pointers
struct WXDLLIMPEXP_BASE wxPointerHash
{
    wxPointerHash() noexcept = default;

#ifdef wxNEEDS_WX_HASH_MAP
    wxUIntPtr operator()( const void* k ) const noexcept { return wxPtrToUInt(k); }
#else
    size_t operator()( const void* k ) const noexcept { return (size_t)k; }
#endif
};

struct WXDLLIMPEXP_BASE wxPointerEqual
{
    wxPointerEqual() noexcept = default;
    bool operator()( const void* a, const void* b ) const noexcept { return a == b; }
};

// wxString, char*, wchar_t*
struct WXDLLIMPEXP_BASE wxStringHash
{
    wxStringHash() noexcept = default;
    unsigned long operator()( const wxString& x ) const noexcept
        { return stringHash( x.wx_str() ); }
    unsigned long operator()( const wchar_t* x ) const noexcept
        { return stringHash( x ); }
    unsigned long operator()( const char* x ) const noexcept
        { return stringHash( x ); }

    static unsigned long stringHash( const wchar_t* );
    static unsigned long stringHash( const char* );
};

struct WXDLLIMPEXP_BASE wxStringEqual
{
    wxStringEqual() noexcept = default;
    bool operator()( const wxString& a, const wxString& b ) const noexcept
        { return a == b; }
    bool operator()( const wxChar* a, const wxChar* b ) const noexcept
        { return wxStrcmp( a, b ) == 0; }
    bool operator()( const char* a, const char* b ) const noexcept
        { return strcmp( a, b ) == 0; }
};

#ifdef wxNEEDS_WX_HASH_MAP

#define wxPTROP_NORMAL(pointer) \
    pointer operator ->() const { return &(m_node->m_value); }
#define wxPTROP_NOP(pointer)

#define _WX_DECLARE_HASH_MAP( KEY_T, VALUE_T, HASH_T, KEY_EQ_T, CLASSNAME, CLASSEXP ) \
wxCLANG_WARNING_SUPPRESS(unused-member-function) \
_WX_DECLARE_PAIR( KEY_T, VALUE_T, CLASSNAME##_wxImplementation_Pair, CLASSEXP ) \
_WX_DECLARE_HASH_MAP_KEY_EX( KEY_T, CLASSNAME##_wxImplementation_Pair, CLASSNAME##_wxImplementation_KeyEx, CLASSEXP ) \
_WX_DECLARE_HASHTABLE( CLASSNAME##_wxImplementation_Pair, KEY_T, HASH_T, \
    CLASSNAME##_wxImplementation_KeyEx, KEY_EQ_T, wxPTROP_NORMAL, \
    CLASSNAME##_wxImplementation_HashTable, CLASSEXP, grow_lf70, never_shrink ) \
CLASSEXP CLASSNAME:public CLASSNAME##_wxImplementation_HashTable \
{ \
public: \
    typedef VALUE_T mapped_type; \
    _WX_DECLARE_PAIR( iterator, bool, Insert_Result, CLASSEXP ) \
 \
    explicit CLASSNAME( size_type hint = 100, hasher hf = hasher(),          \
                        key_equal eq = key_equal() )                         \
        : CLASSNAME##_wxImplementation_HashTable( hint, hf, eq,              \
                                   CLASSNAME##_wxImplementation_KeyEx() ) {} \
 \
    mapped_type& operator[]( const const_key_type& key ) \
    { \
        bool created; \
        return GetOrCreateNode( \
                CLASSNAME##_wxImplementation_Pair( key, mapped_type() ), \
                created)->m_value.second; \
    } \
 \
    const_iterator find( const const_key_type& key ) const \
    { \
        return const_iterator( GetNode( key ), this ); \
    } \
 \
    iterator find( const const_key_type& key ) \
    { \
        return iterator( GetNode( key ), this ); \
    } \
 \
    Insert_Result insert( const value_type& v ) \
    { \
        bool created; \
        Node *node = GetOrCreateNode( \
                CLASSNAME##_wxImplementation_Pair( v.first, v.second ), \
                created); \
        return Insert_Result(iterator(node, this), created); \
    } \
 \
    size_type erase( const key_type& k ) \
        { return CLASSNAME##_wxImplementation_HashTable::erase( k ); } \
    void erase( const iterator& it ) { erase( (*it).first ); } \
 \
    /* count() == 0 | 1 */ \
    size_type count( const const_key_type& key ) \
    { \
        return GetNode( key ) ? 1u : 0u; \
    } \
} \
wxCLANG_WARNING_RESTORE(unused-member-function)

#endif // wxNEEDS_WX_HASH_MAP

// these macros are to be used in the user code
#define WX_DECLARE_HASH_MAP( KEY_T, VALUE_T, HASH_T, KEY_EQ_T, CLASSNAME) \
    _WX_DECLARE_HASH_MAP( KEY_T, VALUE_T, HASH_T, KEY_EQ_T, CLASSNAME, class )

#define WX_DECLARE_STRING_HASH_MAP( VALUE_T, CLASSNAME ) \
    _WX_DECLARE_HASH_MAP( wxString, VALUE_T, wxStringHash, wxStringEqual, \
                          CLASSNAME, class )

#define WX_DECLARE_VOIDPTR_HASH_MAP( VALUE_T, CLASSNAME ) \
    _WX_DECLARE_HASH_MAP( void*, VALUE_T, wxPointerHash, wxPointerEqual, \
                          CLASSNAME, class )

// and these do exactly the same thing but should be used inside the
// library
// note: DECL is not used since the class is inline
#define WX_DECLARE_HASH_MAP_WITH_DECL( KEY_T, VALUE_T, HASH_T, KEY_EQ_T, CLASSNAME, DECL) \
    _WX_DECLARE_HASH_MAP( KEY_T, VALUE_T, HASH_T, KEY_EQ_T, CLASSNAME, class )

#define WX_DECLARE_EXPORTED_HASH_MAP( KEY_T, VALUE_T, HASH_T, KEY_EQ_T, CLASSNAME) \
    WX_DECLARE_HASH_MAP_WITH_DECL( KEY_T, VALUE_T, HASH_T, KEY_EQ_T, \
                                   CLASSNAME, class WXDLLIMPEXP_CORE )

// note: DECL is not used since the class is inline
#define WX_DECLARE_STRING_HASH_MAP_WITH_DECL( VALUE_T, CLASSNAME, DECL ) \
    _WX_DECLARE_HASH_MAP( wxString, VALUE_T, wxStringHash, wxStringEqual, \
                          CLASSNAME, class )

#define WX_DECLARE_EXPORTED_STRING_HASH_MAP( VALUE_T, CLASSNAME ) \
    WX_DECLARE_STRING_HASH_MAP_WITH_DECL( VALUE_T, CLASSNAME, \
                                          class WXDLLIMPEXP_CORE )

// note: DECL is not used since the class is inline
#define WX_DECLARE_VOIDPTR_HASH_MAP_WITH_DECL( VALUE_T, CLASSNAME, DECL ) \
    _WX_DECLARE_HASH_MAP( void*, VALUE_T, wxPointerHash, wxPointerEqual, \
                          CLASSNAME, class )

#define WX_DECLARE_EXPORTED_VOIDPTR_HASH_MAP( VALUE_T, CLASSNAME ) \
    WX_DECLARE_VOIDPTR_HASH_MAP_WITH_DECL( VALUE_T, CLASSNAME, \
                                           class WXDLLIMPEXP_CORE )

// delete all hash elements
//
// NB: the class declaration of the hash elements must be visible from the
//     place where you use this macro, otherwise the proper destructor may not
//     be called (a decent compiler should give a warning about it, but don't
//     count on it)!
#define WX_CLEAR_HASH_MAP(type, hashmap)                                     \
    {                                                                        \
        type::iterator it, en;                                               \
        for( it = (hashmap).begin(), en = (hashmap).end(); it != en; ++it )  \
            delete it->second;                                               \
        (hashmap).clear();                                                   \
    }

//---------------------------------------------------------------------------
// Declarations of common hashmap classes

WX_DECLARE_HASH_MAP_WITH_DECL( long, long, wxIntegerHash, wxIntegerEqual,
                               wxLongToLongHashMap, class WXDLLIMPEXP_BASE );

WX_DECLARE_STRING_HASH_MAP_WITH_DECL( wxString, wxStringToStringHashMap,
                                      class WXDLLIMPEXP_BASE );

WX_DECLARE_STRING_HASH_MAP_WITH_DECL( wxUIntPtr, wxStringToNumHashMap,
                                      class WXDLLIMPEXP_BASE );


#endif // _WX_HASHMAP_H_
