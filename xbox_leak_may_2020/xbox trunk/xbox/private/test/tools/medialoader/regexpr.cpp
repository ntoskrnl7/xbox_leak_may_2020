//+---------------------------------------------------------------------------
//
//  File:         basic_regexpr.cxx
//
//  Contents:    
//
//  Classes:     
//
//  Functions:  
//
//  Coupling:    
//
//  Notes:        
//
//  History:     1-11-1999    ericne    Created
//
//----------------------------------------------------------------------------

// unlimited inline expansion (compile with /Ob1 or /Ob2)
#pragma inline_depth(255)

// warning C4355: 'this' : used in base member initializer list
// warning C4511: copy constructor could not be generated
// warning C4512: assignment operator could not be generated
// warning C4660: template-class specialization 'foo<bar>' is already instantiated
// warning C4706: assignment within conditional expression
// warning C4786: identifier was truncated to '255' characters in the debug information
// warning C4800: 'int' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning( disable : 4355 4511 4512 4660 4706 4786 4800 )

#include <assert.h>
#include <malloc.h> // for _alloca	
#include <algorithm>
#include <minmax.h>
#include "regexpr.h"

using namespace std;

namespace regex 
{

#ifdef _MT
// Global critical section used to synchronize the creation of static const patterns
CRegExCritSect g_objRegExCritSect;
#endif

// For use while doing uppercase/lowercase conversions:
// For use while doing uppercase/lowercase conversions:
inline  char    to_upper(  char    ch ) { return ( char  )toupper(ch); }
inline  char    to_lower(  char    ch ) { return ( char  )tolower(ch); }
inline wchar_t to_upper( wchar_t ch ) { return (wchar_t)towupper(ch); }
inline wchar_t to_lower( wchar_t ch ) { return (wchar_t)towlower(ch); }
template< typename II, typename CI > 
void to_upper( II ibegin, CI iend )
{
     for( ; static_cast<CI>(ibegin) != iend; ++ibegin )
          *ibegin = to_upper( *ibegin );
}
template< typename II, typename CI > 
void to_lower( II ibegin, CI iend )
{
     for( ; static_cast<CI>(ibegin) != iend; ++ibegin )
          *ibegin = to_lower( *ibegin );
}

template< typename II, typename CI >
unsigned parse_int( II & istr, CI iend, const unsigned m_max = unsigned(-1) )
{
     unsigned retval = 0;
     while( static_cast<CI>(istr) != iend && '0' <= *istr && '9' >= *istr && m_max > retval )
     {
          retval *= 10;
          retval += (unsigned)*istr - (unsigned)'0';
          ++istr;
     }
     if( m_max < retval )
     {
          retval /= 10;
          --istr;
     }
     return retval;
}

// The STL member function adapters don't handle const member functions,
// so explicitly handle that special case with const_mem_fun1()
template<class R, class Ty, class A>
class const_mem_fun1_t : public binary_function<const Ty *, A, R> 
{
     R (Ty::*m_p)(A) const throw();
public:
     explicit const_mem_fun1_t(R (Ty::*p)(A) const throw()) throw()
          : m_p(p) {}
     R operator()(const Ty * p, A arg) const throw()
          {return ((p->*m_p)(arg));}
};

template<class R, class Ty, class A>
inline const_mem_fun1_t<R,Ty,A> mem_fun1(R (Ty::*p)(A) const throw()) throw()
     {return const_mem_fun1_t<R,Ty,A>(p);}

// This class is used to speed up character set matching by providing
// a bitset that spans the ASCII range. std::bitset is not used because
// the range-checking slows it down.
// Note: The division and modulus operations are optimized by the compiler
// into bit-shift operations.
class ascii_bitvector
{
     typedef unsigned int elem_type;

     enum { CBELEM = 8 * sizeof elem_type,      // count of bytes per element
              CELEMS = (UCHAR_MAX+1) / CBELEM }; // number of element in array
     elem_type m_rg[ CELEMS ];

     // Used to inline operations like: bv1 |= ~bv2; without creating temp bit vectors.
     struct not_ascii_bitvector
     {
          const ascii_bitvector & m_ref;
          not_ascii_bitvector( const ascii_bitvector & ref ) throw()
                : m_ref(ref) {}
     };
public:
     ascii_bitvector() throw()
          { memset( m_rg, 0, CELEMS * sizeof elem_type ); }
     
     inline void set( unsigned char ch ) throw()
          { m_rg[ ( ch / CBELEM ) ] |= ( (elem_type)1U << ( ch % CBELEM ) ); }
     
     inline bool operator[]( unsigned char ch ) const throw()
          { return 0 != ( m_rg[ ( ch / CBELEM ) ] & ( (elem_type)1U << ( ch % CBELEM ) ) ); }
     
     inline not_ascii_bitvector operator~() const throw()
          { return not_ascii_bitvector(*this); }
     
     inline ascii_bitvector & operator|=( const ascii_bitvector & that ) throw()
          { for( int i=0; i<CELEMS; ++i )
                  m_rg[i] |= that.m_rg[i];
             return *this; }
     
