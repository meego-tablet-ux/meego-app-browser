// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TUPLE_H__
#define BASE_TUPLE_H__

// Traits ----------------------------------------------------------------------
//
// A simple traits class for tuple arguments.
//
// ValueType: the bare, nonref version of a type (same as the type for nonrefs).
// RefType: the ref version of a type (same as the type for refs).
// ParamType: what type to pass to functions (refs should not be constified).

template <class P>
struct TupleTraits {
  typedef P ValueType;
  typedef P& RefType;
  typedef const P& ParamType;
};

template <class P>
struct TupleTraits<P&> {
  typedef P ValueType;
  typedef P& RefType;
  typedef P& ParamType;
};

// Tuple -----------------------------------------------------------------------
//
// This set of classes is useful for bundling 0 or more heterogeneous data types
// into a single variable.  The advantage of this is that it greatly simplifies
// function objects that need to take an arbitrary number of parameters; see
// RunnableMethod and IPC::MessageWithTuple.
//
// Tuple0 is supplied to act as a 'void' type.  It can be used, for example,
// when dispatching to a function that accepts no arguments (see the
// Dispatchers below).
// Tuple1<A> is rarely useful.  One such use is when A is non-const ref that you
// want filled by the dispatchee, and the tuple is merely a container for that
// output (a "tier").  See MakeRefTuple and its usages.

struct Tuple0 {
  typedef Tuple0 ValueTuple;
  typedef Tuple0 RefTuple;
};

template <class A>
struct Tuple1 {
 public:
  typedef A TypeA;
  typedef Tuple1<typename TupleTraits<A>::ValueType> ValueTuple;
  typedef Tuple1<typename TupleTraits<A>::RefType> RefTuple;

  Tuple1() {}
  explicit Tuple1(typename TupleTraits<A>::ParamType a) : a(a) {}

  A a;
};

template <class A, class B>
struct Tuple2 {
 public:
  typedef A TypeA;
  typedef B TypeB;
  typedef Tuple2<typename TupleTraits<A>::ValueType,
                 typename TupleTraits<B>::ValueType> ValueTuple;
  typedef Tuple2<typename TupleTraits<A>::RefType,
                 typename TupleTraits<B>::RefType> RefTuple;

  Tuple2() {}
  Tuple2(typename TupleTraits<A>::ParamType a,
         typename TupleTraits<B>::ParamType b)
      : a(a), b(b) {
  }

  A a;
  B b;
};

template <class A, class B, class C>
struct Tuple3 {
 public:
  typedef A TypeA;
  typedef B TypeB;
  typedef C TypeC;
  typedef Tuple3<typename TupleTraits<A>::ValueType,
                 typename TupleTraits<B>::ValueType,
                 typename TupleTraits<C>::ValueType> ValueTuple;
  typedef Tuple3<typename TupleTraits<A>::RefType,
                 typename TupleTraits<B>::RefType,
                 typename TupleTraits<C>::RefType> RefTuple;

  Tuple3() {}
  Tuple3(typename TupleTraits<A>::ParamType a,
         typename TupleTraits<B>::ParamType b,
         typename TupleTraits<C>::ParamType c)
      : a(a), b(b), c(c){
  }

  A a;
  B b;
  C c;
};

template <class A, class B, class C, class D>
struct Tuple4 {
 public:
  typedef A TypeA;
  typedef B TypeB;
  typedef C TypeC;
  typedef D TypeD;
  typedef Tuple4<typename TupleTraits<A>::ValueType,
                 typename TupleTraits<B>::ValueType,
                 typename TupleTraits<C>::ValueType,
                 typename TupleTraits<D>::ValueType> ValueTuple;
  typedef Tuple4<typename TupleTraits<A>::RefType,
                 typename TupleTraits<B>::RefType,
                 typename TupleTraits<C>::RefType,
                 typename TupleTraits<D>::RefType> RefTuple;

  Tuple4() {}
  Tuple4(typename TupleTraits<A>::ParamType a,
         typename TupleTraits<B>::ParamType b,
         typename TupleTraits<C>::ParamType c,
         typename TupleTraits<D>::ParamType d)
      : a(a), b(b), c(c), d(d) {
  }

  A a;
  B b;
  C c;
  D d;
};

template <class A, class B, class C, class D, class E>
struct Tuple5 {
public:
  typedef A TypeA;
  typedef B TypeB;
  typedef C TypeC;
  typedef D TypeD;
  typedef E TypeE;
  typedef Tuple5<typename TupleTraits<A>::ValueType,
    typename TupleTraits<B>::ValueType,
    typename TupleTraits<C>::ValueType,
    typename TupleTraits<D>::ValueType,
    typename TupleTraits<E>::ValueType> ValueTuple;
  typedef Tuple5<typename TupleTraits<A>::RefType,
    typename TupleTraits<B>::RefType,
    typename TupleTraits<C>::RefType,
    typename TupleTraits<D>::RefType,
    typename TupleTraits<E>::RefType> RefTuple;

