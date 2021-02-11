#pragma once

#include<map>

/*
  A priority queue in which the priorities of entries may be
  updated in lg(n) time.

  Simply two maps glued together, one mapping keys (priorities)
  to values and the other mapping values to keys.  Both keys
  and values must meet the requirements for storage in a
  std::map, i.e. be less than comparable.  Typically not a
  problem for priorities, but can be a pain if Values are stored
  by value in the structures.  Not a problem if pointers to
  values are stored.

  insert, erase and update operations are all logarithmic, but
  approximately 2x slower than std::map since two std::maps
  are updated in each case.
*/
template< typename _Key, typename _Value, typename _KeyComp=std::greater<_Key>, typename _ValueComp=std::less<_Value> >
class PriorityQueue {
public:
  // iterator type for the key2val map, allows in-order traversal of the queue
  typedef typename std::multimap<_Key,_Value,_KeyComp>::const_iterator iterator;
  typedef typename std::multimap<_Key,_Value,_KeyComp>::const_iterator const_iterator;

  // default constructor, uses std::greater and std::less for key and value comparisons
  PriorityQueue() : key2Val(_KeyComp()), val2Key(_ValueComp()) { }

  // constructor with key comparison predicate
  PriorityQueue( const _KeyComp &keyComp ) : key2Val(keyComp), val2Key(_ValueComp()){ }

  // constructor with value comparison predicate
  PriorityQueue( const _ValueComp &valComp ) : key2Val(_KeyComp()), val2Key(valComp) { }

  // constructor with both comparison predicates
  PriorityQueue( const _KeyComp &keyComp, const _ValueComp &valComp ) : key2Val(keyComp), val2Key(valComp) { }

  // copy constructor
  PriorityQueue( const PriorityQueue<_Key,_Value,_KeyComp,_ValueComp> &x ) : key2Val(x.key2Val), val2Key(x.val2Key) {}

  // destructor, clears both maps
  ~PriorityQueue(){
    key2Val.clear();
    val2Key.clear();
  }

  // assignment operator
  inline void operator=( PriorityQueue<_Key,_Value,_KeyComp,_ValueComp> x ){
    key2Val = x.key2Val;
    val2Key = x.val2Key;
  }

  // returns an iterator for the front of the queue
  inline iterator begin(){
    return key2Val.begin();
  }

  // returns an iterator for the end of the queue
  inline iterator end(){
    return key2Val.end();
  }

  // empties the queue
  inline void clear(){
    key2Val.clear();
    val2Key.clear();
  }

  // removes from both maps the value val
  inline void erase( _Value val ){
    typename std::map<_Value,_Key,_ValueComp>::iterator iter=val2Key.find( val );
    if( iter != val2Key.end() ){
      key2Val.erase( _find(iter->second,iter->first) );
      val2Key.erase( iter );
    }
  }

  // returns true if the queue is empty
  inline bool empty(){
    return key2Val.empty();
  }

  // returns the value at the front of the queue
  inline _Value front_value(){
    return (*key2Val.begin())->second;
  }

  // returns the key at the front of the queue
  inline _Value front_key(){
    return (*key2Val.begin())->first;
  }

  // removes the front entry in the queue
  inline void pop(){
    iterator iter = key2Val.begin();
    _erase( iter );
  }

  // push an entry onto the queue
  inline void push( _Value val, _Key key ){
    insert( val, key );
  }

  // adds a value key pair to the queue.
  // an attempt first has to be made to erase
  // the item, since it could exist with another
  // key and the queue does not allow multiple
  // keys per value
  inline void insert( _Value val, _Key key ){
    erase( val );
    key2Val.insert( std::pair<_Key,_Value>( key, val ) );
    val2Key.insert( std::pair<_Value,_Key>( val, key ) );
  }

  // update the key associated with a value,
  // simply a convenience function that calls
  // insert()
  inline void update( _Value val, _Key key ){
    insert( val, key );
  }

  // returns the number of elements in the queue
  inline size_t size(){
    return key2Val.size();
  }

private:
  std::multimap< _Key, _Value, _KeyComp >  key2Val;
  std::map< _Value, _Key, _ValueComp >  val2Key;

  inline void _erase( typename std::multimap<_Key,_Value,_KeyComp>::iterator it ){
    val2Key.erase( it->second );
    key2Val.erase( it );
  }

  inline typename std::multimap<_Key,_Value,_KeyComp>::iterator _find( _Key k, _Value v ){
    typename std::multimap<_Key,_Value,_KeyComp>::iterator it = key2Val.lower_bound(k);
    typename std::multimap<_Key,_Value,_KeyComp>::iterator hi = key2Val.upper_bound(k);
    while( it != hi ){
      if( it->second == v )
        return it;
      it++;
    }
    return key2Val.end();
  }
};