     inline ascii_bitvector & operator|=( const not_ascii_bitvector & that ) throw()
          { for( int i=0; i<CELEMS; ++i )
                  m_rg[i] |= ~that.m_ref.m_rg[i];
             return *this; }
};

const ascii_bitvector & get_digit_vector(void)
{
     // 0-9
     class digit_vector : public ascii_bitvector
     {
     public:
          digit_vector()
          {
                unsigned char ich;
                for( ich ='0'; ich <= '9'; ++ich )
                     set(ich);
          }
     };

     static const digit_vector s_digit_vector;
     return s_digit_vector;
}

const ascii_bitvector & get_word_vector(void)
{
     // a-zA-Z_0-9
     class word_vector : public ascii_bitvector
     {
     public:
          word_vector()
          {
                unsigned char ich;
                for( ich = 'a'; ich <= 'z'; ++ich )
                     set(ich);
                for( ich = 'A'; ich <= 'Z'; ++ich )
                     set(ich);
                for( ich = '0'; ich <= '9'; ++ich )
                     set(ich);
                set('_');
          }
     };

     static const word_vector s_word_vector;
     return s_word_vector;
}

const ascii_bitvector & get_space_vector(void)
{
     // " \t\r\n\f"
     class space_vector : public ascii_bitvector
     {
     public:
          space_vector()
          {
                set(' ');
                set('\t');
                set('\r');
                set('\n');
                set('\f');
          }
     };

     static const space_vector s_space_vector;
     return s_space_vector;
}

//
// Operator implementations
//

// Base type used so that all derived operators share typedefs.
template< typename CI >
struct op_t : public binary_function<match_param<CI>,CI,bool>
{
     typedef CI const_iterator;
     typedef typename iterator_traits<CI>::value_type char_type;
};

// Evaluates the beginning-of-string condition
template< typename CI >
struct bos_t : public op_t<CI>
{
     inline bool operator()( const match_param<CI> & param, CI iter ) const
     {
          return param.ibegin == iter;
     }
};

// Find the beginning of a line, either beginning of a string, or the character
// immediately following a newline
template< typename CI >
struct bol_t : public bos_t<CI>
{
     inline bool operator()( const match_param<CI> & param, CI iter ) const
     {
          return bos_t<CI>::operator()(param,iter) || char_type('\n') == *--iter;
     }
};

// Evaluates end-of-string condition for string's
template< typename CI >
struct eos_t : public op_t<CI>
{
     inline bool operator()( const match_param<CI> & param, CI iter ) const
     {
          return param.istop == iter;
     }
};

// Evaluates end-of-string condidition for C-style string's when the length is unknown by
// looking for the null-terminator.
template< typename CI >
struct eocs_t : public op_t<CI>
{
     inline bool operator()( const match_param<CI> &, CI iter ) const
     {
          return char_type('\0') == *iter;
     }
};

// Evaluates end-of-line conditions, either the end of the string, or a
// return or newline character.
template< typename EOS >
struct eol_t_t : public EOS
{
     typedef typename EOS::const_iterator CI;
     typedef typename EOS::char_type char_type;
     inline bool operator()( const match_param<CI> & param, CI iter ) const
     {
          return EOS::operator()(param,iter) || char_type('\n') == *iter || char_type('\r') == *iter;
     }
};

template< typename CI > struct eol_t  : public eol_t_t<eos_t<CI> >  {};
template< typename CI > struct eocl_t : public eol_t_t<eocs_t<CI> > {};

// Evaluates perl's end-of-string conditions, either the end of the string, or a
// newline character followed by end of string. (Only used by $ and /Z assertions)
template< typename EOS >
struct peos_t_t : public EOS
{
     typedef typename EOS::const_iterator CI;
     typedef typename EOS::char_type char_type;
     inline bool operator()( const match_param<CI> & param, CI iter ) const
     {
          return EOS::operator()(param,iter) || ( ( char_type('\n') == *iter ) && EOS::operator()(param,++iter) );
     }
};

template< typename CI > struct peos_t  : public peos_t_t<eos_t<CI> >  {};
template< typename CI > struct peocs_t : public peos_t_t<eocs_t<CI> > {};

// compare two characters, case-sensitive
template< typename CH >
struct ch_neq_t : public binary_function<CH, CH, bool>
{
     typedef CH char_type;
     inline bool operator()( register CH ch1, register CH ch2 ) const
     {
          return ch1 != ch2;
     }
};

// Compare two characters, disregarding case
template< typename CH >
struct ch_neq_nocase_t : public binary_function<CH, CH, bool>
{
     typedef CH char_type;
     inline bool operator()( register CH ch1, register CH ch2 ) const
     {
          return to_upper(ch1) != to_upper(ch2);
     }
};

//
// Helper functions for match and substitute
//

template< typename CI >
size_t string_length( CI iter )
{
     size_t n = 0;
     while( 0 != *iter++ )
          ++n;
     return n;
}

template< typename CI >
backref_tag<CI> _do_match( const basic_rpattern_base<CI> & pat, match_param<CI> & param ) throw()
{
     typedef typename iterator_traits<CI>::value_type char_type;
     
     bool         floop  = pat.loops();
     unsigned    flags  = pat.flags();
     width_type nwidth = pat.get_width();

     const sub_expr<CI> * pfirst = pat.get_first_subexpression();

     try
     {
          vector<backref_tag<CI> > rgbackrefs; // dummy backref vector

          if( NULL == param.prgbackrefs )
                param.prgbackrefs = & rgbackrefs;

          param.prgbackrefs->resize( pat._cgroups_total() ); 
          fill( param.prgbackrefs->begin(), param.prgbackrefs->end(), backref_tag<CI>() );

          // If a pattern is optimized for CSTRINGS, it can save a call
          // to calculate the length of the string.
          if( CI(0) == param.istop && ( ( RIGHTMOST & flags ) || ( 0 == ( CSTRINGS & flags ) ) ) )
                param.istop = param.istart + string_length( param.istart );

          if( CI(0) != param.istop )
          {
                // If the minimum width of the pattern exceeds the width of the
                // string, a succesful match is impossible
                if( nwidth.m_min <= (size_t)distance( param.istart, param.istop ) )
                {
                     CI local_istop = param.istop;
                     advance( local_istop, -int( nwidth.m_min ) );

                     if( RIGHTMOST & flags )
                     {
                          // begin trying to match after the last character.
                          // Continue to the beginning
                          for( CI icur = local_istop; icur >= param.istart; --icur )
                                if( pfirst->domatch( param, icur ) ) 
                                     break; // m_floop not used for rightmost matches
                     }
                     else
                     {
                          // begin trying to match before the first character.
                          // Continue to the end
                          for( CI icur = param.istart; icur <= local_istop; ++icur )
                                if( pfirst->domatch( param, icur ) || ! floop )
                                     break;
                     }
                }
          }
          else
          {
                // begin trying to match before the first character.
                // Continue to the end
                for( CI icur = param.istart; ; ++icur )
                {
                     if( pfirst->domatch( param, icur ) || ! floop )
                          break;
                     if( char_type('\0') == *icur )
                          break;
                }
          }
     }
     catch(...) // bad alloc, stack overflow?
     {
          fill( param.prgbackrefs->begin(), param.prgbackrefs->end(), backref_tag<CI>() );
     }

     // Shrink the backref vector to chop off information about the "invisible" groups
     param.prgbackrefs->resize( pat.cgroups() );

     return (*param.prgbackrefs)[0];
}

template< typename CI, typename CH, typename TR, typename AL >
size_t _do_subst( basic_regexpr<CH,TR,AL> & str, const basic_rpattern_base<CI> & pat, size_t strpos, size_t strlen ) throw(bad_alloc)
{
     typedef iterator_traits<CI>::value_type char_type;
     typedef list<subst_node>::const_iterator LCI;
     enum { UPPER = -1, NIL, LOWER } next = NIL, rest = NIL;
     bool first = true;
     size_t old_strpos = strpos;
     const list<subst_node> & subst_list = pat.get_subst_list();
     basic_string<CH,TR,AL>::iterator itstrlen = str.begin();
     advance( itstrlen, strpos + strlen );
     const basic_string<char_type> & subst = pat.get_subst();
     push_new_handler pnh( &my_new_handler );
     
     for( LCI isubst = subst_list.begin(); isubst != subst_list.end(); ++isubst )
     {
          size_t sublen;
          basic_string<CH,TR,AL>::const_iterator  itsubpos1; // iter into str
          basic_string<CH,TR,AL>::const_iterator  itsublen1;
          basic_string<char_type>::const_iterator itsubpos2; // iter into subst string
          basic_string<char_type>::const_iterator itsublen2;
          basic_string<CH,TR,AL>::iterator itstrpos = str.begin();
          advance( itstrpos, strpos );

          switch( isubst->stype )
          {
                case subst_node::SUBST_STRING:
                     itsubpos2 = subst.begin();
                     advance( itsubpos2, isubst->subst_string.rstart );
                     itsublen2 = itsubpos2;
                     advance( itsublen2, isubst->subst_string.rlength );

                     first ? str.replace( itstrpos, itstrlen, itsubpos2, itsublen2 ) : 
                                str.insert( itstrpos, itsubpos2, itsublen2 );
                     sublen = distance( itsubpos2, itsublen2 );
                     break;

                case subst_node::SUBST_BACKREF:
                     switch( isubst->subst_backref )
                     {
                     case subst_node::PREMATCH:
                          itsubpos1 = str.backref_str().begin();
                          itsublen1 = itsubpos1;
                          advance( itsublen1, sublen = str.rstart() );
                          break;
                     case subst_node::POSTMATCH:
                          itsubpos1 = str.backref_str().begin();
                          advance( itsubpos1, str.rstart() + str.rlength() );
                          itsublen1 = str.backref_str().end();
                          break;
                     default:
                          itsubpos1 = str.backref_str().begin();
                          advance( itsubpos1, str.rstart( isubst->subst_backref ) );
                          itsublen1 = itsubpos1;
                          advance( itsublen1, str.rlength( isubst->subst_backref ) );
                          break;
                     }

                     first ? str.replace( itstrpos, itstrlen, itsubpos1, itsublen1 ) : 
                                str.insert( itstrpos, itsubpos1, itsublen1 );
                     sublen = distance( itsubpos1, itsublen1 );
                     break;

                case subst_node::SUBST_OP:
                     switch( isubst->op )
                     {
                     case subst_node::UPPER_ON:
                          rest = UPPER;
                          break;
                     case subst_node::UPPER_NEXT:
                          next = UPPER;
                          break;
                     case subst_node::LOWER_ON:
                          rest = LOWER;
                          break;
                     case subst_node::LOWER_NEXT:
                          next = LOWER;
                          break;
                     case subst_node::ALL_OFF:
                          rest = NIL;
                          break;
                     default:
                          __assume(0);
                     }
                     continue; // jump to the next item in the list

                default:
                     __assume(0);
          }

          first = false;

          // Are we upper- or lower-casing this string?
          if( rest )
          {
                basic_string<CH,TR,AL>::iterator istart = str.begin();
                advance( istart, strpos );
                basic_string<CH,TR,AL>::const_iterator istop = istart;
                advance( istop, sublen );
                switch( rest )
                {
                case UPPER:
                     to_upper( istart, istop );
                     break;
                case LOWER:
                     to_lower( istart, istop );
                     break;
                default:
                     __assume(0);
                }
          }

          // Are we upper- or lower-casing the next character?
          if( next )
          {
                switch( next )
                {
                case UPPER:
                     str[strpos] = to_upper(str[strpos]);
                     break;
                case LOWER:
                     str[strpos] = to_lower(str[strpos]);
                     break;
                default:
                     __assume(0);
                }
                next = NIL;
          }

          strpos += sublen;
     }

     // If *first* is still true, then we never called str.replace, and the substitution
     // string is empty. Erase the part of the string that the pattern matched.
     if( first )
          str.erase( strpos, strlen );

     // return length of the substitution
     return strpos - old_strpos;
}

//
// Implementation of basic_regexpr
//

template< typename CH, typename TR, typename AL >
size_t basic_regexpr<CH,TR,AL>::substitute( 
     const basic_rpattern_base<basic_regexpr<CH,TR,AL>::const_iterator> & pat, 
     size_type pos, 
     size_type len ) throw(bad_alloc)
{
     if( pat.flags() & CSTRINGS )
     {
          assert( ! "You can't use a pattern optimized for CSTRINGS with regexpr::substitute" );
          return 0;
     }

     backref_vector rgbackrefs; // dummy backref vector
     backref_vector * prgbackrefs = & rgbackrefs;
     const bool fsave_backrefs = ( pat.uses_backrefs() || !( pat.flags() & NOBACKREFS ) );

     if( fsave_backrefs )
     {
          prgbackrefs = & m_rgbackrefs;
          m_pbackref_str = & ( m_backref_str = *this );
     }
     else
     {
          m_backref_str.erase();
          m_pbackref_str = this;
          m_rgbackrefs.resize( 0 );
     }

     backref_type br;
     size_t csubst = 0;
     long stop_offset = ( len == npos ? 
                                 m_pbackref_str->size() : 
                                 min( pos + len, m_pbackref_str->size() ) );

     match_param<const_iterator> param( m_pbackref_str->begin(),
                                                    m_pbackref_str->begin(),
                                                    prgbackrefs );
     advance( param.istart, pos );
     advance( param.istop, stop_offset );
     param.ibegin = param.istart;

     if( GLOBAL & pat.flags() )
     {
          const bool fAll    = ( ALLBACKREFS    == ( ALLBACKREFS    & pat.flags() ) );
          const bool fFirst = ( FIRSTBACKREFS == ( FIRSTBACKREFS & pat.flags() ) );
          backref_vector rgtempbackrefs; // temporary vector used if fsave_backrefs

          long pos_offset = 0; // keep track of how much the backref_str and
                                      // the current string are out of sync

          while( br = _do_match( pat, param ) )
          {
                ++csubst;
                size_type match_length = distance( br.first, br.second );
                pos = distance( m_pbackref_str->begin(), br.first );
                size_type subst_length = _do_subst( *this, pat, pos + pos_offset, match_length );

                if( fsave_backrefs )
                {
                     pos += match_length;
                     pos_offset += ( subst_length - match_length );

                     // Handle specially the backref flags
                     if( fFirst )
                          rgtempbackrefs.push_back( br );
                     else if( fAll )
                          rgtempbackrefs.insert( rgtempbackrefs.end(),
                                                         param.prgbackrefs->begin(), 
                                                         param.prgbackrefs->end() );
                     else
                          rgtempbackrefs.swap( *param.prgbackrefs );
                }
                else
                {
                     pos += subst_length;
                     stop_offset += ( subst_length - match_length );

                     // we're not saving backref information, so we don't
                     // need to do any special backref maintenance here
                }
                
                // prevent a pattern that matches 0 characters from matching
                // again at the same point in the string
                if( 0 == match_length )
                {
                     if( br.first == param.istop ) // We're at the end, so we're done
                          break;
                     ++pos;
                }

                param.istart = m_pbackref_str->begin();
                advance( param.istart, pos ); // ineffecient for bidirectional iterators.

                param.istop = m_pbackref_str->begin();
                advance( param.istop, stop_offset ); // ineffecient for bidirectional iterators.
          }

          // If we did special backref handling, swap the backref vectors
          if( fsave_backrefs && ( !br || fFirst || fAll ) )
                param.prgbackrefs->swap( rgtempbackrefs );
          else if( ! (*param.prgbackrefs)[0] )
                param.prgbackrefs->clear();
     }
     else if( br = _do_match( pat, param ) )
     {
          ++csubst;
          _do_subst( *this, pat,
                         distance( m_pbackref_str->begin(), br.first ),
                         distance( br.first, br.second ) );
     }

     if( NOBACKREFS == ( pat.flags() & NOBACKREFS ) )
          param.prgbackrefs->clear();

     return csubst;
}

//
// Helper functions called from both basic_regexpr match methods
//

template< typename EOS >
backref_tag< typename EOS::const_iterator > _match_helper( 
     const basic_rpattern_base<typename EOS::const_iterator> & pat,
     match_param<typename EOS::const_iterator> & param,
     EOS eos )
{
     typedef typename EOS::const_iterator CI;

     if( GLOBAL & pat.flags() ) // do a global find
     {
          // The NOBACKREFS flag is ignored in the match method.
          const bool fAll    = ( ALLBACKREFS    == ( ALLBACKREFS    & pat.flags() ) );
          const bool fFirst = ( FIRSTBACKREFS == ( FIRSTBACKREFS & pat.flags() ) );

          backref_tag<CI>  br;
          vector<backref_tag<CI> > rgtempbackrefs;
          
          while( br = _do_match( pat, param ) )
          {
                // Handle specially the backref flags
                if( fFirst )
                     rgtempbackrefs.push_back( br );
                else if( fAll )
                     rgtempbackrefs.insert( rgtempbackrefs.end(),
                                                    param.prgbackrefs->begin(),
                                                    param.prgbackrefs->end() );
                else
                     rgtempbackrefs.swap( *param.prgbackrefs );

                if( br.first == ( param.istart = br.second ) )
                {
                     if( eos( param, param.istart ) )
                          break;
                     ++param.istart;
                }
          }

          // restore the backref vectors
          if( !br || fFirst || fAll )
                param.prgbackrefs->swap( rgtempbackrefs );
          else if( ! (*param.prgbackrefs)[0] )
                param.prgbackrefs->clear();

          return param.prgbackrefs->empty() ? backref_tag<CI>() : (*param.prgbackrefs)[0];
     }
     else 
          return _do_match( pat, param );
}

template< typename CH, typename TR, typename AL >
basic_regexpr<CH,TR,AL>::backref_type basic_regexpr<CH,TR,AL>::match( 
     const basic_rpattern_base<const_iterator> & pat,
     size_type pos, size_type len ) const throw()
{
     if( pat.flags() & CSTRINGS )
     {
          assert( ! "A pattern optimized for CSTRINGS can only be used with the static regexpr::match method" );
          return backref_type();
     }

     m_pbackref_str = this;
     m_backref_str.erase(); // free up unused memory

     const_iterator istart = begin();
     advance( istart, pos );

     const_iterator istop;
     if( len == npos || pos + len >= size() )
          istop = end();
     else
          advance( istop = begin(), pos + len );

     match_param<const_iterator> param( istart, istop, & m_rgbackrefs );
     return _match_helper<eos_t<const_iterator> >( pat, param, eos_t<const_iterator>() );
}

backref_tag<const char *> _static_match_helper( 
     const char * szstr, 
     const basic_rpattern_base<const char *> & pat,
     vector< backref_tag< const char * > > * prgbackrefs ) throw()
{
     vector< backref_tag< const char * > > rgdummyvector;
     if( NULL == prgbackrefs )
          prgbackrefs = &rgdummyvector;

     match_param<const char *> param( szstr, NULL, prgbackrefs );
     return _match_helper<eocs_t<const char *> >( pat, param, eocs_t<const char *>() );
}

backref_tag<const wchar_t *> _static_match_helper( 
     const wchar_t * szstr, 
     const basic_rpattern_base<const wchar_t *> & pat,
     vector< backref_tag< const wchar_t * > > * prgbackrefs ) throw()
{
     vector< backref_tag< const wchar_t * > > rgdummyvector;
     if( NULL == prgbackrefs )
          prgbackrefs = &rgdummyvector;

     match_param<const wchar_t *> param( szstr, NULL, prgbackrefs );
     return _match_helper<eocs_t<const wchar_t *> >( pat, param, eocs_t<const wchar_t *>() );
}

//
// Helper function called from both basic_regexpr::count methods
//
template< typename EOS >
size_t _count_helper( 
     const basic_rpattern_base<typename EOS::const_iterator> & pat,
     match_param<typename EOS::const_iterator> & param,
     EOS eos )
{
     typedef typename EOS::const_iterator CI;

     size_t cmatches = 0;
     vector<backref_tag<CI> > rgbackrefs; // dummy backref vector
     backref_tag<CI>             br;
     param.prgbackrefs = &rgbackrefs;

     while( br = _do_match( pat, param ) )
     {
          ++cmatches;

          if( br.first == ( param.istart = br.second ) )
          {
                if( eos( param, param.istart ) )
                     break;
                ++param.istart;
          }
     }
     return cmatches;
}

template< typename CH, typename TR, typename AL >
size_t basic_regexpr<CH,TR,AL>::count( 
     const basic_rpattern_base<basic_regexpr<CH,TR,AL>::const_iterator> & pat,
     size_type pos, 
     size_type len ) const throw()
{
     if( pat.flags() & CSTRINGS )
     {
          assert( ! "A pattern optimized for CSTRINGS can only be used with the static regexpr::count method" );
          return backref_type();
     }

     m_pbackref_str = this;

     const_iterator istart = begin();
     advance( istart, pos );

     const_iterator istop;
     if( len == npos || pos + len >= size() )
          istop = end();
     else
          advance( istop = begin(), pos + len );

     match_param<const_iterator> param( istart, istop, NULL );
     return _count_helper<eos_t<const_iterator> >( pat, param, eos_t<const_iterator>() );
}

size_t _static_count_helper( 
     const char * szstr, 
     const basic_rpattern_base<const char *> & pat ) throw()
{
     match_param<const char *> param( szstr, NULL, NULL );
     return _count_helper<eocs_t<const char *> >( pat, param, eocs_t<const char *>() );
}

size_t _static_count_helper( 
     const wchar_t * szstr, 
     const basic_rpattern_base<const wchar_t *> & pat ) throw()
{
     match_param<const wchar_t *> param( szstr, NULL, NULL );
     return _count_helper<eocs_t<const wchar_t *> >( pat, param, eocs_t<const wchar_t *>() );
}

// Base class for sub-expressions which are zero-width 
// (i.e., assertions eat no characters during matching)
// Assertions cannot be quantified.
template< typename CI >
class assertion : public sub_expr<CI>
{
public:
     virtual bool is_assertion() const throw() { return true; }
protected:
     virtual width_type _width_this() throw() { return width_type(0,0); }
};

template< typename OP >
class assert_op : public assertion<typename OP::const_iterator>
{
public:
     typedef OP op_type;
     typedef typename OP::const_iterator CI;
     virtual bool domatch( match_param<CI> & param, CI icur ) const throw()
     {
          return ( assert_op::_match_this(param,icur) && match_next(param,icur) );
     }
protected:
     virtual bool _match_this( match_param<CI> & param, CI & icur ) const throw()
     {
          return m_op( param, icur );
     }
     op_type m_op;
};

template< typename CI >
assertion<CI> * create_bos( unsigned /*flags*/ )
{
     return new assert_op<bos_t<CI> >();
}

template< typename CI >
assertion<CI> * create_eos( unsigned flags )
{
     switch( CSTRINGS & flags )
     {
     case 0:
          return new assert_op<peos_t<CI> >();
     case CSTRINGS:
          return new assert_op<peocs_t<CI> >();
     default:
          __assume(0); // tells the compiler that this is unreachable
     }
}

template< typename CI >
assertion<CI> * create_eoz( unsigned flags )
{
     switch( CSTRINGS & flags )
     {
     case 0:
          return new assert_op<eos_t<CI> >();
     case CSTRINGS:
          return new assert_op<eocs_t<CI> >();
     default:
          __assume(0); // tells the compiler that this is unreachable
     }
}

template< typename CI >
assertion<CI> * create_bol( unsigned flags )
{
     switch( MULTILINE & flags )
     {
     case 0:
          return new assert_op<bos_t<CI> >();
     case MULTILINE:
          return new assert_op<bol_t<CI> >();
     default:
          __assume(0); // tells the compiler that this is unreachable
     }
}

template< typename CI >
assertion<CI> * create_eol( unsigned flags )
{
     switch( ( MULTILINE | CSTRINGS ) & flags )
     {
     case 0:
          return new assert_op<peos_t<CI> >();
     case MULTILINE:
          return new assert_op<eol_t<CI> >();
     case CSTRINGS:
          return new assert_op<peocs_t<CI> >();
     case MULTILINE | CSTRINGS:
          return new assert_op<eocl_t<CI> >();
     default:
          __assume(0); // tells the compiler that this is unreachable
     }
}

template< typename CI >
class match_atom : public sub_expr<CI>
{
public:
	typedef basic_string<sub_expr<CI>::char_type> string_type;
	typedef string_type::iterator iterator;
	typedef string_type::const_iterator const_iterator;

