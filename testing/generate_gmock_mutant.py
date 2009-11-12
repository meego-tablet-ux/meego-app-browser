﻿#!/usr/bin/python
# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import string

HEADER = """\
// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file automatically generated by testing/generate_gmock_mutant.py.
// DO NOT EDIT.

#ifndef TESTING_GMOCK_MUTANT_H_
#define TESTING_GMOCK_MUTANT_H_

// The intention of this file is to make possible using GMock actions in
// all of its syntactic beauty. Classes and helper functions can be used as
// more generic variants of Task and Callback classes (see base/task.h)
// Mutant supports both pre-bound arguments (like Task) and call-time
// arguments (like Callback) - hence the name. :-)
//
// DispatchToMethod/Function supports two sets of arguments: pre-bound (P) and
// call-time (C). The arguments as well as the return type are templatized.
// DispatchToMethod/Function will also try to call the selected method or
// function even if provided pre-bound arguments does not match exactly with
// the function signature hence the X1, X2 ... XN parameters in CreateFunctor.
// DispatchToMethod will try to invoke method that may not belong to the
// object's class itself but to the object's class base class.
//
// Additionally you can bind the object at calltime by binding a pointer to
// pointer to the object at creation time - before including this file you
// have to #define GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING.
//
// TODO(stoyan): It's yet not clear to me should we use T& and T&* instead
// of T* and T** when we invoke CreateFunctor to match the EXPECT_CALL style.
//
//
// Sample usage with gMock:
//
// struct Mock : public ObjectDelegate {
//   MOCK_METHOD2(string, OnRequest(int n, const string& request));
//   MOCK_METHOD1(void, OnQuit(int exit_code));
//   MOCK_METHOD2(void, LogMessage(int level, const string& message));
//
//   string HandleFlowers(const string& reply, int n, const string& request) {
//     string result = SStringPrintf("In request of %d %s ", n, request);
//     for (int i = 0; i < n; ++i) result.append(reply)
//     return result;
//   }
//
//   void DoLogMessage(int level, const string& message) {
//   }
//
//   void QuitMessageLoop(int seconds) {
//     MessageLoop* loop = MessageLoop::current();
//     loop->PostDelayedTask(FROM_HERE, new MessageLoop::QuitTask,
//                           1000 * seconds);
//   }
// };
//
// Mock mock;
// // Will invoke mock.HandleFlowers("orchids", n, request)
// // "orchids" is a pre-bound argument, and <n> and <request> are call-time
// // arguments - they are not known until the OnRequest mock is invoked.
// EXPECT_CALL(mock, OnRequest(Ge(5), StartsWith("flower"))
//   .Times(1)
//   .WillOnce(Invoke(CreateFunctor(&mock, &Mock::HandleFlowers,
//       string("orchids"))));
//
//
// // No pre-bound arguments, two call-time arguments passed
// // directly to DoLogMessage
// EXPECT_CALL(mock, OnLogMessage(_, _))
//   .Times(AnyNumber())
//   .WillAlways(Invoke(CreateFunctor, &mock, &Mock::DoLogMessage));
//
//
// // In this case we have a single pre-bound argument - 3. We ignore
// // all of the arguments of OnQuit.
// EXCEPT_CALL(mock, OnQuit(_))
//   .Times(1)
//   .WillOnce(InvokeWithoutArgs(CreateFunctor(
//       &mock, &Mock::QuitMessageLoop, 3)));
//
// MessageLoop loop;
// loop.Run();
//
//
//  // Here is another example of how we can set an action that invokes
//  // method of an object that is not yet created.
// struct Mock : public ObjectDelegate {
//   MOCK_METHOD1(void, DemiurgeCreated(Demiurge*));
//   MOCK_METHOD2(void, OnRequest(int count, const string&));
//
//   void StoreDemiurge(Demiurge* w) {
//     demiurge_ = w;
//   }
//
//   Demiurge* demiurge;
// }
//
// EXPECT_CALL(mock, DemiurgeCreated(_)).Times(1)
//    .WillOnce(Invoke(CreateFunctor(&mock, &Mock::StoreDemiurge)));
//
// EXPECT_CALL(mock, OnRequest(_, StrEq("Moby Dick")))
//    .Times(AnyNumber())
//    .WillAlways(WithArgs<0>(Invoke(
//        CreateFunctor(&mock->demiurge_, &Demiurge::DecreaseMonsters))));
//

#include "base/linked_ptr.h"
#include "base/tuple.h"  // for Tuple

namespace testing {"""

