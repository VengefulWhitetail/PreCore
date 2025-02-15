#ifndef BPSTD_DETAIL_STRING_VIEW_INL
#define BPSTD_DETAIL_STRING_VIEW_INL

namespace es {

  //--------------------------------------------------------------------------
  // Constructor
  //--------------------------------------------------------------------------

  template<typename CharT, typename Traits>
  inline constexpr basic_string_view<CharT,Traits>::basic_string_view()
    noexcept
    : m_str(nullptr),
      m_size(0)
  {

  }

  template<typename CharT, typename Traits>
  template<typename Allocator>
  inline constexpr basic_string_view<CharT,Traits>::basic_string_view( const std::basic_string<CharT,Traits,Allocator>& str )
    noexcept
    : m_str(str.c_str()),
      m_size(str.size())
  {

  }

  template<typename CharT, typename Traits>
  inline constexpr basic_string_view<CharT,Traits>::basic_string_view( const char_type* str )
    noexcept
    : m_str(str),
      m_size(traits_type::length(str))
  {

  }

  template<typename CharT, typename Traits>
  inline constexpr basic_string_view<CharT,Traits>::basic_string_view( const char_type* str, size_type count )
    noexcept
    : m_str(str),
      m_size(count)
  {

  }

  template<typename CharT, typename Traits>
  inline constexpr basic_string_view<CharT,Traits>::basic_string_view( const char_type* start, const char_type* end )
    noexcept
    : m_str(start),
      m_size(std::distance(start, end))
  {

  }

  //--------------------------------------------------------------------------
  // Capacity
  //--------------------------------------------------------------------------

  template<typename CharT, typename Traits>
  inline constexpr typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::size()
    const noexcept
  {
    return m_size;
  }

  template<typename CharT, typename Traits>
  inline constexpr typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::length()
    const noexcept
  {
    return size();
  }

  template<typename CharT, typename Traits>
  inline constexpr typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::max_size()
    const noexcept
  {
    return npos - 1;
  }

  template<typename CharT, typename Traits>
  inline constexpr bool basic_string_view<CharT,Traits>::empty()
    const noexcept
  {
    return m_size == 0;
  }

  //--------------------------------------------------------------------------
  // Element Access
  //--------------------------------------------------------------------------

  template<typename CharT, typename Traits>
  inline constexpr const typename basic_string_view<CharT,Traits>::char_type*
    basic_string_view<CharT,Traits>::c_str()
    const noexcept
  {
    return m_str;
  }

  template<typename CharT, typename Traits>
  inline constexpr const typename basic_string_view<CharT,Traits>::char_type*
    basic_string_view<CharT,Traits>::data()
    const noexcept
  {
    return m_str;
  }

  template<typename CharT, typename Traits>
  inline constexpr typename basic_string_view<CharT,Traits>::const_reference
    basic_string_view<CharT,Traits>::operator[]( size_t pos )
    const noexcept
  {
    return m_str[pos];
  }

  template<typename CharT, typename Traits>
  inline constexpr typename basic_string_view<CharT,Traits>::const_reference
    basic_string_view<CharT,Traits>::at( size_t pos )
    const
  {
    return pos < m_size ? m_str[pos] : throw std::out_of_range("Input out of range in basic_string_view::at"), m_str[pos];
  }

  template<typename CharT, typename Traits>
  inline constexpr typename basic_string_view<CharT,Traits>::const_reference
    basic_string_view<CharT,Traits>::front( )
    const noexcept
  {
    return *m_str;
  }

  template<typename CharT, typename Traits>
  inline constexpr typename basic_string_view<CharT,Traits>::const_reference
    basic_string_view<CharT,Traits>::back( )
    const noexcept
  {
    return m_str[m_size-1];
  }

  //--------------------------------------------------------------------------
  // Modifiers
  //--------------------------------------------------------------------------

  template<typename CharT, typename Traits>
  inline void
    basic_string_view<CharT,Traits>::remove_prefix( size_type n )
    noexcept
  {
    m_str += n, m_size -= n;
  }

  template<typename CharT, typename Traits>
  inline void
    basic_string_view<CharT,Traits>::remove_suffix( size_type n )
    noexcept
  {
    m_size -= n;
  }