  Tuple5() {}
  Tuple5(typename TupleTraits<A>::ParamType a,
    typename TupleTraits<B>::ParamType b,
    typename TupleTraits<C>::ParamType c,
    typename TupleTraits<D>::ParamType d,
    typename TupleTraits<E>::ParamType e)
    : a(a), b(b), c(c), d(d), e(e) {
  }

  A a;
  B b;
  C c;
  D d;
  E e;
};

// Tuple creators -------------------------------------------------------------
//
// Helper functions for constructing tuples while inferring the template
// argument types.

inline Tuple0 MakeTuple() {
  return Tuple0();
}

template <class A>
inline Tuple1<A> MakeTuple(const A& a) {
  return Tuple1<A>(a);
}

template <class A, class B>
inline Tuple2<A, B> MakeTuple(const A& a, const B& b) {
  return Tuple2<A, B>(a, b);
}

template <class A, class B, class C>
inline Tuple3<A, B, C> MakeTuple(const A& a, const B& b, const C& c) {
  return Tuple3<A, B, C>(a, b, c);
}

template <class A, class B, class C, class D>
inline Tuple4<A, B, C, D> MakeTuple(const A& a, const B& b, const C& c,
                                    const D& d) {
  return Tuple4<A, B, C, D>(a, b, c, d);
}

template <class A, class B, class C, class D, class E>
inline Tuple5<A, B, C, D, E> MakeTuple(const A& a, const B& b, const C& c,
                                       const D& d, const E& e) {
  return Tuple5<A, B, C, D, E>(a, b, c, d, e);
}

// The following set of helpers make what Boost refers to as "Tiers" - a tuple
// of references.

template <class A>
inline Tuple1<A&> MakeRefTuple(A& a) {
  return Tuple1<A&>(a);
}

template <class A, class B>
inline Tuple2<A&, B&> MakeRefTuple(A& a, B& b) {
  return Tuple2<A&, B&>(a, b);
}

template <class A, class B, class C>
inline Tuple3<A&, B&, C&> MakeRefTuple(A& a, B& b, C& c) {
  return Tuple3<A&, B&, C&>(a, b, c);
}

template <class A, class B, class C, class D>
inline Tuple4<A&, B&, C&, D&> MakeRefTuple(A& a, B& b, C& c, D& d) {
  return Tuple4<A&, B&, C&, D&>(a, b, c, d);
}

template <class A, class B, class C, class D, class E>
inline Tuple5<A&, B&, C&, D&, E&> MakeRefTuple(A& a, B& b, C& c, D& d, E& e) {
  return Tuple5<A&, B&, C&, D&, E&>(a, b, c, d, e);
}

// Dispatchers ----------------------------------------------------------------
//
// Helper functions that call the given method on an object, with the unpacked
// tuple arguments.  Notice that they all have the same number of arguments,
// so you need only write:
//   DispatchToMethod(object, &Object::method, args);
// This is very useful for templated dispatchers, since they don't need to know
// what type |args| is.

// Non-Static Dispatchers with no out params.

template <class ObjT, class Method>
inline void DispatchToMethod(ObjT* obj, Method method, const Tuple0& arg) {
  (obj->*method)();
}

template <class ObjT, class Method, class A>
inline void DispatchToMethod(ObjT* obj, Method method, const A& arg) {
  (obj->*method)(arg);
}

template <class ObjT, class Method, class A>
inline void DispatchToMethod(ObjT* obj, Method method, const Tuple1<A>& arg) {
  (obj->*method)(arg.a);
}

template<class ObjT, class Method, class A, class B>
inline void DispatchToMethod(ObjT* obj, Method method, const Tuple2<A, B>& arg) {
  (obj->*method)(arg.a, arg.b);
}

template<class ObjT, class Method, class A, class B, class C>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<A, B, C>& arg) {
  (obj->*method)(arg.a, arg.b, arg.c);
}

template<class ObjT, class Method, class A, class B, class C, class D>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<A, B, C, D>& arg) {
  (obj->*method)(arg.a, arg.b, arg.c, arg.d);
}

template<class ObjT, class Method, class A, class B, class C, class D, class E>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<A, B, C, D, E>& arg) {
  (obj->*method)(arg.a, arg.b, arg.c, arg.d, arg.e);
}

// Static Dispatchers with no out params.

template <class Function>
inline void DispatchToFunction(Function function, const Tuple0& arg) {
  (*function)();
}