MUTANT = """\

// Interface that is exposed to the consumer, that does the actual calling
// of the method.
template <typename R, typename Params>
class MutantRunner {
 public:
  virtual R RunWithParams(const Params& params) = 0;
  virtual ~MutantRunner() {}
};

// Mutant holds pre-bound arguments (like Task). Like Callback
// allows call-time arguments. You bind a pointer to the object
// at creation time.
template <typename R, typename T, typename Method,
          typename PreBound, typename Params>
class Mutant : public MutantRunner<R, Params> {
 public:
  Mutant(T* obj, Method method, const PreBound& pb)
      : obj_(obj), method_(method), pb_(pb) {
  }

  // MutantRunner implementation
  virtual R RunWithParams(const Params& params) {
    return DispatchToMethod<R>(this->obj_, this->method_, pb_, params);
  }

  T* obj_;
  Method method_;
  PreBound pb_;
};

template <typename R, typename Function, typename PreBound, typename Params>
class MutantFunction : public MutantRunner<R, Params> {
 public:
  MutantFunction(Function function, const PreBound& pb)
      : function_(function), pb_(pb) {
  }

  // MutantRunner implementation
  virtual R RunWithParams(const Params& params) {
    return DispatchToFunction<R>(function_, pb_, params);
  }

  Function function_;
  PreBound pb_;
};

#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING
// MutantLateBind is like Mutant, but you bind a pointer to a pointer
// to the object. This way you can create actions for an object
// that is not yet created (has only storage for a pointer to it).
template <typename R, typename T, typename Method,
          typename PreBound, typename Params>
class MutantLateObjectBind : public MutantRunner<R, Params> {
 public:
  MutantLateObjectBind(T** obj, Method method, const PreBound& pb)
      : obj_(obj), method_(method), pb_(pb) {
  }

  // MutantRunner implementation.
  virtual R RunWithParams(const Params& params) {
    EXPECT_THAT(*this->obj_, testing::NotNull());
    if (NULL == *this->obj_)
      return R();
    return DispatchToMethod<R>( *this->obj_, this->method_, pb_, params);
  }

  T** obj_;
  Method method_;
  PreBound pb_;
};
#endif

// Simple MutantRunner<> wrapper acting as a functor.
// Redirects operator() to MutantRunner<Params>::Run()
template <typename R, typename Params>
struct MutantFunctor {
  explicit MutantFunctor(MutantRunner<R, Params>*  cb) : impl_(cb) {
  }

  ~MutantFunctor() {
  }

  inline R operator()() {
    return impl_->RunWithParams(Tuple0());
  }

  template <typename Arg1>
  inline R operator()(const Arg1& a) {
    return impl_->RunWithParams(Params(a));
  }

  template <typename Arg1, typename Arg2>
  inline R operator()(const Arg1& a, const Arg2& b) {
    return impl_->RunWithParams(Params(a, b));
  }

  template <typename Arg1, typename Arg2, typename Arg3>
  inline R operator()(const Arg1& a, const Arg2& b, const Arg3& c) {
    return impl_->RunWithParams(Params(a, b, c));
  }

  template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
  inline R operator()(const Arg1& a, const Arg2& b, const Arg3& c,
                         const Arg4& d) {
    return impl_->RunWithParams(Params(a, b, c, d));
  }

 private:
  // We need copy constructor since MutantFunctor is copied few times
  // inside GMock machinery, hence no DISALLOW_EVIL_CONTRUCTORS
  MutantFunctor();
  linked_ptr<MutantRunner<R, Params> > impl_;
};
"""

FOOTER = """\
}  // namespace testing

#endif  // TESTING_GMOCK_MUTANT_H_"""

# Templates for DispatchToMethod/DispatchToFunction functions.
# template_params - typename P1, typename P2.. typename C1..
# prebound - TupleN<P1, .. PN>
# calltime - TupleN<C1, .. CN>
# args - p.a, p.b.., c.a, c.b..
DISPATCH_TO_METHOD_TEMPLATE = """\
template <typename R, typename T, typename Method, %(template_params)s>
inline R DispatchToMethod(T* obj, Method method,
                          const %(prebound)s& p,
                          const %(calltime)s& c) {
  return (obj->*method)(%(args)s);
}
"""

DISPATCH_TO_FUNCTION_TEMPLATE = """\
template <typename R, typename Function, %(template_params)s>
inline R DispatchToFunction(Function function,
                            const %(prebound)s& p,
                            const %(calltime)s& c) {
  return (*function)(%(args)s);
}
"""

# Templates for CreateFunctor functions.
# template_params - typename P1, typename P2.. typename C1.. typename X1..
# prebound - TupleN<P1, .. PN>
# calltime - TupleN<A1, .. AN>
# params - X1,.. , A1, ..
# args - const P1& p1 ..
# call_args - p1, p2, p3..
CREATE_METHOD_FUNCTOR_TEMPLATE = """\
template <typename R, typename T, typename U, %(template_params)s>
inline MutantFunctor<R, %(calltime)s>
CreateFunctor(T* obj, R (U::*method)(%(params)s), %(args)s) {
  MutantRunner<R, %(calltime)s>* t =
      new Mutant<R, T, R (U::*)(%(params)s),
                 %(prebound)s, %(calltime)s>
          (obj, method, MakeTuple(%(call_args)s));
  return MutantFunctor<R, %(calltime)s>(t);
}
"""

CREATE_FUNCTION_FUNCTOR_TEMPLATE = """\
template <typename R, %(template_params)s>
inline MutantFunctor<R, %(calltime)s>
CreateFunctor(R (*function)(%(params)s), %(args)s) {
  MutantRunner<R, %(calltime)s>* t =
      new MutantFunction<R, R (*)(%(params)s),
                         %(prebound)s, %(calltime)s>
          (function, MakeTuple(%(call_args)s));
  return MutantFunctor<R, %(calltime)s>(t);
}
"""

