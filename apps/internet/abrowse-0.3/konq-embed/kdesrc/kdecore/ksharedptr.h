/* This file is part of the KDE libraries
   Copyright (c) 1999 Waldo Bastian <bastian@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#ifndef KSharedPTR_H
#define KSharedPTR_H

/**
 * Reference counting for shared objects.  If you derive your object
 * from this class, then you may use it in conjunction with
 * @ref KSharedPtr to control the lifetime of your object.
 *
 * Specifically, all classes that derive from KShared have an internal
 * counter keeping track of how many other objects have a reference to
 * their object.  If used with @ref KSharedPtr, then your object will
 * not be deleted until all references to the object have been
 * released.
 *
 * You should probably not ever use any of the methods in this class
 * directly -- let the @ref KSharedPtr take care of that.  Just derive
 * your class from KShared and forget about it.
 *
 * @author Waldo Bastian <bastian@kde.org>
 * @version $Id$
 */
class KShared {
public:
   /**
    * Standard constructor.  This will initialize the reference count
    * on this object to 0
    */
   KShared() : count(0) { }

   /**
    * Copy constructor.  This will @em not actually copy the objects
    * but it will initialize the reference count on this object to 0
    */
   KShared( const KShared & ) : count(0) { }

   /**
    * Overloaded assignment operator
    */
   KShared &operator=(const KShared & ) { return *this; }

   /**
    * Increases the reference count by one
    */
   void _KShared_ref() { count++; }

   /**
    * Releases a reference (decreases the reference count by one).  If
    * the count goes to 0, this object will delete itself
    */
   void _KShared_unref() { if (!--count) delete this; }

   /**
    * Return the current number of references held
    *
    * @return Number of references
    */
   int _KShared_count() { return count; }

protected:
   virtual ~KShared() { }
   int count;
};

/**
 * Can be used to control the lifetime of an object that has derived
 * @ref KShared. As long a someone holds a KSharedPtr on some KShared
 * object it won't become deleted but is deleted once its reference
 * count is 0.  This struct emulates C++ pointers perfectly. So just
 * use it like a simple C++ pointer.
 *
 * KShared and KSharedPtr are preferred over QShared / QSharedPtr
 * since they are more safe.
 *
 * @author Waldo Bastian <bastian@kde.org>
 * @version $Id$
 */
template< class T >
struct KSharedPtr
{
public:
  KSharedPtr()
    : ptr(0) { }
  KSharedPtr( T* t )
    : ptr(t) { if ( ptr ) ptr->_KShared_ref(); }
  KSharedPtr( const KSharedPtr& p )
    : ptr(p.ptr) { if ( ptr ) ptr->_KShared_ref(); }

  ~KSharedPtr() { if ( ptr ) ptr->_KShared_unref(); }

  KSharedPtr<T>& operator= ( const KSharedPtr<T>& p ) {
    if ( ptr == p.ptr ) return *this;
    if ( ptr ) ptr->_KShared_unref();
    ptr = p.ptr;
    if ( ptr ) ptr->_KShared_ref();
    return *this;
  }
  KSharedPtr<T>& operator= ( T* p ) {
    if ( ptr == p ) return *this;
    if ( ptr ) ptr->_KShared_unref();
    ptr = p;
    if ( ptr ) ptr->_KShared_ref();
    return *this;
  }
  bool operator== ( const KSharedPtr<T>& p ) const { return ( ptr == p.ptr ); }
  bool operator!= ( const KSharedPtr<T>& p ) const { return ( ptr != p.ptr ); }
  bool operator== ( const T* p ) const { return ( ptr == p ); }
  bool operator!= ( const T* p ) const { return ( ptr != p ); }
  bool operator!() const { return ( ptr == 0 ); }
  operator T*() const { return ptr; }

  T* data() { return ptr; }
  const T* data() const { return ptr; }

  const T& operator*() const { return *ptr; }
  T& operator*() { return *ptr; }
  const T* operator->() const { return ptr; }
  T* operator->() { return ptr; }

  int count() const { return ptr->_KShared_count(); } // for debugging purposes
private:
  T* ptr;
};

#endif