     match_atom( iterator istart, const_iterator istop ) 
          : m_istart( istart ), m_istop( istop ) {}

     const iterator m_istart;
     const const_iterator m_istop;
protected:
     virtual width_type _width_this() throw() 
     { 
          size_t width = distance<const_iterator>( m_istart, m_istop ); 
          return width_type( width, width );
     }
};

template< typename EOS >
class match_atom_t : public match_atom<typename EOS::const_iterator>
{
public:
     typedef EOS eos_type;
     typedef typename EOS::const_iterator CI;

     match_atom_t( iterator istart, const_iterator istop ) 
          : match_atom<CI>( istart, istop ) {}

     virtual bool domatch( match_param<CI> & param, CI icur ) const throw()
     {
          return ( match_atom_t::_match_this(param,icur) && match_next(param,icur) );
     }
protected:
     virtual bool _match_this( match_param<CI> & param, CI & icur ) const throw()
     {
          CI icur_tmp = icur;
          const_iterator ithis = m_istart;
          for( ; ithis != m_istop; ++icur_tmp, ++ithis )
          {
                if( m_eos( param, icur_tmp ) || *ithis != *icur_tmp )
                     return false;
          }
          icur = icur_tmp;
          return true;
     }
     eos_type  m_eos;
};

template< typename EOS >
class match_atom_nocase_t : public match_atom<typename EOS::const_iterator>
{
public:
     typedef EOS eos_type;
     typedef typename EOS::const_iterator CI;

     match_atom_nocase_t( iterator istart, const_iterator istop ) 
          : match_atom<CI>( istart, istop ), 
		  m_strlower( static_cast<const_iterator>(istart), istop ) 
     {
          // Store the uppercase version of the atom in [m_istart,m_istop).
          to_upper( m_istart, m_istop );
          // Store the lowercase version of the atom in m_strlower.
          to_lower( m_strlower.begin(), m_strlower.end() );
     }

     virtual bool domatch( match_param<CI> & param, CI icur ) const throw()
     {
          return ( match_atom_nocase_t::_match_this(param,icur) && match_next(param,icur) );
     }
protected:
     virtual bool _match_this( match_param<CI> & param, CI & icur ) const throw()
     {
          CI icur_tmp = icur;
          const_iterator ithisu    = m_istart;              // uppercase
          const_iterator ithisl    = m_strlower.begin(); // lowercase
          for( ; ithisu != m_istop; ++icur_tmp, ++ithisu, ++ithisl )
          {
                if( m_eos( param, icur_tmp ) || ( *ithisu != *icur_tmp && *ithisl != *icur_tmp ) )
                     return false;
          }
          icur = icur_tmp;
          return true;
     }
     eos_type     m_eos;
     string_type m_strlower;
};

template< typename CI >
match_atom<CI> * create_atom(
     basic_string<iterator_traits<CI>::value_type>::iterator istart,
     basic_string<iterator_traits<CI>::value_type>::const_iterator istop, 
     unsigned flags )
{
     switch( ( NOCASE | CSTRINGS ) & flags )
     {
     case 0:
          return new match_atom_t<eos_t<CI> >( istart, istop );
     case NOCASE:
          return new match_atom_nocase_t<eos_t<CI> >( istart, istop );
     case CSTRINGS:
          return new match_atom_t<eocs_t<CI> >( istart, istop );
     case NOCASE | CSTRINGS:
          return new match_atom_nocase_t<eocs_t<CI> >( istart, istop );
     default:
          __assume(0); // tells the compiler that this is unreachable
     }
}

template< typename CI >
match_atom<CI> * create_atom( 
     basic_string<iterator_traits<CI>::value_type>::iterator istart, 
     unsigned flags )
{
     basic_string<iterator_traits<CI>::value_type>::const_iterator istop = istart;
     return create_atom<CI>( istart, ++istop, flags );
}

template< typename CI >
class match_any : public sub_expr<CI>
{
protected:
     virtual width_type _width_this() throw() { return width_type(1,1); }
};

template< typename EOS >
class match_any_t : public match_any<typename EOS::const_iterator>
{
public:
     typedef EOS eos_type;
     typedef typename EOS::const_iterator CI;

     virtual bool domatch( match_param<CI> & param, CI icur ) const throw()
     {
          return ( match_any_t::_match_this(param,icur) && match_next(param,icur) );
     }
protected:
     virtual bool _match_this( match_param<CI> & param, CI & icur ) const throw()
     {
          if( m_eos( param, icur ) )
                return false;
          ++icur;
          return true;
     }
     eos_type m_eos;
};

template< typename CI >
match_any<CI> * create_any( unsigned flags ) 
{
     switch( ( SINGLELINE | CSTRINGS ) & flags )
     {
     case 0:
          return new match_any_t<eol_t<CI> >();
     case SINGLELINE:
          return new match_any_t<eos_t<CI> >();
     case CSTRINGS:
          return new match_any_t<eocl_t<CI> >();
     case SINGLELINE | CSTRINGS:
          return new match_any_t<eocs_t<CI> >();
     default:
          __assume(0); // tells the compiler that this is unreachable
     }
}

typedef pair<wchar_t,wchar_t> range_type;
const vector<range_type> g_rgranges; // empty
const vector<wctype_t>    g_rgwctype; // empty

template< typename CI >
class match_charset : public sub_expr<CI>
{
public:
     match_charset( bool fcomplement,
                         const ascii_bitvector & bvect )
          : m_fcomplement( fcomplement ),
             m_rgascii( bvect ),
             m_rgranges( g_rgranges ),
             m_ncharflags(0),
             m_rgcharflagsoff( g_rgwctype )
     {
     }

     // Note that only the references are copied here -- they are not ref counted.
     // Beware of variable lifetime issues.
     match_charset( const match_charset<CI> & that )
          : m_fcomplement( that.m_fcomplement ),
             m_rgascii( that.m_rgascii ),
             m_rgranges( that.m_rgranges ),
             m_ncharflags( that.m_ncharflags ),
             m_rgcharflagsoff( that.m_rgcharflagsoff )
     {
     }

     // this method should never be called. match_charset is only a base class
     // for match_charset_t
     virtual bool is_in_charset( char_type ch ) const throw()
     {
          assert(false);
          return true;
     }

     const bool                             m_fcomplement;
     const ascii_bitvector            & m_rgascii;  // bitmap for chars in range 0-255
     const vector<range_type>        & m_rgranges; // vector of included character ranges 256-65535
     wctype_t                                m_ncharflags; // Parameter to iswctype()
     const vector<wctype_t>          & m_rgcharflagsoff;
     vector<const match_charset *>    m_rgnestedcharsets;

     // The case-sensitivity of a character set is "compiled" into the ascii_bitvector
     // but not into the range vector because it is too computationally expensive. Instead,
     // when doing a unicode case-insensitive match on the ranges vector, two lookups
     // must be performed -- one lowercase and one uppercase. By contrast, only one lookup
     // is needed for the ascii_bitvector.

protected:

     match_charset( bool fcomplement, 
                         const ascii_bitvector & bvect,
                         const vector<range_type> & rgranges,
                         const vector<wctype_t> & rgcharflagsoff )
          : m_fcomplement( fcomplement ),
             m_rgascii( bvect ),
             m_rgranges( rgranges ),
             m_ncharflags(0),
             m_rgcharflagsoff( rgcharflagsoff )
     {
     }

     // this method should never be called. match_charset is only a base class
     // for match_charset_t
     virtual bool _match_this( match_param<CI> &, CI & ) const throw()
     {
          assert(false);
          return true;
     }

     template< typename SY >
     match_charset<CI> * get_altern_charset( char_type ch, unsigned flags, SY /*sy*/ ) const throw()
     {
          return basic_rpattern<CI,SY>::s_charset_map.get( ch, flags );
     }

     virtual width_type _width_this() throw() 
     { 
          return width_type(1,1);
     }
};

// Used as a template parameter to find a unicode character in an array of ranges.
class match_range : public unary_function<wchar_t,bool>
{
protected:
     const vector<range_type> & m_rgranges;

     // determines if one range is less then another.
     // used in binary search of range vector
     inline static bool _range_less( const range_type & rg1,
                                                const range_type & rg2 ) throw()
     {
          return rg1.second < rg2.first;
     }

     match_range( const vector<range_type> & rgranges )
          : m_rgranges( rgranges ) {}
};

class match_range_with_case : public match_range
{
public:
     match_range_with_case( const vector<range_type> & rgranges )
          : match_range( rgranges ) {}
     inline bool operator()( wchar_t ch ) const throw()
     {
          return binary_search( m_rgranges.begin(), m_rgranges.end(), 
                                        range_type(ch,ch), _range_less );
     }
};

class match_range_no_case : public match_range
{
public:
     match_range_no_case( const vector<range_type> & rgranges )
          : match_range( rgranges ) {}
     inline bool operator()( wchar_t ch ) const throw()
     {
          const wchar_t chup = towupper( ch );
          if( binary_search( m_rgranges.begin(), m_rgranges.end(), 
                                    range_type(chup,chup), _range_less ) )
                return true;

          const wchar_t chlo = towlower( ch );
          if( chup != chlo &&
                binary_search( m_rgranges.begin(), m_rgranges.end(), 
                                    range_type(chlo,chlo), _range_less ) )
                return true;

          return false;     
     }
};

template< typename EOS, typename RGM >
class match_charset_t : public match_charset<typename EOS::const_iterator>
{
public:
     typedef EOS eos_type;
     typedef RGM range_match_type;
     typedef typename EOS::const_iterator CI;

     match_charset_t( const match_charset<CI> & that )
          : match_charset<CI>( that ), m_rgm( m_rgranges ) 
     {
     }

     inline bool is_in_charset( char_type ch ) const throw()
     {
          return m_fcomplement != _is_in_charset( ch );
     }

     virtual bool domatch( match_param<CI> & param, CI icur ) const throw()
     {
          return ( match_charset_t::_match_this(param,icur) && match_next(param,icur) );
     }
protected:
     match_charset_t( bool fcomplement,
                            const ascii_bitvector & bvect,
                            const vector<range_type> & rgranges,
                            const vector<wctype_t> & rgcharflagsoff )
          : match_charset<CI>( fcomplement, bvect, rgranges, rgcharflagsoff ), 
             m_rgm( m_rgranges ) 
     {
     }

     // Note overloading based on parameter
     inline bool _is_in_charset( char ch ) const throw()
     {
          return     ( m_rgascii[ (unsigned char)(ch) ] )

                    || ( m_ncharflags && _isctype( ch, m_ncharflags ) )

                    || ( ! m_rgcharflagsoff.empty() && m_rgcharflagsoff.end() != 
                          find_if( m_rgcharflagsoff.begin(), m_rgcharflagsoff.end(), 
                                      not1( bind1st( ptr_fun(_isctype), ch ) ) ) )

                    || ( ! m_rgnestedcharsets.empty() && m_rgnestedcharsets.end() !=
                          find_if( m_rgnestedcharsets.begin(), m_rgnestedcharsets.end(),
                                      bind2nd( mem_fun1( &match_charset<CI>::is_in_charset ), ch ) ) );
     }