  template<typename CharT, typename Traits>
  inline void
    basic_string_view<CharT,Traits>::swap( basic_string_view& v )
    noexcept
  {
    using std::swap;
    swap(m_size,v.m_size);
    swap(m_str,v.m_str);
  }

  template<typename CharT, typename Traits>
  inline void
    basic_string_view<CharT,Traits>::resize( size_type n )
    noexcept
  {
    m_size = n;
  }


  //--------------------------------------------------------------------------
  // Conversions
  //--------------------------------------------------------------------------

  template<typename CharT, typename Traits>
  template<class Allocator>
  inline constexpr std::basic_string<CharT, Traits, Allocator>
    basic_string_view<CharT,Traits>::to_string( const Allocator& a )
    const
  {
    return std::basic_string<CharT,Traits,Allocator>( m_str, m_size, a );
  }

  template<typename CharT, typename Traits>
  template<class Allocator>
  inline constexpr basic_string_view<CharT,Traits>::operator
    std::basic_string<CharT, Traits, Allocator>()
    const
  {
    return std::basic_string<CharT,Traits,Allocator>( m_str, m_size );
  }

  //--------------------------------------------------------------------------
  // String Operations
  //--------------------------------------------------------------------------

  template<typename CharT, typename Traits>
  inline typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::copy( char_type* dest,
                                           size_type count,
                                           size_type pos )
    const
  {
    if(pos >= m_size) throw std::out_of_range("Index out of range in basic_string_view::copy");

    const size_type rcount = std::min(m_size - pos,count+1);
    std::copy( m_str + pos, m_str + pos + rcount, dest);
    return rcount;
  }

  template<typename CharT, typename Traits>
  inline basic_string_view<CharT,Traits>
    basic_string_view<CharT,Traits>::substr( size_t pos,
                                             size_t len )
    const
  {
    const size_type max_length = pos > m_size ? 0 : m_size - pos;

    return pos < m_size ? basic_string_view<CharT,Traits>( m_str + pos, len > max_length ? max_length : len ) : throw std::out_of_range("Index out of range in basic_string_view::substr");
  }

  //--------------------------------------------------------------------------

  template<typename CharT, typename Traits>
  inline int basic_string_view<CharT,Traits>::compare( basic_string_view v )
    const noexcept
  {
    const size_type rlen = std::min(m_size,v.m_size);
    const int compare = Traits::compare(m_str,v.m_str,rlen);

    return (compare ? compare : (m_size < v.m_size ? -1 : (m_size > v.m_size ? 1 : 0)));
  }

  template<typename CharT, typename Traits>
  inline int basic_string_view<CharT,Traits>::compare( size_type pos,
                                                       size_type count,
                                                       basic_string_view v )
    const
  {
    return substr(pos,count).compare(v);
  }

  template<typename CharT, typename Traits>
  inline int basic_string_view<CharT,Traits>::compare( size_type pos1,
                                                       size_type count1,
                                                       basic_string_view v,
                                                       size_type pos2,
                                                       size_type count2 )
    const
  {
    return substr(pos1,count1).compare( v.substr(pos2,count2) );
  }

  template<typename CharT, typename Traits>
  inline int basic_string_view<CharT,Traits>::compare( const char_type* s )
    const
  {
    return compare(basic_string_view<CharT,Traits>(s));
  }

  template<typename CharT, typename Traits>
  inline int basic_string_view<CharT,Traits>::compare( size_type pos,
                                                       size_type count,
                                                       const char_type* s )
    const
  {
    return substr(pos, count).compare( basic_string_view<CharT,Traits>(s) );
  }

  template<typename CharT, typename Traits>
  inline int basic_string_view<CharT,Traits>::compare( size_type pos,
                                                       size_type count1,
                                                       const char_type* s,
                                                       size_type count2 )
    const
  {
    return substr(pos, count1).compare( basic_string_view<CharT,Traits>(s, count2) );
  }

  //--------------------------------------------------------------------------

  template <typename CharT, typename Traits>
  inline typename basic_string_view<CharT, Traits>::size_type
  basic_string_view<CharT, Traits>::find(basic_string_view v,
                                         size_type pos) const {
    // Can't find a substring if the substring is bigger than this
    if (pos > size()) {
      return npos;
    }
    if ((pos + v.size()) > size()) {
      return npos;
    }

    const auto offset = pos;
    const auto increments = size() - v.size();

    for (auto i = offset; i <= increments; ++i) {
      if (substr(i, v.size()) == v) {
        return i;
      }
    }

    return npos;
  }