template <class Function, class A>
inline void DispatchToFunction(Function function, const A& arg) {
  (*function)(arg);
}

template <class Function, class A>
inline void DispatchToFunction(Function function, const Tuple1<A>& arg) {
  (*function)(arg.a);
}

template<class Function, class A, class B>
inline void DispatchToFunction(Function function, const Tuple2<A, B>& arg) {
  (*function)(arg.a, arg.b);
}

template<class Function, class A, class B, class C>
inline void DispatchToFunction(Function function, const Tuple3<A, B, C>& arg) {
  (*function)(arg.a, arg.b, arg.c);
}

template<class Function, class A, class B, class C, class D>
inline void DispatchToFunction(Function function,
                               const Tuple4<A, B, C, D>& arg) {
  (*function)(arg.a, arg.b, arg.c, arg.d);
}

template<class Function, class A, class B, class C, class D, class E>
inline void DispatchToFunction(Function function,
                               const Tuple5<A, B, C, D, E>& arg) {
  (*function)(arg.a, arg.b, arg.c, arg.d, arg.e);
}

// Dispatchers with 0 out param (as a Tuple0).

template <class ObjT, class Method>
inline void DispatchToMethod(ObjT* obj, Method method, const Tuple0& arg, Tuple0*) {
  (obj->*method)();
}

template <class ObjT, class Method, class A>
inline void DispatchToMethod(ObjT* obj, Method method, const A& arg, Tuple0*) {
  (obj->*method)(arg);
}

template <class ObjT, class Method, class A>
inline void DispatchToMethod(ObjT* obj, Method method, const Tuple1<A>& arg, Tuple0*) {
  (obj->*method)(arg.a);
}

template<class ObjT, class Method, class A, class B>
inline void DispatchToMethod(ObjT* obj, Method method, const Tuple2<A, B>& arg, Tuple0*) {
  (obj->*method)(arg.a, arg.b);
}

template<class ObjT, class Method, class A, class B, class C>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<A, B, C>& arg, Tuple0*) {
  (obj->*method)(arg.a, arg.b, arg.c);
}

template<class ObjT, class Method, class A, class B, class C, class D>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<A, B, C, D>& arg, Tuple0*) {
  (obj->*method)(arg.a, arg.b, arg.c, arg.d);
}

template<class ObjT, class Method, class A, class B, class C, class D, class E>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<A, B, C, D, E>& arg, Tuple0*) {
  (obj->*method)(arg.a, arg.b, arg.c, arg.d, arg.e);
}

// Dispatchers with 1 out param.

template<class ObjT, class Method,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple0& in,
                             Tuple1<OutA>* out) {
  (obj->*method)(&out->a);
}

template<class ObjT, class Method, class InA,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const InA& in,
                             Tuple1<OutA>* out) {
  (obj->*method)(in, &out->a);
}

template<class ObjT, class Method, class InA,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple1<InA>& in,
                             Tuple1<OutA>* out) {
  (obj->*method)(in.a, &out->a);
}

template<class ObjT, class Method, class InA, class InB,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple2<InA, InB>& in,
                             Tuple1<OutA>* out) {
  (obj->*method)(in.a, in.b, &out->a);
}

template<class ObjT, class Method, class InA, class InB, class InC,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<InA, InB, InC>& in,
                             Tuple1<OutA>* out) {
  (obj->*method)(in.a, in.b, in.c, &out->a);
}

template<class ObjT, class Method, class InA, class InB, class InC, class InD,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<InA, InB, InC, InD>& in,
                             Tuple1<OutA>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, &out->a);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE,
         class OutA>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<InA, InB, InC, InD, InE>& in,
                             Tuple1<OutA>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, in.e, &out->a);
}

// Dispatchers with 2 out params.

template<class ObjT, class Method,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple0& in,
                             Tuple2<OutA, OutB>* out) {
  (obj->*method)(&out->a, &out->b);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const InA& in,
                             Tuple2<OutA, OutB>* out) {
  (obj->*method)(in, &out->a, &out->b);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple1<InA>& in,
                             Tuple2<OutA, OutB>* out) {
  (obj->*method)(in.a, &out->a, &out->b);
}

template<class ObjT, class Method, class InA, class InB,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple2<InA, InB>& in,
                             Tuple2<OutA, OutB>* out) {
  (obj->*method)(in.a, in.b, &out->a, &out->b);
}

template<class ObjT, class Method, class InA, class InB, class InC,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<InA, InB, InC>& in,
                             Tuple2<OutA, OutB>* out) {
  (obj->*method)(in.a, in.b, in.c, &out->a, &out->b);
}