     // Note overloading based on parameter
     inline bool _is_in_charset( wchar_t ch ) const throw()
     {
          // use range_match_type to see if this character is within one of the
          // ranges stored in m_rgranges.
          return     ( ( UCHAR_MAX >= ch ) ? 
                          ( m_rgascii[ (unsigned char)(ch) ] ) :
                          ( ! m_rgranges.empty() && m_rgm( ch ) ) )

                    || ( m_ncharflags && iswctype( ch, m_ncharflags ) )

                    || ( ! m_rgcharflagsoff.empty() && m_rgcharflagsoff.end() != 
                          find_if( m_rgcharflagsoff.begin(), m_rgcharflagsoff.end(), 
                                      not1( bind1st( ptr_fun(iswctype), ch ) ) ) )

                    || ( ! m_rgnestedcharsets.empty() && m_rgnestedcharsets.end() !=
                          find_if( m_rgnestedcharsets.begin(), m_rgnestedcharsets.end(),
                                      bind2nd( mem_fun1( &match_charset<CI>::is_in_charset ), ch ) ) );
     }

     virtual bool _match_this( match_param<CI> & param, CI & icur ) const throw()
     {
          if( m_eos( param, icur ) || ! match_charset_t::is_in_charset( *icur ) )
                return false;
          ++icur;
          return true;
     }

     // range_match_type encapsulates the case-sensitivity
     // issues with doing a unicode lookup on the ranges vector.
     range_match_type m_rgm;
     eos_type            m_eos;
};

template< typename EOS, typename RGM >
class match_custom_charset_t : public match_charset_t<EOS,RGM>
{
public:
     template< typename SY >
     match_custom_charset_t( bool fcomplement,
                                     basic_string<char_type>::iterator & icur,
                                     basic_string<char_type>::const_iterator istop,
                                     unsigned flags, SY /*sy*/ ) throw(bad_regexpr,bad_alloc)
          : match_charset_t<EOS,RGM>( fcomplement, m_rgasciicustom, m_rgrangescustom, 
                                                m_rgcharflagsoffcustom )
     {
          _parse_charset( icur, istop, flags, SY() );
          _optimize();
     }

     // for including one character set in another
     match_custom_charset_t<EOS,RGM> & operator|=( const match_charset<CI> & that ) throw(bad_regexpr)
     {
          if( that.m_fcomplement )
          {
                // If no posix-style character sets are used, then we can merge this
                // nested character set directly into the enclosing character set.
                if( 0 == that.m_ncharflags &&
                     m_rgcharflagsoffcustom.empty() &&
                     m_rgnestedcharsets.empty() )
                {
                     m_rgasciicustom |= ~ that.m_rgascii;
                
                     // append the inverse of that.m_rgranges to this->m_rgrangescustom
                     wchar_t chlow = UCHAR_MAX;
                     typedef vector<range_type>::const_iterator VCI;
                     for( VCI prg = that.m_rgranges.begin(); prg != that.m_rgranges.end(); ++prg )
                     {
                          if( UCHAR_MAX + 1 != prg->first )
                                m_rgrangescustom.push_back( range_type( wchar_t(chlow+1), wchar_t(prg->first-1) ) );
                          chlow = prg->second;
                     }
                     if( WCHAR_MAX != chlow )
                          m_rgrangescustom.push_back( range_type( wchar_t(chlow+1), WCHAR_MAX ) );
                }
                else
                {
                     // There is no simple way to merge this nested character
                     // set into the enclosing character set, so we must save
                     // a pointer to the nested character set in a list.
                     // Note: the nested character set is user-defined. If the
                     // user purges the intrinsic character sets, this will point
                     // to garbage. Should I make a copy here instead?
                     m_rgnestedcharsets.push_back( &that );
                }
          }
          else
          {
                m_rgasciicustom |= that.m_rgascii;
                m_rgrangescustom.insert( m_rgrangescustom.end(),
                                                 that.m_rgranges.begin(), 
                                                 that.m_rgranges.end() );

                m_ncharflags |= that.m_ncharflags;
                m_rgcharflagsoffcustom.insert( m_rgcharflagsoffcustom.end(), 
                                                         that.m_rgcharflagsoff.begin(),
                                                         that.m_rgcharflagsoff.end() );

                m_rgnestedcharsets.insert( m_rgnestedcharsets.end(),
                                                    that.m_rgnestedcharsets.begin(),
                                                    that.m_rgnestedcharsets.end() );
          }
          return *this;
     }

protected:

     // If we reached the end of the string before finding the end of the
     // character set, then this is an ill-formed regex
     void _check_iter( basic_string<char_type>::const_iterator icur,
                             basic_string<char_type>::const_iterator istop ) throw(bad_regexpr)
     {
          if( icur == istop )
                throw bad_regexpr("expecting end of character set");
     }

     template< typename SY >
     void _parse_charset( basic_string<char_type>::iterator & icur,
                                 basic_string<char_type>::const_iterator istop,
                                 unsigned flags, SY /*sy*/ ) throw(bad_regexpr,bad_alloc)
     {
          TOKEN tok;
          char_type ch_prev = 0;
          match_charset<CI> * pcharset;
          basic_string<char_type>::iterator iprev = icur;
          const bool fnocase = ( NOCASE == ( NOCASE & flags ) );

          _check_iter( icur, istop );

          // remember the current position and grab the next token
          tok = SY::charset_token( icur, istop );
          do
          {
                _check_iter( icur, istop );

                if( CHARSET_RANGE == tok && ch_prev )
                {
                     // remember the current position
                     basic_string<char_type>::iterator iprev2 = icur;
                     char_type old_ch = ch_prev;
                     ch_prev = 0;

                     // old_ch is lower bound of a range
                     switch( SY::charset_token( icur, istop ) )
                     {
                     case CHARSET_RANGE:
                     case CHARSET_NEGATE:
                          icur = iprev2; // un-get these tokens and fall through
                     case NO_TOKEN:
                     case CHARSET_ESCAPE: // BUGBUG user-defined charset?
                          _set_bit_range( old_ch, *icur++, fnocase );
                          continue;
                     case CHARSET_BACKSPACE:
                          _set_bit_range( old_ch, char_type(8), fnocase ); // backspace
                          continue;
                     case CHARSET_END: // fall through
                     default:             // not a range.
                          icur = iprev; // backup to range token
                          _set_bit( old_ch, fnocase );
                          _set_bit( *icur++, fnocase );
                          continue;
                     }
                }

                if( ch_prev )
                     _set_bit( ch_prev, fnocase );
                ch_prev = 0;

                switch( tok )
                {
                // None of the intrinsic charsets are case-sensitive,
                // so no special handling must be done when the NOCASE 
                // flag is set.
                case CHARSET_RANGE:
                case CHARSET_NEGATE:
                case CHARSET_END:
                     icur = iprev; // un-get these tokens
                     ch_prev = *icur++;
                     continue;
                case CHARSET_BACKSPACE:
                     ch_prev = char_type(8); // backspace
                     continue;
                case ESC_DIGIT:
                     *this |= match_charset<CI>( false, get_digit_vector() );
                     continue;
                case ESC_NOT_DIGIT:
                     *this |= match_charset<CI>( true,  get_digit_vector() );
                     continue;
                case ESC_SPACE:
                     *this |= match_charset<CI>( false, get_space_vector() );
                     continue;
                case ESC_NOT_SPACE:
                     *this |= match_charset<CI>( true,  get_space_vector() );
                     continue;
                case ESC_WORD:
                     *this |= match_charset<CI>( false, get_word_vector() );
                     continue;
                case ESC_NOT_WORD:
                     *this |= match_charset<CI>( true,  get_word_vector() );
                     continue;
                case CHARSET_ALNUM:
                     m_ncharflags |= (_ALPHA|_DIGIT);
                     continue;
                case CHARSET_NOT_ALNUM:
                     m_rgcharflagsoffcustom.push_back(_ALPHA|_DIGIT);
                     continue;
                case CHARSET_ALPHA:
                     m_ncharflags |= (_ALPHA);
                     continue;
                case CHARSET_NOT_ALPHA:
                     m_rgcharflagsoffcustom.push_back(_ALPHA);
                     continue;
                case CHARSET_BLANK:
                     m_ncharflags |= (_BLANK);
                     continue;
                case CHARSET_NOT_BLANK:
                     m_rgcharflagsoffcustom.push_back(_BLANK);
                     continue;
                case CHARSET_CNTRL:
                     m_ncharflags |= (_CONTROL);
                     continue;
                case CHARSET_NOT_CNTRL:
                     m_rgcharflagsoffcustom.push_back(_CONTROL);
                     continue;
                case CHARSET_DIGIT:
                     m_ncharflags |= (_DIGIT);
                     continue;
                case CHARSET_NOT_DIGIT:
                     m_rgcharflagsoffcustom.push_back(_DIGIT);
                     continue;
                case CHARSET_GRAPH:
                     m_ncharflags |= (_PUNCT|_ALPHA|_DIGIT);
                     continue;
                case CHARSET_NOT_GRAPH:
                     m_rgcharflagsoffcustom.push_back(_PUNCT|_ALPHA|_DIGIT);
                     continue;
                case CHARSET_LOWER:
                     if( NOCASE == ( NOCASE & flags ) )
                          m_ncharflags |= (_LOWER|_UPPER);
                     else
                          m_ncharflags |= (_LOWER);
                     continue;
                case CHARSET_NOT_LOWER:
                     if( NOCASE == ( NOCASE & flags ) )
                          m_rgcharflagsoffcustom.push_back(_LOWER|_UPPER);
                     else
                          m_rgcharflagsoffcustom.push_back(_LOWER);
                     continue;
                case CHARSET_PRINT:
                     m_ncharflags |= (_BLANK|_PUNCT|_ALPHA|_DIGIT);
                     continue;
                case CHARSET_NOT_PRINT:
                     m_rgcharflagsoffcustom.push_back(_BLANK|_PUNCT|_ALPHA|_DIGIT);
                     continue;
                case CHARSET_PUNCT:
                     m_ncharflags |= (_PUNCT);
                     continue;
                case CHARSET_NOT_PUNCT:
                     m_rgcharflagsoffcustom.push_back(_PUNCT);
                     continue;
                case CHARSET_SPACE:
                     m_ncharflags |= (_SPACE);
                     continue;
                case CHARSET_NOT_SPACE:
                     m_rgcharflagsoffcustom.push_back(_SPACE);
                     continue;
                case CHARSET_UPPER:
                     if( NOCASE == ( NOCASE & flags ) )
                          m_ncharflags |= (_UPPER|_LOWER);
                     else
                          m_ncharflags |= (_UPPER);
                     continue;
                case CHARSET_NOT_UPPER:
                     if( NOCASE == ( NOCASE & flags ) )
                          m_rgcharflagsoffcustom.push_back(_UPPER|_LOWER);
                     else
                          m_rgcharflagsoffcustom.push_back(_UPPER);
                     continue;
                case CHARSET_XDIGIT:
                     m_ncharflags |= (_HEX);
                     continue;
                case CHARSET_NOT_XDIGIT:
                     m_rgcharflagsoffcustom.push_back(_HEX);
                     continue;
                case CHARSET_ESCAPE:
                     // Maybe this is a user-defined intrinsic charset
                     pcharset = get_altern_charset( *icur, flags, SY() );
                     if( NULL != pcharset )
                     {
                          *this |= *pcharset;
                          ++icur;
                          continue;
                     }
                     // else fall through
                default:
                     ch_prev = *icur++;
                     continue;
                }
          }
          while( _check_iter( iprev = icur, istop ), CHARSET_END != ( tok = SY::charset_token( icur, istop ) ) );

          if( ch_prev )
                _set_bit( ch_prev, fnocase );
     }

     void _optimize()
     {
          // this sorts on range_type.first (uses operator<() for pair templates)
          sort( m_rgrangescustom.begin(), m_rgrangescustom.end() ); 
          
          // This merges ranges that overlap
          for( size_t index = 1; index < m_rgrangescustom.size(); )
          {
                if( m_rgrangescustom[index].first <= m_rgrangescustom[index-1].second + 1 )
                {
                     m_rgrangescustom[index-1].second = max( 
                          m_rgrangescustom[index-1].second, m_rgrangescustom[index].second );
                     m_rgrangescustom.erase( m_rgrangescustom.begin() + index );
                }
                else
                     ++index;
          }
     }

     // Note overloading based on second parameter
     void _set_bit( char ch, const bool fnocase ) throw()
     { 
          if( fnocase )
          {
                m_rgasciicustom.set( (unsigned char)(tolower(ch)) );
                m_rgasciicustom.set( (unsigned char)(toupper(ch)) );
          }
          else
          {
                m_rgasciicustom.set( (unsigned char)(ch) ); 
          }
     }

     // Note overloading based on second parameter
     void _set_bit( wchar_t ch, const bool fnocase ) throw(bad_alloc)
     { 
          if( UCHAR_MAX >= ch )
                _set_bit( char(ch), fnocase );
          else
                m_rgrangescustom.push_back( range_type( ch, ch ) ); 
     }