  template<typename CharT, typename Traits>
  inline typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::find( char_type c,
                                           size_type pos )
    const
  {
    return find(basic_string_view<CharT,Traits>(&c, 1), pos);
  }

  template<typename CharT, typename Traits>
  inline typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::find( const char_type* s, size_type pos,
                                           size_type count )
    const
  {
    return find(basic_string_view<CharT,Traits>(s, count), pos);
  }

  template<typename CharT, typename Traits>
  inline typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::find( const char_type* s,
                                          size_type pos )
    const
  {
    return find(basic_string_view<CharT,Traits>(s), pos);
  }

  //--------------------------------------------------------------------------

  template<typename CharT, typename Traits>
  inline typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::rfind( basic_string_view v,
                                            size_type pos )
    const
  {
    if (empty()) {
      return v.empty() ? 0u : npos;
    }
    if (v.empty()) {
      return std::min(size() - 1, pos);
    }
    if (v.size() > size()) {
      return npos;
    }

    auto i = std::min(pos, (size() - v.size()));
    while (i != npos) {
      if (substr(i, v.size()) == v) {
        return i;
      }
      --i;
    }
    return npos;
  }

  template<typename CharT, typename Traits>
  inline typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::rfind( char_type c,
                                           size_type pos )
    const
  {
    return rfind(basic_string_view<CharT,Traits>(&c, 1), pos);
  }

  template<typename CharT, typename Traits>
  inline typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::rfind( const char_type* s, size_type pos,
                                           size_type count )
    const
  {
    return rfind(basic_string_view<CharT,Traits>(s, count), pos);
  }

  template<typename CharT, typename Traits>
  inline typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::rfind( const char_type* s,
                                          size_type pos )
    const
  {
    return rfind(basic_string_view<CharT,Traits>(s), pos);
  }

  //--------------------------------------------------------------------------

  template <typename CharT, typename Traits>
  inline bool is_one_of(CharT c, basic_string_view<CharT, Traits> str)
  {
    for (auto s : str) {
      if (c == s) {
        return true;
      }
    }
    return false;
  }

  template<typename CharT, typename Traits>
  inline typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::find_first_of( basic_string_view v,
                                                    size_type pos )
    const
  {
    const auto max_index = size();
    for (auto i = pos; i < max_index;  ++i) {
      if (is_one_of(m_str[i],v)) {
        return i;
      }
    }
    return npos;
  }

  template<typename CharT, typename Traits>
  inline typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::find_first_of( char_type c,
                                                    size_type pos )
    const
  {
    return find_first_of(basic_string_view<CharT,Traits>(&c, 1), pos);
  }

  template<typename CharT, typename Traits>
  inline typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::find_first_of( const char_type* s, size_type pos,
                                                    size_type count )
    const
  {
    return find_first_of(basic_string_view<CharT,Traits>(s, count), pos);
  }

  template<typename CharT, typename Traits>
  inline typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::find_first_of( const char_type* s,
                                                    size_type pos )
    const
  {
    return find_first_of( basic_string_view<CharT,Traits>(s), pos );
  }

  //--------------------------------------------------------------------------

  template<typename CharT, typename Traits>
  inline typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::find_last_of( basic_string_view v,
                                                   size_type pos )
    const
  {
    if (empty()) {
      return npos;
    }
    const auto max_index = std::min(size() - 1, pos);
    for (auto i = 0u; i <= max_index;  ++i) {
      const auto j = max_index - i;

      if (is_one_of(m_str[j],v)) {
        return j;
      }
    }

    return npos;
  }

  template<typename CharT, typename Traits>
  inline typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::find_last_of( char_type c,
                                                   size_type pos )
    const
  {
    return find_last_of( basic_string_view<CharT,Traits>(&c, 1), pos );
  }

  template<typename CharT, typename Traits>
  inline typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::find_last_of( const char_type* s, size_type pos,
                                                   size_type count )
    const
  {
    return find_last_of( basic_string_view<CharT,Traits>(s, count), pos );
  }

  template<typename CharT, typename Traits>
  inline typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::find_last_of( const char_type* s,
                                                   size_type pos )
    const
  {
    return find_last_of( basic_string_view<CharT,Traits>(s), pos );
  }