template<class ObjT, class Method, class InA, class InB, class InC, class InD,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<InA, InB, InC, InD>& in,
                             Tuple2<OutA, OutB>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, &out->a, &out->b);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE,
         class OutA, class OutB>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<InA, InB, InC, InD, InE>& in,
                             Tuple2<OutA, OutB>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, in.e, &out->a, &out->b);
}

// Dispatchers with 3 out params.

template<class ObjT, class Method,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple0& in,
                             Tuple3<OutA, OutB, OutC>* out) {
  (obj->*method)(&out->a, &out->b, &out->c);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const InA& in,
                             Tuple3<OutA, OutB, OutC>* out) {
  (obj->*method)(in, &out->a, &out->b, &out->c);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple1<InA>& in,
                             Tuple3<OutA, OutB, OutC>* out) {
  (obj->*method)(in.a, &out->a, &out->b, &out->c);
}

template<class ObjT, class Method, class InA, class InB,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple2<InA, InB>& in,
                             Tuple3<OutA, OutB, OutC>* out) {
  (obj->*method)(in.a, in.b, &out->a, &out->b, &out->c);
}

template<class ObjT, class Method, class InA, class InB, class InC,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<InA, InB, InC>& in,
                             Tuple3<OutA, OutB, OutC>* out) {
  (obj->*method)(in.a, in.b, in.c, &out->a, &out->b, &out->c);
}

template<class ObjT, class Method, class InA, class InB, class InC, class InD,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<InA, InB, InC, InD>& in,
                             Tuple3<OutA, OutB, OutC>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, &out->a, &out->b, &out->c);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE,
         class OutA, class OutB, class OutC>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<InA, InB, InC, InD, InE>& in,
                             Tuple3<OutA, OutB, OutC>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, in.e, &out->a, &out->b, &out->c);
}

// Dispatchers with 4 out params.

template<class ObjT, class Method,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple0& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out) {
  (obj->*method)(&out->a, &out->b, &out->c, &out->d);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const InA& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out) {
  (obj->*method)(in, &out->a, &out->b, &out->c, &out->d);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple1<InA>& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out) {
  (obj->*method)(in.a, &out->a, &out->b, &out->c, &out->d);
}

template<class ObjT, class Method, class InA, class InB,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple2<InA, InB>& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out) {
  (obj->*method)(in.a, in.b, &out->a, &out->b, &out->c, &out->d);
}

template<class ObjT, class Method, class InA, class InB, class InC,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<InA, InB, InC>& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out) {
  (obj->*method)(in.a, in.b, in.c, &out->a, &out->b, &out->c, &out->d);
}

template<class ObjT, class Method, class InA, class InB, class InC, class InD,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<InA, InB, InC, InD>& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, &out->a, &out->b, &out->c, &out->d);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE,
         class OutA, class OutB, class OutC, class OutD>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<InA, InB, InC, InD, InE>& in,
                             Tuple4<OutA, OutB, OutC, OutD>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, in.e,
                 &out->a, &out->b, &out->c, &out->d);
}

// Dispatchers with 5 out params.

template<class ObjT, class Method,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple0& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out) {
  (obj->*method)(&out->a, &out->b, &out->c, &out->d, &out->e);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const InA& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out) {
  (obj->*method)(in, &out->a, &out->b, &out->c, &out->d, &out->e);
}

template<class ObjT, class Method, class InA,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple1<InA>& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out) {
  (obj->*method)(in.a, &out->a, &out->b, &out->c, &out->d, &out->e);
}

template<class ObjT, class Method, class InA, class InB,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple2<InA, InB>& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out) {
  (obj->*method)(in.a, in.b, &out->a, &out->b, &out->c, &out->d, &out->e);
}

template<class ObjT, class Method, class InA, class InB, class InC,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple3<InA, InB, InC>& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out) {
  (obj->*method)(in.a, in.b, in.c, &out->a, &out->b, &out->c, &out->d, &out->e);
}

template<class ObjT, class Method, class InA, class InB, class InC, class InD,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple4<InA, InB, InC, InD>& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, &out->a, &out->b, &out->c, &out->d,
                 &out->e);
}

template<class ObjT, class Method,
         class InA, class InB, class InC, class InD, class InE,
         class OutA, class OutB, class OutC, class OutD, class OutE>
inline void DispatchToMethod(ObjT* obj, Method method,
                             const Tuple5<InA, InB, InC, InD, InE>& in,
                             Tuple5<OutA, OutB, OutC, OutD, OutE>* out) {
  (obj->*method)(in.a, in.b, in.c, in.d, in.e,
                 &out->a, &out->b, &out->c, &out->d, &out->e);
}

#endif  // BASE_TUPLE_H__