def SplitLine(line, width):
  """Splits a single line at comma, at most |width| characters long."""
  if len(line) < width:
    return (line, None)
  n = 1 + line[:width].rfind(",")
  if n == 0:  # If comma cannot be found give up and return the entire line.
    return (line, None)
  # Assume there is a space after the comma
  assert line[n] == " "
  return (line[:n], line[n + 1:])


def Wrap(s, width, subsequent_offset=4):
  """Wraps a single line |s| at commas so every line is at most |width|
     characters long.
  """
  w = []
  spaces = " " * subsequent_offset
  while s:
    (f, s) = SplitLine(s, width)
    w.append(f)
    if s:
      s = spaces  + s
  return "\n".join(w)


def Clean(s):
  """Cleans artifacts from generated C++ code.

  Our simple string formatting/concatenation may introduce extra commas.
  """
  s = s.replace("<>", "")
  s = s.replace(", >", ">")
  s = s.replace(", )", ")")
  s = s.replace(">>", "> >")
  return s


def ExpandPattern(pattern, it):
  """Return list of expanded pattern strings.

  Each string is created by replacing all '%' in |pattern| with element of |it|.
  """
  return [pattern.replace("%", x) for x in it]


def Gen(pattern, n):
  """Expands pattern replacing '%' with sequential integers.

  Expanded patterns will be joined with comma separator.
  GenAlphs("X%", 3) will return "X1, X2, X3".
  """
  it = string.hexdigits[1:n + 1]
  return ", ".join(ExpandPattern(pattern, it))


def GenAlpha(pattern, n):
  """Expands pattern replacing '%' with sequential small ASCII letters.

  Expanded patterns will be joined with comma separator.
  GenAlphs("X%", 3) will return "Xa, Xb, Xc".
  """
  it = string.ascii_lowercase[0:n]
  return ", ".join(ExpandPattern(pattern, it))


def Merge(a):
  return ", ".join(filter(len, a))


def GenTuple(pattern, n):
  return Clean("Tuple%d<%s>" % (n, Gen(pattern, n)))


def FixCode(s):
  lines = Clean(s).splitlines()
  # Wrap sometimes very long 1st and 3rd line at 80th column.
  lines[0] = Wrap(lines[0], 80, 10)
  lines[2] = Wrap(lines[2], 80, 4)
  return "\n".join(lines)


def GenerateDispatch(prebound, calltime):
  print "\n// %d - %d" % (prebound, calltime)
  args = {
      "template_params": Merge([Gen("typename P%", prebound),
                                Gen("typename C%", calltime)]),
      "prebound": GenTuple("P%", prebound),
      "calltime": GenTuple("C%", calltime),
      "args": Merge([GenAlpha("p.%", prebound), GenAlpha("c.%", calltime)]),
  }

  print FixCode(DISPATCH_TO_METHOD_TEMPLATE % args)
  print FixCode(DISPATCH_TO_FUNCTION_TEMPLATE % args)


def GenerateCreateFunctor(prebound, calltime):
  print "// %d - %d" % (prebound, calltime)
  args = {
      "calltime": GenTuple("A%", calltime),
      "prebound": GenTuple("P%", prebound),
      "params": Merge([Gen("X%", prebound), Gen("A%", calltime)]),
      "args": Gen("const P%& p%", prebound),
      "call_args": Gen("p%", prebound),
      "template_params": Merge([Gen("typename P%", prebound),
                                Gen("typename A%", calltime),
                                Gen("typename X%", prebound)])
  }

  mutant = FixCode(CREATE_METHOD_FUNCTOR_TEMPLATE % args)
  print mutant

  # Slightly different version for free function call.
  print "\n", FixCode(CREATE_FUNCTION_FUNCTOR_TEMPLATE % args)

  # Functor with pointer to a pointer of the object.
  print "\n#ifdef GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING"
  mutant2 = mutant.replace("CreateFunctor(T* obj,", "CreateFunctor(T** obj,")
  mutant2 = mutant2.replace("new Mutant", "new MutantLateObjectBind")
  mutant2 = mutant2.replace(" " * 17 + "Tuple", " " * 31 + "Tuple")
  print mutant2
  print "#endif  // GMOCK_MUTANT_INCLUDE_LATE_OBJECT_BINDING\n"

  # Functor for method with __stdcall calling conventions.
  print "#if defined (OS_WIN)"
  stdcall = CREATE_METHOD_FUNCTOR_TEMPLATE.replace("U::", "__stdcall U::")
  print FixCode(stdcall % args)
  print "#endif  // OS_WIN\n"


def main():
  print HEADER
  for prebound in xrange(0, 4 + 1):
    for args in xrange(0, 4 + 1):
      GenerateDispatch(prebound, args)
  print MUTANT
  for prebound in xrange(0, 4 + 1):
    for args in xrange(0, 4 + 1):
      GenerateCreateFunctor(prebound, args)
  print FOOTER

if __name__ == "__main__":
  main()