  //--------------------------------------------------------------------------

  template<typename CharT, typename Traits>
  inline typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::find_first_not_of( basic_string_view v,
                                                        size_type pos )
    const
  {
    const auto max_index = size();
    for (auto i = pos; i < max_index;  ++i) {
      if (!is_one_of(m_str[i],v)) {
        return i;
      }
    }

    return npos;
  }

  template<typename CharT, typename Traits>
  inline typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::find_first_not_of( char_type c,
                                                        size_type pos )
    const
  {
    return find_first_not_of( basic_string_view<CharT,Traits>(&c, 1), pos );
  }

  template<typename CharT, typename Traits>
  inline typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::find_first_not_of( const char_type* s, size_type pos,
                                                        size_type count )
    const
  {
    return find_first_not_of( basic_string_view<CharT,Traits>(s, count), pos );
  }

  template<typename CharT, typename Traits>
  inline typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::find_first_not_of( const char_type* s,
                                                        size_type pos )
    const
  {
    return find_first_not_of( basic_string_view<CharT,Traits>(s), pos );
  }

  //--------------------------------------------------------------------------

  template<typename CharT, typename Traits>
  inline typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::find_last_not_of( basic_string_view v,
                                                       size_type pos )
    const
  {
    if (empty()) {
      return npos;
    }
    const auto max_index = std::min(size() - 1, pos);
    for (auto i = 0u; i <= max_index;  ++i) {
      const auto j = max_index - i;

      if (!is_one_of(m_str[j],v)) {
        return j;
      }
    }

    return npos;
  }

  template<typename CharT, typename Traits>
  inline typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::find_last_not_of( char_type c,
                                                       size_type pos )
    const
  {
    return find_last_not_of( basic_string_view<CharT,Traits>(&c, 1), pos );
  }

  template<typename CharT, typename Traits>
  inline typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::find_last_not_of( const char_type* s, size_type pos,
                                                       size_type count )
    const
  {
    return find_last_not_of( basic_string_view<CharT,Traits>(s, count), pos );
  }

  template<typename CharT, typename Traits>
  inline typename basic_string_view<CharT,Traits>::size_type
    basic_string_view<CharT,Traits>::find_last_not_of( const char_type* s,
                                                       size_type pos )
    const
  {
    return find_last_not_of( basic_string_view<CharT,Traits>(s), pos );
  }

  template<typename CharT, typename Traits>
  inline bool basic_string_view<CharT,Traits>::begins_with( basic_string_view v ) const
  {
    return find(v) == 0;
  }

  template<typename CharT, typename Traits>
  inline bool basic_string_view<CharT,Traits>::ends_with( basic_string_view v ) const
  {
    return size() >= v.size() && find(v) == size() - v.size();
  }

  //--------------------------------------------------------------------------
  // Iterator
  //--------------------------------------------------------------------------

  template<typename CharT, typename Traits>
  constexpr  typename basic_string_view<CharT,Traits>::const_iterator
    basic_string_view<CharT,Traits>::begin()
    const noexcept
  {
    return m_str;
  }

  template<typename CharT, typename Traits>
  constexpr  typename basic_string_view<CharT,Traits>::const_iterator
    basic_string_view<CharT,Traits>::end()
    const noexcept
  {
    return m_str + m_size;
  }

  template<typename CharT, typename Traits>
  constexpr  typename basic_string_view<CharT,Traits>::const_iterator
    basic_string_view<CharT,Traits>::cbegin()
    const noexcept
  {
    return m_str;
  }

  template<typename CharT, typename Traits>
  constexpr  typename basic_string_view<CharT,Traits>::const_iterator
    basic_string_view<CharT,Traits>::cend()
    const noexcept
  {
    return m_str + m_size;
  }

  //--------------------------------------------------------------------------
  // Public Functions
  //--------------------------------------------------------------------------

  template<typename CharT, typename Traits>
  std::basic_ostream<CharT,Traits>& operator << ( std::basic_ostream<CharT,Traits>& o,
                                                  const basic_string_view<CharT,Traits>& str )
  {
    o.write(str.data(),str.size());
    return o;
  }

  template<typename CharT, typename Traits>
  inline void swap( basic_string_view<CharT,Traits>& lhs,
                    basic_string_view<CharT,Traits>& rhs )
    noexcept
  {
    lhs.swap(rhs);
  }