     // Note overloading based on second parameter
     void _set_bit_range( char ch1, char ch2, const bool fnocase ) throw(bad_regexpr)
     {
          if( (unsigned char)(ch1) > (unsigned char)(ch2) )
                throw bad_regexpr("invalid range specified in character set");

          if( fnocase )
          {
                // i is unsigned int to prevent overflow if ch2 is UCHAR_MAX
                for( unsigned int i = (unsigned char)(ch1); i <= (unsigned char)(ch2); ++i )
                {
                     m_rgasciicustom.set( (unsigned char)( toupper(i) ) );
                     m_rgasciicustom.set( (unsigned char)( tolower(i) ) );
                }
          }
          else
          {
                // i is unsigned int to prevent overflow if ch2 is UCHAR_MAX
                for( unsigned int i = (unsigned char)(ch1); i <= (unsigned char)(ch2); ++i )
                     m_rgasciicustom.set( (unsigned char)(i) );
          }
     }

     // Note overloading based on second parameter
     void _set_bit_range( wchar_t ch1, wchar_t ch2, const bool fnocase ) throw(bad_regexpr,bad_alloc)
     {
          if( ch1 > ch2 )
                throw bad_regexpr("invalid range specified in character set");

          if( UCHAR_MAX >= ch1 )
                _set_bit_range( char(ch1), char( min(wchar_t(UCHAR_MAX),ch2) ), fnocase );

          if( UCHAR_MAX < ch2 )
                m_rgrangescustom.push_back( range_type( max(wchar_t(UCHAR_MAX+1),ch1), ch2 ) );
     }

     ascii_bitvector     m_rgasciicustom;
     vector<range_type> m_rgrangescustom;
     vector<wctype_t>    m_rgcharflagsoffcustom;
};


template< typename CI >
match_charset<CI> * create_charset( 
     const match_charset<CI> & that,
     unsigned flags )
{
     switch( ( NOCASE | CSTRINGS ) & flags )
     {
     case 0:
          return new match_charset_t<eos_t<CI>,match_range_with_case>( that );
     case NOCASE:
          return new match_charset_t<eos_t<CI>,match_range_no_case>( that );
     case CSTRINGS:
          return new match_charset_t<eocs_t<CI>,match_range_with_case>( that );
     case NOCASE | CSTRINGS:
          return new match_charset_t<eocs_t<CI>,match_range_no_case>( that );
     default:
          __assume(0); // tells the compiler that this is unreachable
     }
}

template< typename EOS >
class word_assertion_t : public assertion<typename EOS::const_iterator>
{
public:
     typedef EOS eos_type;
     typedef typename EOS::const_iterator CI;
     word_assertion_t() 
          : m_isword( match_charset<CI>( false, get_word_vector() ) ) {}

protected:
     bos_t<CI>  m_bos;
     eos_type    m_eos;
     match_charset_t<eos_type,match_range_with_case> m_isword;
};

template< typename EOS >
class word_boundary_t : public word_assertion_t<EOS>
{
public:
     word_boundary_t( const bool fisboundary ) 
          : m_fisboundary( fisboundary ) {}

     virtual bool domatch( match_param<CI> & param, CI icur ) const throw()
     {
          return ( word_boundary_t::_match_this(param,icur) && match_next(param,icur) );
     }
protected:
     virtual bool _match_this( match_param<CI> & param, CI & icur ) const throw()
     {
          CI iprev = icur;
          --iprev;

          const bool fprevword = ! m_bos( param, icur ) && m_isword.is_in_charset( *iprev );
          const bool fthisword = ! m_eos( param, icur ) && m_isword.is_in_charset( *icur  );

          return ( m_fisboundary == ( fprevword != fthisword ) );
     }
     const bool m_fisboundary;
};

template< typename EOS >
class word_start_t : public word_assertion_t<EOS>
{
public:
     word_start_t() {}

     virtual bool domatch( match_param<CI> & param, CI icur ) const throw()
     {
          return ( word_start_t::_match_this(param,icur) && match_next(param,icur) );
     }
protected:
     virtual bool _match_this( match_param<CI> & param, CI & icur ) const throw()
     {
          CI iprev = icur;
          --iprev;

          const bool fprevword = ! m_bos( param, icur ) && m_isword.is_in_charset( *iprev );
          const bool fthisword = ! m_eos( param, icur ) && m_isword.is_in_charset( *icur  );

          return ! fprevword && fthisword;
     }
};

template< typename EOS >
class word_stop_t : public word_assertion_t<EOS>
{
public:
     word_stop_t() {}

     virtual bool domatch( match_param<CI> & param, CI icur ) const throw()
     {
          return ( word_stop_t::_match_this(param,icur) && match_next(param,icur) );
     }
protected:
     virtual bool _match_this( match_param<CI> & param, CI & icur ) const throw()
     {
          CI iprev = icur;
          --iprev;

          const bool fprevword = ! m_bos( param, icur ) && m_isword.is_in_charset( *iprev );
          const bool fthisword = ! m_eos( param, icur ) && m_isword.is_in_charset( *icur  );

          return fprevword && ! fthisword;
     }
};

template< typename CI >
assertion<CI> * create_word_boundary( const bool fisboundary, unsigned flags )
{
     switch( CSTRINGS & flags )
     {
     case 0:
          return new word_boundary_t<eos_t<CI> >( fisboundary );
     case CSTRINGS:
          return new word_boundary_t<eocs_t<CI> >( fisboundary );
     default:
          __assume(0); // tells the compiler that this is unreachable
     }
}

template< typename CI >
assertion<CI> * create_word_start( unsigned flags )
{
     switch( CSTRINGS & flags )
     {
     case 0:
          return new word_start_t<eos_t<CI> >();
     case CSTRINGS:
          return new word_start_t<eocs_t<CI> >();
     default:
          __assume(0); // tells the compiler that this is unreachable
     }
}

template< typename CI >
assertion<CI> * create_word_stop( unsigned flags )
{
     switch( CSTRINGS & flags )
     {
     case 0:
          return new word_stop_t<eos_t<CI> >();
     case CSTRINGS:
          return new word_stop_t<eocs_t<CI> >();
     default:
          __assume(0); // tells the compiler that this is unreachable
     }
}

template< typename CI > class group_quantifier;

template< typename CI >
class match_group_base : public sub_expr<CI>
{
public:
     friend class group_quantifier<CI>;

     match_group_base( size_t cgroup )
          : m_rgalternates(), m_pptail(NULL),
             m_cgroup( cgroup ), m_nwidth(uninit_width) {}

     virtual bool domatch( match_param<CI> & param, CI icur ) const throw()
     {
          CI old_istart = CI();
          
          if( size_t(-1) != m_cgroup ) // could be -1 if this is a lookahead_assertion
          {
                old_istart = (*param.prgbackrefs)[ m_cgroup ].first;
                (*param.prgbackrefs)[ m_cgroup ].first = icur;
          }

          typedef vector<sub_expr<CI>*>::const_iterator VCI;
          for( VCI ialt = m_rgalternates.begin(); ialt != m_rgalternates.end(); ++ialt )
          {
                if( (*ialt)->domatch( param, icur ) )
                     return true;
          }

          if( size_t(-1) != m_cgroup )
                (*param.prgbackrefs)[ m_cgroup ].first = old_istart;

          return false;
     }
     
     virtual void _delete()
     {
          typedef vector<sub_expr<CI>*>::iterator VI;
          for( VI ialt = m_rgalternates.begin(); ialt != m_rgalternates.end(); ++ialt )
                delete_sub_expr( *ialt );
          sub_expr<CI>::_delete();
     }

     size_t group_number() const 
     { 
          return m_cgroup; 
     }

     void group_number( size_t cgroup ) 
     { 
          m_cgroup = cgroup; 
     }

     void add_item( sub_expr<CI> * pitem )
     {
          *m_pptail = pitem;
          m_pptail = & pitem->next();
     }

     void add_alternate()
     {
          m_rgalternates.push_back( NULL );
          m_pptail = & m_rgalternates.back();
     }

     void end_alternate()
     {
          *m_pptail = _get_end_group();
     }

     size_t calternates() const 
     {
          return m_rgalternates.size();
     }

     width_type group_width()
     {
          (void) match_group_base<CI>::_width_this();
          return m_nwidth;
     }

protected:

     virtual sub_expr<CI> * _get_end_group() = 0;

     virtual width_type _width_this() throw()
     {
          typedef vector<sub_expr<CI>*>::const_iterator VCI;
          if( uninit_width == m_nwidth )
          {
                m_nwidth = width_type(size_t(-1),0);
                for( VCI ialt = m_rgalternates.begin(); worst_width != m_nwidth && ialt != m_rgalternates.end(); ++ialt )
                {
                     width_type temp_width = (*ialt)->get_width();
                     m_nwidth.m_min = min( m_nwidth.m_min, temp_width.m_min );
                     m_nwidth.m_max = max( m_nwidth.m_max, temp_width.m_max );
                }
          }
          return m_nwidth;
     }

     vector<sub_expr<CI>*> m_rgalternates;
     sub_expr<CI>         ** m_pptail; // only used when adding elements
     size_t                     m_cgroup;
     width_type                m_nwidth;
};

template< typename CI >
class end_group_base : public sub_expr<CI>
{
     void * operator new( size_t );
public:
     virtual void _delete() {} // don't delete this, because it was never alloc'ed
protected:
     // since m_pnext is always NULL for end_groups, get_width() stops recursing here
     virtual width_type _width_this() throw() 
     {
          return width_type(0,0); 
     }
};

template< typename CI >
class match_group : public match_group_base<CI>
{
public:
     match_group( size_t cgroup )
          : match_group_base<CI>( cgroup ),
             m_end_group( this ) {}
protected:
     bool _call_back( match_param<CI> & param, CI icur ) const throw()
	{
          CI old_iend = CI();

          if( size_t(-1) != m_cgroup )
          {
                old_iend = (*param.prgbackrefs)[ m_cgroup ].second;
                (*param.prgbackrefs)[ m_cgroup ].second = icur;
          }

          if( match_next( param, icur ) )
                return true;

          if( size_t(-1) != m_cgroup )
                (*param.prgbackrefs)[ m_cgroup ].second = old_iend;

          return false;
	}

     class end_group;
     friend class end_group;
     class end_group : public end_group_base<CI>
     {
          match_group<CI> *const m_pgroup;
     public:
          end_group( match_group<CI> * pgroup )
                : m_pgroup( pgroup ) {}
          virtual bool domatch( match_param<CI> & param, CI icur ) const throw()
          {
			return m_pgroup->_call_back( param, icur );
          }
     } m_end_group;

     virtual sub_expr<CI> * _get_end_group()
     {
          return & m_end_group;
     }
};

// Behaves like a lookahead assertion if m_cgroup is -1, or like
// an independent group otherwise.
template< typename CI >
class independent_group_base : public match_group_base<CI>
{
public:
     independent_group_base() 
          : match_group_base<CI>( size_t(-1) ), m_fexpected(true) {}

     virtual bool domatch( match_param<CI> & param, CI icur ) const throw()
     {
          // Copy the entire backref vector onto the stack
          backref_tag<CI> * prgbr =  (backref_tag<CI>*)_alloca( param.prgbackrefs->size() * sizeof backref_tag<CI> );
          copy( param.prgbackrefs->begin(), param.prgbackrefs->end(), 
                  raw_storage_iterator<backref_tag<CI>*,backref_tag<CI> >(prgbr) );

          // Match until the end of this group and then return
          const bool fdomatch = match_group_base<CI>::domatch( param, icur );

          if( m_fexpected == fdomatch )
          {
                // If m_cgroup != 1, then this is not a zero-width assertion.
                if( size_t(-1) != m_cgroup )
                     icur = (*param.prgbackrefs)[ m_cgroup ].second;

                if( match_next( param, icur ) )
                     return true;
          }

          // if match_group::domatch returned true, the backrefs must be restored
          if( fdomatch )
                copy( prgbr, prgbr + param.prgbackrefs->size(), param.prgbackrefs->begin() );

          return false;
     }

protected:

     independent_group_base( const bool fexpected ) 
          : match_group_base<CI>( size_t(-1) ), m_fexpected(fexpected) {}

     const bool m_fexpected;
};

template< typename CI >
class independent_group : public independent_group_base<CI>
{
public:
     independent_group()
          : independent_group_base<CI>(),
             m_end_group( this ) {}

protected:
     independent_group( const bool fexpected ) 
          : independent_group_base<CI>( fexpected ),
             m_end_group( this ) {}
	
	bool _call_back( match_param<CI> & param, CI icur ) const throw()
	{
          if( size_t(-1) != m_cgroup )
                (*param.prgbackrefs)[ m_cgroup ].second = icur;
          return true;
	}

     class end_group;
     friend class end_group;
     class end_group : public end_group_base<CI>
     {
          independent_group<CI> *const m_pgroup;
     public:
          end_group( independent_group<CI> * pgroup )
                : m_pgroup( pgroup ) {}

          virtual bool domatch( match_param<CI> & param, CI icur ) const throw()
          {
			return m_pgroup->_call_back( param, icur );
          }
     } m_end_group;

     virtual sub_expr<CI> * _get_end_group()
     {
          return & m_end_group;
     }
};

template< typename CI >
class lookahead_assertion : public independent_group<CI>
{
public:
     lookahead_assertion( const bool fexpected )
          : independent_group<CI>( fexpected ) {}
     virtual bool is_assertion() const throw() { return true; }
protected:
     virtual width_type _width_this() throw() { return width_type(0,0); }
};

template< typename CI >
class lookbehind_assertion : public independent_group_base<CI>
{
public:
     lookbehind_assertion( const bool fexpected )
          : independent_group_base<CI>( fexpected ) {}