  //--------------------------------------------------------------------------
  // Comparison Functions
  //--------------------------------------------------------------------------

  template<typename CharT, typename Traits>
  inline bool operator == ( const basic_string_view<CharT,Traits>& lhs,
                            const basic_string_view<CharT,Traits>& rhs )
    noexcept
  {
    return lhs.compare(rhs) == 0;
  }

  template<typename CharT, typename Traits>
  inline bool operator == ( basic_string_view<CharT,Traits> lhs,
                            const CharT* rhs )
    noexcept
  {
    return lhs == basic_string_view<CharT,Traits>(rhs);
  }

  template<typename CharT, typename Traits>
  inline bool operator == ( const CharT* lhs,
                            const basic_string_view<CharT,Traits>& rhs )
    noexcept
  {
    return basic_string_view<CharT,Traits>(lhs) == rhs;
  }

  template<typename CharT, typename Traits, typename Allocator>
  inline bool operator == ( const std::basic_string<CharT,Traits,Allocator>& lhs,
                            const basic_string_view<CharT,Traits>& rhs )
  {
    return basic_string_view<CharT,Traits>(lhs) == rhs;
  }

  template<typename CharT, typename Traits, typename Allocator>
  inline bool operator == ( const basic_string_view<CharT,Traits>& lhs,
                            const std::basic_string<CharT,Traits,Allocator>& rhs )
  {
    return lhs == basic_string_view<CharT,Traits>(rhs);
  }

  //--------------------------------------------------------------------------

  template<typename CharT, typename Traits>
  inline bool operator != ( const basic_string_view<CharT,Traits>& lhs,
                            const basic_string_view<CharT,Traits>& rhs )
    noexcept
  {
    return lhs.compare(rhs) != 0;
  }

  template<typename CharT, typename Traits>
  inline bool operator != ( const basic_string_view<CharT,Traits>& lhs,
                            const CharT* rhs )
    noexcept
  {
    return lhs != basic_string_view<CharT,Traits>(rhs);
  }

  template<typename CharT, typename Traits>
  inline bool operator != ( const CharT* lhs,
                            const basic_string_view<CharT,Traits>& rhs )
    noexcept
  {
    return basic_string_view<CharT,Traits>(lhs) != rhs;
  }

  template<typename CharT, typename Traits, typename Allocator>
  inline bool operator != ( const std::basic_string<CharT,Traits,Allocator>& lhs,
                            const basic_string_view<CharT,Traits>& rhs )
  {
    return basic_string_view<CharT,Traits>(lhs) != rhs;
  }

  template<typename CharT, typename Traits, typename Allocator>
  inline bool operator != ( const basic_string_view<CharT,Traits>& lhs,
                            const std::basic_string<CharT,Traits,Allocator>& rhs )
  {
    return lhs != basic_string_view<CharT,Traits>(rhs);
  }
  //--------------------------------------------------------------------------

  template<typename CharT, typename Traits>
  inline bool operator < ( const basic_string_view<CharT,Traits>& lhs,
                           const basic_string_view<CharT,Traits>& rhs )
    noexcept
  {
    return lhs.compare(rhs) < 0;
  }

  template<typename CharT, typename Traits>
  inline bool operator < ( const basic_string_view<CharT,Traits>& lhs,
                           const CharT* rhs )
    noexcept
  {
    return lhs < basic_string_view<CharT,Traits>(rhs);
  }

  template<typename CharT, typename Traits>
  inline bool operator < ( const CharT* lhs,
                           const basic_string_view<CharT,Traits>& rhs )
    noexcept
  {
    return basic_string_view<CharT,Traits>(lhs) < rhs;
  }

  template<typename CharT, typename Traits, typename Allocator>
  inline bool operator < ( const std::basic_string<CharT,Traits,Allocator>& lhs,
                           const basic_string_view<CharT,Traits>& rhs )
  {
    return basic_string_view<CharT,Traits>(lhs) < rhs;
  }

  template<typename CharT, typename Traits, typename Allocator>
  inline bool operator < ( const basic_string_view<CharT,Traits>& lhs,
                           const std::basic_string<CharT,Traits,Allocator>& rhs )
  {
    return lhs < basic_string_view<CharT,Traits>(rhs);
  }

  //--------------------------------------------------------------------------

  template<typename CharT, typename Traits>
  inline bool operator > ( const basic_string_view<CharT,Traits>& lhs,
                           const basic_string_view<CharT,Traits>& rhs )
    noexcept
  {
    return lhs.compare(rhs) > 0;
  }

  template<typename CharT, typename Traits>
  inline bool operator > ( const basic_string_view<CharT,Traits>& lhs,
                           const CharT* rhs )
    noexcept
  {
    return lhs > basic_string_view<CharT,Traits>(rhs);
  }

  template<typename CharT, typename Traits>
  inline bool operator > ( const CharT* lhs,
                           const basic_string_view<CharT,Traits>& rhs )
    noexcept
  {
    return basic_string_view<CharT,Traits>(lhs) > rhs;
  }

  template<typename CharT, typename Traits, typename Allocator>
  inline bool operator > ( const std::basic_string<CharT,Traits,Allocator>& lhs,
                           const basic_string_view<CharT,Traits>& rhs )
  {
    return basic_string_view<CharT,Traits>(lhs) > rhs;
  }

  template<typename CharT, typename Traits, typename Allocator>
  inline bool operator > ( const basic_string_view<CharT,Traits>& lhs,
                           const std::basic_string<CharT,Traits,Allocator>& rhs )
  {
    return lhs > basic_string_view<CharT,Traits>(rhs);
  }

  //--------------------------------------------------------------------------

  template<typename CharT, typename Traits>
  inline bool operator <= ( const basic_string_view<CharT,Traits>& lhs,
                            const basic_string_view<CharT,Traits>& rhs )
    noexcept
  {
    return lhs.compare(rhs) <= 0;
  }

  template<typename CharT, typename Traits>
  inline bool operator <= ( const basic_string_view<CharT,Traits>& lhs,
                            const CharT* rhs )
    noexcept
  {
    return lhs <= basic_string_view<CharT,Traits>(rhs);
  }

  template<typename CharT, typename Traits>
  inline bool operator <= ( const CharT* lhs,
                            const basic_string_view<CharT,Traits>& rhs )
    noexcept
  {
    return basic_string_view<CharT,Traits>(lhs) <= rhs;
  }

  template<typename CharT, typename Traits, typename Allocator>
  inline bool operator <= ( const std::basic_string<CharT,Traits,Allocator>& lhs,
                            const basic_string_view<CharT,Traits>& rhs )
  {
    return basic_string_view<CharT,Traits>(lhs) <= rhs;
  }

  template<typename CharT, typename Traits, typename Allocator>
  inline bool operator <= ( const basic_string_view<CharT,Traits>& lhs,
                            const std::basic_string<CharT,Traits,Allocator>& rhs )
  {
    return lhs <= basic_string_view<CharT,Traits>(rhs);
  }

  //--------------------------------------------------------------------------

  template<typename CharT, typename Traits>
  inline bool operator >= ( const basic_string_view<CharT,Traits>& lhs,
                            const basic_string_view<CharT,Traits>& rhs )
    noexcept
  {
    return lhs.compare(rhs) >= 0;
  }

  template<typename CharT, typename Traits>
  inline bool operator >= ( const basic_string_view<CharT,Traits>& lhs,
                            const CharT* rhs )
    noexcept
  {
    return lhs >= basic_string_view<CharT,Traits>(rhs);
  }

  template<typename CharT, typename Traits>
  inline bool operator >= ( const CharT* lhs,
                            const basic_string_view<CharT,Traits>& rhs )
    noexcept
  {
    return basic_string_view<CharT,Traits>(lhs) >= rhs;
  }

  template<typename CharT, typename Traits, typename Allocator>
  inline bool operator >= ( const std::basic_string<CharT,Traits,Allocator>& lhs,
                            const basic_string_view<CharT,Traits>& rhs )
  {
    return basic_string_view<CharT,Traits>(lhs) >= rhs;
  }

  template<typename CharT, typename Traits, typename Allocator>
  inline bool operator >= ( const basic_string_view<CharT,Traits>& lhs,
                            const std::basic_string<CharT,Traits,Allocator>& rhs )
  {
    return lhs >= basic_string_view<CharT,Traits>(rhs);
  }

} // namespace es

#endif /* BPSTD_DETAIL_STRING_VIEW_INL */