     virtual bool domatch( match_param<CI> & param, CI icur ) const throw()
     {
          // This is the room in the string from the start to the current position
          size_t room = distance( param.ibegin, icur );

          // If we don't have enough room to match the lookbehind, the match fails.
          // If we wanted the match to fail, try to match the rest of the pattern.
          if( m_nwidth.m_min > room )
                return m_fexpected ? false : match_next( param, icur );

          // Copy the entire backref vector onto the stack
          backref_tag<CI> * prgbr = (backref_tag<CI>*)_alloca( param.prgbackrefs->size() * sizeof backref_tag<CI> );
          copy( param.prgbackrefs->begin(), param.prgbackrefs->end(), 
                  raw_storage_iterator<backref_tag<CI>*,backref_tag<CI> >(prgbr) );

          CI local_istart  = icur;
          advance( local_istart, -int( min( m_nwidth.m_max, room ) ) );

          CI local_istop = icur;
          advance( local_istop, -int( m_nwidth.m_min ) );

          // Create a local param struct that has icur as param.iend
          match_param<CI> local_param(param.ibegin,param.istart,icur,param.prgbackrefs);

          // Find the rightmost match that ends at icur.
          for( CI local_icur = local_istart; local_icur <= local_istop; ++local_icur )
          {
                // Match until the end of this group and then return
                const bool fmatched = match_group_base<CI>::domatch( local_param, local_icur );

                // If the match results were what we were expecting, try to match the
                // rest of the pattern. If that succeeds, return true.
                if( m_fexpected == fmatched && match_next( param, icur ) )
                     return true;

                // if match_group::domatch returned true, the backrefs must be restored
                if( fmatched )
                {
                     copy( prgbr, prgbr + param.prgbackrefs->size(), param.prgbackrefs->begin() );

                     // Match succeeded. If this is a negative lookbehind, we didn't want it
                     // to succeed, so return false.
                     if( ! m_fexpected )
                          return false;
                }
          }

          // No variation of the lookbehind was satisfied in a way that permited
          // the rest of the pattern to match successfully, so return false.
          return false;
     }
     
     virtual bool is_assertion() const throw() 
     { 
          return true; 
     }

protected:

     class end_group;
     friend class end_group;
     class end_group : public end_group_base<CI>
     {
     public:
          virtual bool domatch( match_param<CI> & param, CI icur ) const throw()
          {
                return param.istop == icur;
          }
     } m_end_group;

     virtual sub_expr<CI> * _get_end_group()
     {
          return & m_end_group;
     }

     virtual width_type _width_this() throw() 
     { 
          return width_type(0,0); 
     }
};

template< typename CI >
class match_wrapper : public sub_expr<CI>
{
public:
     match_wrapper( sub_expr<CI> * psub ) 
          : m_psub(psub) {}

     virtual void _delete()
     {
          delete_sub_expr( m_psub );
          sub_expr<CI>::_delete();
     }

protected:

     bool _wrapped_match_this( match_param<CI> & param, CI & icur ) const throw()
     {
          return m_psub->_match_this( param, icur );
     }
     
     virtual width_type _width_this() throw()
     {
          return m_psub->_width_this();
     }

     sub_expr<CI> * m_psub;
};

template< typename CI >
class match_quantifier : public match_wrapper<CI>
{
public:     
     match_quantifier( sub_expr<CI> * psub, size_t lbound, size_t ubound ) 
          : match_wrapper<CI>( psub ), m_lbound(lbound), m_ubound(ubound) {}

protected:

     virtual width_type _width_this() throw()
     {
          width_type this_width = match_wrapper<CI>::_width_this();

          return this_width * width_type( m_lbound, m_ubound );
     }

     const size_t m_lbound;
     const size_t m_ubound;
};

template< typename CI >
class max_atom_quantifier : public match_quantifier<CI>
{
public:
     max_atom_quantifier( sub_expr<CI> * psub, size_t lbound, size_t ubound ) 
          : match_quantifier<CI>( psub, lbound, ubound ) {}

     virtual bool domatch( match_param<CI> & param, CI icur ) const throw()
     {
          size_t cmatches = 0;
          int     cdiff     = 0; // must be a signed quantity for advance() below

          if( cmatches < m_ubound )
          {
                CI istart = icur;
                if( _wrapped_match_this( param, icur ) )
                {
                     ++cmatches;
                     cdiff = distance( istart, icur );
                
                     if( 0 == cdiff )
                          return ( match_next( param, icur ) );

                     while( cmatches < m_ubound && 
                              _wrapped_match_this( param, icur ) )
                     {
                          ++cmatches;
                     }
                }
          }

          if( cmatches >= m_lbound )
          {
                if( ! next() )
                     return true;
     
                for(;;)
                {
                     if( next()->domatch( param, icur ) )
                          return true;
                     
                     if( cmatches-- <= m_lbound )
                          break;
                     
                     advance( icur, -cdiff );
                }
          }

          return false;
     }
};

template< typename CI >
class min_atom_quantifier : public match_quantifier<CI>
{
public:     
     min_atom_quantifier( sub_expr<CI> * psub, size_t lbound, size_t ubound ) 
          : match_quantifier<CI>( psub, lbound, ubound ) {}

     virtual bool domatch( match_param<CI> & param, CI icur ) const throw()
     {
          size_t cmatches = 0;
          bool    fsuccess = true;
          CI icur_tmp = icur;

          if( _wrapped_match_this( param, icur_tmp ) )
          {
                if( icur_tmp == icur )
                     return ( match_next( param, icur ) );

                if( m_lbound )
                {
                     icur = icur_tmp;
                     ++cmatches;
                }
                
                while( ( cmatches < m_lbound ) &&
                         ( fsuccess = _wrapped_match_this( param, icur ) ) )
                {
                     ++cmatches;
                }
          }
          else
          {
                fsuccess = ! m_lbound;
          }

          if( fsuccess && next() )
          {
                do
                {
                     if( next()->domatch( param, icur ) )
                          break;
                } while( fsuccess = ( cmatches++ < m_ubound && 
                                             _wrapped_match_this( param, icur ) ) );
          }

          return fsuccess;
     }
};

template< typename CI >
class end_quantifier_base : public sub_expr<CI>
{
     void * operator new( size_t );
public:
     virtual void _delete() {} // don't delete this, since it wasn't alloc'ed
protected:
     virtual width_type _width_this() throw() { return width_type(0,0); }
};


template< typename CI >
class group_quantifier : public match_quantifier<CI>
{
public:
     
     group_quantifier( match_group_base<CI> * psub,
                       size_t lbound, size_t ubound,
                       end_quantifier_base<CI> * pend_quant ) 
          : match_quantifier<CI>( psub, lbound, ubound ),
	        m_group( *psub ) 
     {
          m_group.next() = pend_quant;
     }

protected:

     size_t group_number() const 
     { 
          return m_group.group_number(); 
     }

     size_t & cmatches( match_param<CI> & param ) const 
     { 
          return (*param.prgbackrefs)[ group_number() ].reserved; 
     }

	match_group_base<CI> & m_group;
};

template< typename CI >
class max_group_quantifier : public group_quantifier<CI>
{
public:
     max_group_quantifier( match_group_base<CI> * psub, size_t lbound, size_t ubound ) 
          : group_quantifier<CI>( psub, lbound, ubound, & m_end_quant ),
		    m_end_quant( this ) {}

     virtual bool domatch( match_param<CI> & param, CI icur ) const throw()
     {
          backref_tag<CI> & br = (*param.prgbackrefs)[ group_number() ];
          backref_tag<CI> old_backref = br;
          br = backref_tag<CI>( icur, icur ); // sets cmatches (reserved) to 0

          if( _recurse( param, icur ) )
                return true;

          br = old_backref;
          return false;
     }

protected:

     bool _recurse( match_param<CI> & param, CI icur ) const throw()
     {
          if( m_ubound == cmatches( param ) )
                return match_next( param, icur );

          ++cmatches( param );
          if( m_psub->domatch( param, icur ) )
                return true;

          if( --cmatches( param ) < m_lbound )
                return false;

          return match_next( param, icur );
     }

     class end_quantifier;
     friend class end_quantifier;
     class end_quantifier : public end_quantifier_base<CI>
     {
          max_group_quantifier<CI> * m_pquant;
     public:
          end_quantifier( max_group_quantifier<CI> * pquant ) 
                : m_pquant( pquant ) {}

          virtual bool domatch( match_param<CI> & param, CI icur ) const throw()
          {
                // handle special the case where a group matches 0 characters
                backref_tag<CI> & br = (*param.prgbackrefs)[ m_pquant->group_number() ];
                if( icur == br.first )
                {
                     size_t old_cmatches = br.reserved;
                     br.reserved = m_pquant->m_ubound;
                     if( m_pquant->_recurse( param, icur ) )
                          return true;
                     br.reserved = old_cmatches;
                     return false;
                }
                return m_pquant->_recurse( param, icur );
          }
     } m_end_quant;
};

template< typename CI >
class min_group_quantifier : public group_quantifier<CI>
{
public:
     min_group_quantifier( match_group_base<CI> * psub, size_t lbound, size_t ubound ) 
          : group_quantifier<CI>( psub, lbound, ubound, & m_end_quant ),
             m_end_quant( this ) {}

     virtual bool domatch( match_param<CI> & param, CI icur ) const throw()
     {
          backref_tag<CI> & br = (*param.prgbackrefs)[ group_number() ];
          backref_tag<CI> old_backref = br;
          br = backref_tag<CI>( icur, icur ); // sets cmatches (reserved) to 0

          if( _recurse( param, icur ) )
                return true;

          br = old_backref;
          return false;
     }

protected:

     bool _recurse( match_param<CI> & param, CI icur ) const throw()
     {
          if( m_lbound > cmatches( param ) )
          {
                ++cmatches( param );
                return m_psub->domatch( param, icur );
          }

          if( match_next( param, icur ) )
                return true;

          if( cmatches( param )++ == m_ubound )
                return false;
                
          return m_psub->domatch( param, icur );
     }

     class end_quantifier;
     friend class end_quantifier;
     class end_quantifier : public end_quantifier_base<CI>
     {
          min_group_quantifier<CI> * m_pquant;
     public:
          end_quantifier( min_group_quantifier<CI> * pquant ) 
                : m_pquant( pquant ) {}

          virtual bool domatch( match_param<CI> & param, CI icur ) const throw()
          {
                // handle special the case where a group matches 0 characters
                backref_tag<CI> & br = (*param.prgbackrefs)[ m_pquant->group_number() ];
                if( icur == br.first )
                {
                     size_t old_cmatches = br.reserved;
                     br.reserved = m_pquant->m_ubound;
                     if( m_pquant->_recurse( param, icur ) )
                          return true;
                     br.reserved = old_cmatches;
                     return false;
                }
                return m_pquant->_recurse( param, icur );
          }
     } m_end_quant;
};

template< typename CI >
class match_backref : public sub_expr<CI>
{
public:
     match_backref( size_t cbackref, const width_type & group_width ) 
          : m_cbackref( cbackref ), m_nwidth(group_width) {}
protected:
     // Return the width specifications of the group to which this backref refers
     virtual width_type _width_this() throw() { return m_nwidth; }
     const size_t m_cbackref;
     const width_type m_nwidth;
};

template< typename CMP, typename EOS >
class match_backref_t : public match_backref<typename EOS::const_iterator>
{
public:
     typedef CMP cmp_type;
     typedef EOS eos_type;
     typedef typename EOS::const_iterator CI;
     match_backref_t( size_t cbackref, const width_type & group_width ) 
          : match_backref<CI>( cbackref, group_width ) {}
     virtual bool domatch( match_param<CI> & param, CI icur ) const throw()
     {
          return ( match_backref_t::_match_this(param,icur) && match_next(param,icur) );
     }
protected:
     virtual bool _match_this( match_param<CI> & param, CI & icur ) const throw()
     {
          CI ithis      = (*param.prgbackrefs)[ m_cbackref ].first;
          CI istop      = (*param.prgbackrefs)[ m_cbackref ].second;
          CI icur_tmp  = icur;

          // Don't match a backref that hasn't match anything
          if( ! (*param.prgbackrefs)[ m_cbackref ] )
                return false;

          for( ; ithis != istop; ++icur_tmp, ++ithis )
          {
                if( m_eos( param, icur_tmp ) || m_cmp( *icur_tmp, *ithis ) )
                     return false;
          }
          icur = icur_tmp;
          return true;
     }
     cmp_type m_cmp;
     eos_type m_eos;
};

template< typename CI >
match_backref<CI> * create_backref( 
     size_t cbackref, 
     const width_type & group_width, 
     unsigned flags ) 
{
     typedef typename iterator_traits<CI>::value_type char_type;

     switch( ( NOCASE | CSTRINGS ) & flags )
     {
     case 0:
          return new match_backref_t<ch_neq_t<char_type>,eos_t<CI> >( cbackref, group_width );
     case NOCASE:
          return new match_backref_t<ch_neq_nocase_t<char_type>,eos_t<CI> >( cbackref, group_width );
     case CSTRINGS:
          return new match_backref_t<ch_neq_t<char_type>,eocs_t<CI> >( cbackref, group_width );
     case NOCASE | CSTRINGS:
          return new match_backref_t<ch_neq_nocase_t<char_type>,eocs_t<CI> >( cbackref, group_width );
     default:
          __assume(0); // tells the compiler that this is unreachable
     }
}

// Replace some escape sequences with the actual characters
// they represent
template< typename CI >
void basic_rpattern_base<CI>::_normalize_string( basic_string<basic_rpattern_base<CI>::char_type> & str )
{
     // Don't do pattern normalization if the user didn't ask for it.
     if( NORMALIZE != ( NORMALIZE & m_flags ) )
          return;

     process_escapes( str );
}

//
// Implementation of basic_rpattern:
//
template< typename CI, typename SY >
basic_rpattern<CI,SY>::basic_rpattern() throw()
     : basic_rpattern_base<CI>( 0 )
{
}

template< typename CI, typename SY >
basic_rpattern<CI,SY>::basic_rpattern( 
     const basic_string<basic_rpattern<CI,SY>::char_type> & pat, 
     unsigned flags ) throw(bad_regexpr,bad_alloc)
     : basic_rpattern_base<CI>( flags, pat )
{
     push_new_handler pnh( &my_new_handler );
     _normalize_string( m_pat );
     _common_init( flags );
}

template< typename CI, typename SY >
basic_rpattern<CI,SY>::basic_rpattern( 
     const basic_string<basic_rpattern<CI,SY>::char_type> & pat, 
     const basic_string<basic_rpattern<CI,SY>::char_type> & subst, 
     unsigned flags ) throw(bad_regexpr,bad_alloc)
     : basic_rpattern_base<CI>( flags, pat, subst )
{
     push_new_handler pnh( &my_new_handler );
     _normalize_string( m_pat );
     _common_init( flags );
     _normalize_string( m_subst );     
     _parse_subst(); // must come after _common_init
}

template< typename CI, typename SY >
void basic_rpattern<CI,SY>::init( 
     const basic_string<basic_rpattern<CI,SY>::char_type> & pat, 
     unsigned flags ) throw(bad_regexpr,bad_alloc)
{
     push_new_handler pnh( &my_new_handler );
     _reset();
     m_flags = flags;
     m_pat    = pat;
     _normalize_string( m_pat );
     _common_init( m_flags );
}

template< typename CI, typename SY >
void basic_rpattern<CI,SY>::init(
     const basic_string<basic_rpattern<CI,SY>::char_type> & pat, 
     const basic_string<basic_rpattern<CI,SY>::char_type> & subst, 
     unsigned flags ) throw(bad_regexpr,bad_alloc)
{
     push_new_handler pnh( &my_new_handler );
     _reset();
     m_flags = flags;
     m_pat    = pat;
     m_subst = subst;
     _normalize_string( m_pat );
     _common_init( m_flags );
     _normalize_string( m_subst );     
     _parse_subst(); // must come after _common_init
}

template< typename CI, typename SY >
void basic_rpattern<CI,SY>::_common_init( unsigned flags )
{
     m_cgroups = 0;
     vector<match_group_base<CI>*> rggroups;
     basic_string<char_type>::iterator ipat = m_pat.begin();
     match_group_base<CI> * pgroup = _find_next_group( ipat, flags, rggroups );

     m_pfirst = pgroup;
     m_nwidth = pgroup->group_width();

     // Number the invisible groups
     m_cgroups_visible = m_cgroups;
     while( ! m_invisible_groups.empty() )
     {
          m_invisible_groups.front()->group_number( _get_next_group_nbr() );
          m_invisible_groups.pop_front();
     }

     //
     // determine if we can get away with only calling m_pfirst->domatch only once
     //

     m_floop = true;

     // Optimization: if first character of pattern string is '^'
     // and we are not doing a multiline match, then we only 
     // need to try domatch once
     basic_string<char_type>::iterator icur = m_pat.begin();
     if( MULTILINE != ( MULTILINE & m_flags ) &&
          1 == pgroup->calternates() && 
          icur != m_pat.end() && 
          BEGIN_LINE == SY::reg_token( icur, m_pat.end() ) )
     {
          m_flags &= ~RIGHTMOST;
          m_floop = false;
     }

     // Optimization: if first 2 characters of pattern string are ".*" or ".+",
     // then we only need to try domatch once
     icur = m_pat.begin();
     if( RIGHTMOST != ( RIGHTMOST & m_flags ) &&
          SINGLELINE == ( SINGLELINE & m_flags ) &&
          1 == pgroup->calternates() &&
          icur != m_pat.end() && 
          MATCH_ANY == SY::reg_token( icur, m_pat.end() ) &&
          icur != m_pat.end() )
     {
          switch( SY::quant_token( icur, m_pat.end() ) )
          {
                case ONE_OR_MORE:
                case ZERO_OR_MORE:
                case ONE_OR_MORE_MIN:
                case ZERO_OR_MORE_MIN:
                     m_floop = false;
          }
     }
}

template< typename CI, typename SY >
void basic_rpattern<CI,SY>::_reset() throw()
{
     basic_rpattern_base<CI>::_reset();

     m_cgroups = m_cgroups_visible = 0;
     m_floop    = true;

     m_subst.erase();
     m_pat.erase();

     m_pfirst.free_ptr();
     m_nwidth = uninit_width;
     m_subst_list.clear();
     m_invisible_groups.clear();
}

template< typename CI, typename SY >
void basic_rpattern<CI,SY>::set_flags( unsigned flags ) throw(bad_regexpr,bad_alloc)
{
     push_new_handler pnh( &my_new_handler );
     m_pfirst.free_ptr();
     m_flags = flags;
     _common_init( m_flags );
}

template< typename CI, typename SY >
void basic_rpattern<CI,SY>::set_substitution( const basic_string<basic_rpattern<CI,SY>::char_type> & subst ) throw(bad_regexpr,bad_alloc)
{
     push_new_handler pnh( &my_new_handler );
     m_subst_list.clear();
     m_subst = subst;
     _normalize_string( m_subst );
     _parse_subst();
}

template< typename CI, typename SY >
match_group_base<CI> * basic_rpattern<CI,SY>::_find_next_group( 
     basic_string<basic_rpattern<CI,SY>::char_type>::iterator & ipat, 
     unsigned & flags,
     vector<match_group_base<CI>*> & rggroups )
{
     auto_sub_ptr<match_group_base<CI> > pgroup;
     basic_string<char_type>::iterator itemp = ipat;
     unsigned old_flags = flags;
     TOKEN tok;

     // Look for group extensions. (This could change the value of the flags variable.)
     if( ipat != m_pat.end() && NO_TOKEN != ( tok = SY::ext_token( ipat, m_pat.end(), flags ) ) )
     {
          if( itemp == m_pat.begin() || ipat == m_pat.end() )
                throw bad_regexpr("ill-formed regular expression");

          // Don't process empty groups
          if( END_GROUP != SY::reg_token( itemp = ipat, m_pat.end() ) )
          {
                switch( tok )
                {
                case EXT_NOBACKREF:
                     // invisible groups are numbered only after all
                     // visible groups have been numbererd
                     pgroup = new match_group<CI>( size_t(-1) );
                     m_invisible_groups.push_back( pgroup.get() );
                     break;

                case EXT_INDEPENDENT:
                     pgroup = new independent_group<CI>();
                     m_invisible_groups.push_back( pgroup.get() );
                     break;

                case EXT_POS_LOOKAHEAD:
                     pgroup = new lookahead_assertion<CI>( true );
                     break;
                
                case EXT_NEG_LOOKAHEAD:
                     pgroup = new lookahead_assertion<CI>( false );
                     break;
                
                case EXT_POS_LOOKBEHIND:
                     // For look-behind assertions, turn off the CSTRINGs optimization
                     flags &= ~CSTRINGS;
                     pgroup = new lookbehind_assertion<CI>( true );
                     break;
                
                case EXT_NEG_LOOKBEHIND:
                     // For look-behind assertions, turn off the CSTRINGs optimization
                     flags &= ~CSTRINGS;
                     pgroup = new lookbehind_assertion<CI>( false );
                     break;
                
                default:
                     throw bad_regexpr("bad extension sequence");
                }
          }
          else
          {
                // Skip over the END_GROUP token
                ipat = itemp;
          }
     }
     else
     {
          pgroup = new match_group<CI>( _get_next_group_nbr() );
     }

     if( NULL != pgroup.get() )
     {
          pgroup->add_alternate();
          while( _find_next( ipat, pgroup.get(), flags, rggroups ) );
          pgroup->end_alternate();

          // Add this group to the rggroups array
          if( size_t(-1) != pgroup->group_number() )
          {
                if( pgroup->group_number() >= rggroups.size() )
                     rggroups.resize( pgroup->group_number() + 1, NULL );
                rggroups[ pgroup->group_number() ] = pgroup.get();
          }

          // The group should calculate its own width now and 
          // save the result for later.
          pgroup->group_width();

          // If this is not a pattern modifier, restore the 
          // flags to their previous settings.  This causes 
          // pattern modifiers to have the scope of their 
          // enclosing group.
          flags = old_flags;
     }

     return pgroup.release();
}

//
// Read ahead through the pattern and treat sequential atoms
// as a single atom, making sure to handle quantification
// correctly. Warning: dense code ahead.
//
template< typename CI, typename SY >
void basic_rpattern<CI,SY>::_find_atom( 
     basic_string<basic_rpattern<CI,SY>::char_type>::iterator & ipat,
     match_group_base<CI> * pgroup, 
     unsigned flags )
{
     basic_string<char_type>::iterator itemp = ipat, istart = ipat;

     do
     {
          switch( SY::quant_token( itemp, m_pat.end() ) )
          {
          // if {,} can't be interpreted as quantifiers, treat them as regular chars
          case BEGIN_RANGE:
                if( istart != ipat ) // treat as a quantifier
                     goto quantify;
          case NO_TOKEN:
          case END_RANGE:
          case END_RANGE_MIN:
          case RANGE_SEPARATOR:
                break;

          default: 
                if( istart == ipat ) // must be able to quantify something.
                     throw bad_regexpr("quantifier not expected");
quantify:    if( istart != --ipat )
                     pgroup->add_item( create_atom<CI>( istart, ipat, flags ) );
                auto_sub_ptr<sub_expr<CI> > pnew( create_atom<CI>( ipat++, flags ) );
                _quantify( pnew, NULL, ipat );
                pgroup->add_item( pnew.release() );
                return;
          }
     } while( m_pat.end() != ++ipat && ! SY::reg_token( itemp = ipat, m_pat.end() ) );

     assert( ipat != istart );
     pgroup->add_item( create_atom<CI>( istart, ipat, flags ) );
}

template< typename CI, typename SY >
bool basic_rpattern<CI,SY>::_find_next( 
     basic_string<basic_rpattern<CI,SY>::char_type>::iterator & ipat,
     match_group_base<CI> * pgroup, 
     unsigned & flags,
     vector<match_group_base<CI>*> & rggroups )
{
     match_group_base<CI> * pnew_group = NULL;
     auto_sub_ptr<sub_expr<CI> > pnew;
     basic_string<char_type>::iterator istart, itemp;
     bool fdone;

     if( ipat == m_pat.end() )
     {
          if( 0 != pgroup->group_number() )
                throw bad_regexpr( "mismatched parenthesis" );
          return false;
     }

     switch( SY::reg_token( ipat, m_pat.end() ) )
     {
     case NO_TOKEN: // not a token. Must be an atom
          _find_atom( ipat, pgroup, flags );
          return true;
     
     case END_GROUP:
          if( 0 == pgroup->group_number() )
                throw bad_regexpr( "mismatched parenthesis" );
          return false;

     case ALTERNATION:
          pgroup->end_alternate();
          pgroup->add_alternate();
          return true;
     
     case BEGIN_GROUP:
          // Find next group could return NULL if the group is really
          // a pattern modifier, like: (?s-i)
          pnew = pnew_group = _find_next_group( ipat, flags, rggroups );
          break;

     case BEGIN_LINE:
          pnew = create_bol<CI>( flags );
          break;

     case END_LINE:
          pnew = create_eol<CI>( flags );
          break;

     case BEGIN_CHARSET:
          pnew = create_charset_helper<CI,SY>::create_charset_aux( m_pat, ipat, flags );
          break;

     case MATCH_ANY:
          pnew = create_any<CI>( flags );
          break;

     case ESC_WORD_BOUNDARY:
          pnew = create_word_boundary<CI>( true, flags );
          break;
     
     case ESC_NOT_WORD_BOUNDARY:
          pnew = create_word_boundary<CI>( false, flags );
          break;
     
     case ESC_WORD_START:
          pnew = create_word_start<CI>( flags );
          break;
     
     case ESC_WORD_STOP:
          pnew = create_word_stop<CI>( flags );
          break;
     
     case ESC_DIGIT:
          pnew = create_charset<CI>( match_charset<CI>( false,
                                                                     get_digit_vector() ),
                                                                     flags );
          break;

     case ESC_NOT_DIGIT:
          pnew = create_charset<CI>( match_charset<CI>( true,
                                                                     get_digit_vector() ),
                                                                     flags );
          break;

     case ESC_WORD:
          pnew = create_charset<CI>( match_charset<CI>( false,
                                                                     get_word_vector() ),
                                                                     flags );
          break;

     case ESC_NOT_WORD:
          pnew = create_charset<CI>( match_charset<CI>( true,
                                                                     get_word_vector() ),
                                                                     flags );
          break;

     case ESC_SPACE:
          pnew = create_charset<CI>( match_charset<CI>( false,
                                                                     get_space_vector() ),
                                                                     flags );
          break;

     case ESC_NOT_SPACE:
          pnew = create_charset<CI>( match_charset<CI>( true,
                                                                     get_space_vector() ),
                                                                     flags );
          break;

     case ESC_BEGIN_STRING:
          pnew = create_bos<CI>( flags );
          break;

     case ESC_END_STRING:
          pnew = create_eos<CI>( flags );
          break;

     case ESC_END_STRING_z:
          pnew = create_eoz<CI>( flags );
          break;

     case ESCAPE:

          if( char_type('0') <= *ipat && char_type('9') >= *ipat )
          {
                // use _cgroups_total here since the invisible groups have not been numbered yet.
                unsigned nbackref = parse_int( ipat, m_pat.end(), _cgroups_total() - 1 );// always at least 1 group
                if( 0 == nbackref || rggroups.size() <= nbackref || NULL == rggroups[ nbackref ] )
                     throw bad_regexpr( "invalid backreference" );
                pnew = create_backref<CI>( nbackref, rggroups[nbackref]->group_width(), flags );
          }
          else
          {
                // Is this a user-defined intrinsic character set?
                match_charset<CI> * pcharset = s_charset_map.get( *ipat, flags );
                if( NULL != pcharset )
                     pnew = create_charset<CI>( *pcharset, flags );
                else
                     pnew = create_atom<CI>( ipat, flags );
                ++ipat;
          }
          break;

     // If quotemeta, loop until we find quotemeta off or end of string
     case ESC_QUOTE_META_ON:
          for( istart = itemp = ipat, fdone = false; !fdone && ipat != m_pat.end(); )
          {
                switch( SY::reg_token( ipat, m_pat.end() ) )
                {
                case ESC_QUOTE_META_OFF:
                     fdone = true;
                     break;
                case NO_TOKEN:
                     ++ipat; // fallthrough
                default:
                     itemp = ipat;
                     break;
                }
          }
          if( itemp != istart )
                pgroup->add_item( create_atom<CI>( istart, itemp, flags ) );

          // skip the quantification code below
          return true;

     // Should never get here for valid patterns
     case ESC_QUOTE_META_OFF:
          throw bad_regexpr("quotemeta turned off, but was never turned on");

     default:
          assert( ! "Unhandled token type" );
          break;
     }
     
     // If pnew is null, then the current subexpression is a no-op.
     if( pnew.get() )
     {
          // Look for quantifiers
          _quantify( pnew, pnew_group, ipat );

          // Add the item to the group
          pgroup->add_item( pnew.release() );
     }
     return true;
}

template< typename CI, typename SY >
void basic_rpattern<CI,SY>::_quantify( 
     auto_sub_ptr<sub_expr<CI> > & pnew,
     match_group_base<CI> * pnew_group,
     basic_string<basic_rpattern<CI,SY>::char_type>::iterator & ipat )
{
     if( ipat != m_pat.end() && ! pnew->is_assertion() )
     {
          basic_string<char_type>::iterator itemp = ipat;
          bool fmin = false;
          
          // Since size_t is unsigned, -1 is really the largest size_t
          size_t lbound = (size_t)-1;
          size_t ubound = (size_t)-1;
          size_t ubound_tmp;

          switch( SY::quant_token( itemp, m_pat.end() ) )
          {
          case ZERO_OR_MORE_MIN:
                fmin = true;
          case ZERO_OR_MORE:
                lbound = 0;
                break;

          case ONE_OR_MORE_MIN:
                fmin = true;
          case ONE_OR_MORE:
                lbound = 1;
                break;

          case ZERO_OR_ONE_MIN:
                fmin = true;
          case ZERO_OR_ONE:
                lbound = 0;
                ubound = 1; 
                break;

          case BEGIN_RANGE:
                lbound = parse_int( itemp, m_pat.end() );
                if( itemp == m_pat.end() )
                     throw bad_regexpr( "expecting end of range" );

                switch( SY::quant_token( itemp, m_pat.end() ) )
                {
                case END_RANGE_MIN:
                     fmin = true;
                case END_RANGE:
                     ubound = lbound;
                     break;

                case RANGE_SEPARATOR:
                     ipat = itemp;
                     ubound_tmp = parse_int( itemp, m_pat.end() );
                     if( itemp != ipat )
                          ubound = ubound_tmp;
                     if( itemp == m_pat.end() )
                          throw bad_regexpr( "expecting end of range" );
                     switch( SY::quant_token( itemp, m_pat.end() ) )
                     {
                     case END_RANGE_MIN:
                          fmin = true;
                     case END_RANGE:
                          break;
                     default:
                          throw bad_regexpr( "expecting end of range" );
                     }
                     break;

                default:
                     throw bad_regexpr( "ill-formed quantifier" );
                }

                if( ubound < lbound  )
                     throw bad_regexpr( "ill-formed quantifier" );

                break;
          }

          if( (size_t)-1 != lbound )
          {
                auto_sub_ptr<match_quantifier<CI> > pquant;

                // a group quantifier is less efficient than an atom quantifier
                if( fmin )
                {
                     if( pnew_group )
                          pquant = new min_group_quantifier<CI>( pnew_group, 
                                                                                      lbound, ubound );
                     else
                          pquant = new min_atom_quantifier<CI>( pnew.get(), 
                                                                                     lbound, ubound );
                }
                else
                {
                     if( pnew_group )
                          pquant = new max_group_quantifier<CI>( pnew_group, 
                                                                                      lbound, ubound );
                     else
                          pquant = new max_atom_quantifier<CI>( pnew.get(),
                                                                                     lbound, ubound );
                }

                pnew.release();
                pnew = pquant.release();
                ipat = itemp;
          }
     }
}

template< typename CI, typename SY >
void basic_rpattern<CI,SY>::_add_subst_backref( subst_node & snode, size_t nbackref, size_t rstart )
{
     m_fuses_backrefs = true;
     assert( subst_node::SUBST_STRING == snode.stype );
     if( snode.subst_string.rlength )
          m_subst_list.push_back( snode );

     snode.stype = subst_node::SUBST_BACKREF;
     snode.subst_backref = nbackref;
     m_subst_list.push_back( snode );

     // re-initialize the subst_node
     snode.stype = subst_node::SUBST_STRING;
     snode.subst_string.rstart = rstart;
     snode.subst_string.rlength = 0;
}

template< typename CI, typename SY >
void basic_rpattern<CI,SY>::_parse_subst()
{
     TOKEN tok;
     subst_node snode;
     basic_string<char_type>::iterator icur = m_subst.begin();
     size_t nbackref;
     basic_string<char_type>::iterator itemp;
     bool fdone;

     m_fuses_backrefs = false;

     // Initialize the subst_node
     snode.stype = subst_node::SUBST_STRING;
     snode.subst_string.rstart = 0;
     snode.subst_string.rlength = 0;

     while( icur != m_subst.end() )
     {
          switch( tok = SY::subst_token( icur, m_subst.end() ) )
          {
          case SUBST_MATCH:
                _add_subst_backref( snode, 0, distance( m_subst.begin(), icur ) );
                break;

          case SUBST_PREMATCH:
                _add_subst_backref( snode, (size_t)subst_node::PREMATCH, distance( m_subst.begin(), icur ) );
                break;

          case SUBST_POSTMATCH:
                _add_subst_backref( snode, (size_t)subst_node::POSTMATCH, distance( m_subst.begin(), icur ) );
                break;

          case SUBST_BACKREF:
                nbackref = parse_int( icur, m_subst.end(), cgroups() - 1 ); // always at least 1 group
                if( 0 == nbackref )
                     throw bad_regexpr( "invalid backreference in substitution" );

                _add_subst_backref( snode, nbackref, distance( m_subst.begin(), icur ) );
                break;

          case SUBST_QUOTE_META_ON:
                assert( subst_node::SUBST_STRING == snode.stype );
                if( snode.subst_string.rlength )
                     m_subst_list.push_back( snode );

                snode.subst_string.rstart = distance( m_subst.begin(), icur );
                for( itemp = icur, fdone = false; !fdone && icur != m_subst.end(); )
                {
                     switch( tok = SY::subst_token( icur, m_subst.end() ) )
                     {
                     case SUBST_ALL_OFF:
                          fdone = true;
                          break;
                     case NO_TOKEN:
                          ++icur; // fall-through
                     default:
                          itemp = icur;
                          break;
                     }
                }
                snode.subst_string.rlength = distance( m_subst.begin(), itemp ) - snode.subst_string.rstart;
                if( snode.subst_string.rlength )
                     m_subst_list.push_back( snode );

                if( tok == SUBST_ALL_OFF )
                {
                     snode.stype = subst_node::SUBST_OP;
                     snode.op     = subst_node::ALL_OFF;
                     m_subst_list.push_back( snode );
                }

                // re-initialize the subst_node
                snode.stype = subst_node::SUBST_STRING;
                snode.subst_string.rstart = distance( m_subst.begin(), icur );
                snode.subst_string.rlength = 0;
                break;

          case SUBST_UPPER_ON:
          case SUBST_UPPER_NEXT:
          case SUBST_LOWER_ON:
          case SUBST_LOWER_NEXT:
          case SUBST_ALL_OFF:
                assert( subst_node::SUBST_STRING == snode.stype );
                if( snode.subst_string.rlength )
                     m_subst_list.push_back( snode );

                snode.stype = subst_node::SUBST_OP;
                snode.op     = (subst_node::op_type) tok;
                m_subst_list.push_back( snode );

                // re-initialize the subst_node
                snode.stype = subst_node::SUBST_STRING;
                snode.subst_string.rstart = distance( m_subst.begin(), icur );
                snode.subst_string.rlength = 0;
                break;
          
          case SUBST_ESCAPE:
                if( icur == m_subst.end() )
                     throw bad_regexpr("expecting escape sequence in substitution string");
                assert( subst_node::SUBST_STRING == snode.stype );
                if( snode.subst_string.rlength )
                     m_subst_list.push_back( snode );
                snode.subst_string.rstart = distance( m_subst.begin(), icur++ );
                snode.subst_string.rlength = 1;
                break;

          case NO_TOKEN:
          default:
                ++snode.subst_string.rlength;
                ++icur;
                break;
          }
     }
     assert( subst_node::SUBST_STRING == snode.stype );
     if( snode.subst_string.rlength )
          m_subst_list.push_back( snode );
}

template< typename CI, typename SY >
basic_rpattern<CI,SY>::charset_map basic_rpattern<CI,SY>::s_charset_map;

// Pass in an interator to one after the opening bracket of the character set.
// On return, icur points to one character after the closing bracket
template< typename CI, typename SY >
sub_expr<CI> * create_charset_helper<CI,SY>::create_charset_aux(
     basic_string<iterator_traits<CI>::value_type> & str,
     basic_string<iterator_traits<CI>::value_type>::iterator & icur,
     unsigned flags )
{
     bool fcomplement = false;
     match_charset<CI> * pnew = NULL;
     basic_string<iterator_traits<CI>::value_type>::iterator itemp = icur;

     if( itemp != str.end() && CHARSET_NEGATE == SY::charset_token( itemp, str.end() ) )
     {
          fcomplement = true;
          icur = itemp;
     }

     switch( ( NOCASE | CSTRINGS ) & flags )
     {
     case 0:
          pnew = new match_custom_charset_t<eos_t<CI>,match_range_with_case>( fcomplement, icur, str.end(), flags, SY() );
          break;
     case NOCASE:
          pnew = new match_custom_charset_t<eos_t<CI>,match_range_no_case>( fcomplement, icur, str.end(), flags, SY() );
          break;
     case CSTRINGS:
          pnew = new match_custom_charset_t<eocs_t<CI>,match_range_with_case>( fcomplement, icur, str.end(), flags, SY() );
          break;
     case NOCASE | CSTRINGS:
          pnew = new match_custom_charset_t<eocs_t<CI>,match_range_no_case>( fcomplement, icur, str.end(), flags, SY() );
          break;
     default:
          __assume(0); // tells the compiler that this is unreachable
     }

     return pnew;
}

#pragma warning( disable : 4660 )

// Explicit instantiation
#ifdef REGEX_FORCE_INSTANTIATION
 template class basic_regexpr<char>;
 template class basic_regexpr<wchar_t>;
#else
 template class basic_regexpr<TCHAR>;
#endif

#ifndef NO_PERL_RE
 #ifdef REGEX_FORCE_INSTANTIATION
  template class basic_rpattern<const char *>;
  template class basic_rpattern<const wchar_t *>;
  template class basic_rpattern<string::const_iterator>;
  template class basic_rpattern<wstring::const_iterator>;
 #else
  template class basic_rpattern<const TCHAR *>;
  template class basic_rpattern<tstring::const_iterator>;
 #endif
#endif

#ifdef POSIX_RE
 #ifdef REGEX_FORCE_INSTANTIATION
  template class basic_rpattern<const char *,posix_syntax<char> >;
  template class basic_rpattern<const wchar_t *,posix_syntax<wchar_t> >;
  template class basic_rpattern<string::const_iterator,posix_syntax<char> >;
  template class basic_rpattern<wstring::const_iterator,posix_syntax<wchar_t> >;
 #else
  template class basic_rpattern<const TCHAR *,posix_syntax<TCHAR> >;
  template class basic_rpattern<tstring::const_iterator,posix_syntax<TCHAR> >;
 #endif
#endif

} // namespace regex

