// Copyright 2016 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <iostream>
#include <stdexcept>
#include <vector>

#include "test_helpers.h"
#include "zcpointer.h"

class TestFailure : public std::logic_error {
 public:
  using std::logic_error::logic_error;
};

#define STRING(x) QUOTE(x)
#define QUOTE(x) #x
#define AT_FILE_LINE " @ " __FILE__ ":" STRING(__LINE__)

#define EXPECT(expr) do { if (!(expr)) { throw TestFailure(#expr AT_FILE_LINE); } } while(0)

#if defined(ZCPOINTER_TRACK_REFS) && ZCPOINTER_TRACK_REFS

#define EXPECT_UAF(expr) do { \
    try { \
      (expr); \
      throw TestFailure("Expected use-after-free: " #expr AT_FILE_LINE); \
    } catch (zc::UseAfterFreeError) {} \
  } while(0)

#else

#define EXPECT_UAF(expr) do { \
    std::cout << ">>> ZCPOINTER_TRACK_REFS not enabled, cannot catch UAF" << std::endl; \
    try { \
      (expr); \
    } catch (std::logic_error& e) { \
      std::cout << ">>> Caught error: " << typeid(e).name() << ": " << e.what() << std::endl; \
    } \
  } while(0)

#endif

void TestReset() {
  zc::owned<C> c(new C());
  zc::ref<C> owned = c.get();
  zc::ref<C> owned2 = owned;
  c.reset();
  EXPECT_UAF(owned2->DoThing());
}

template <typename T>
void TestUnwrap() {
  zc::owned<T> t(new T());
  //T* unwrap = t.get();

  zc::ref<T> ref = t.get();
  T* unwrap2 = ref;
}

void TestMove() {
  zc::owned<C> c(new C());
  zc::ref<C> owned = c.get();

  zc::owned<C> c2(std::move(c));
  owned->DoThing();

  c2.reset();
  EXPECT_UAF(owned->DoThing());
}

void PtrHelper(zc::ref<C>* out) {
  zc::owned<C> c(new C());
  *out = c.get();
}

void TestPtr() {
  zc::ref<C> ref;
  PtrHelper(&ref);
  EXPECT_UAF(ref->DoThing());
}

void TestEquality() {
  zc::owned<C> a(new C());
  zc::owned<C> b(new C());

  EXPECT(a == a);
  EXPECT(b == b);
  EXPECT(a != b);

  zc::ref<C> ra = a.get();
  zc::ref<C> rb = b.get();

  EXPECT(ra == ra);
  EXPECT(ra == a.get());
  EXPECT(rb == rb);
  EXPECT(rb == b.get());

  EXPECT(rb != ra);

  zc::ref<C> r = a.get();
  EXPECT(r == ra);
  EXPECT(r == a.get());

  zc::owned<C> c;
  zc::owned<C> c2;
  zc::ref<C> rc = nullptr;

  EXPECT(rc == c.get());
  EXPECT(c == nullptr);
  EXPECT(rc == nullptr);
  EXPECT(a != c);
  EXPECT(c == c2);
}

void TestNulls() {
  zc::owned<C> l;
  zc::owned<C> r;

  zc::ref<C> rl = l.get();
  zc::ref<C> rr = r.get();

  r = std::move(l);
  rl = rr;

  EXPECT(l == nullptr);
  EXPECT(r == nullptr);
  EXPECT(rl == nullptr);
  EXPECT(rr == nullptr);
}

void TestVector() {
  zc::owned<C> c;
  std::vector<zc::ref<C>> vec{
    c.get(),
    c.get(),
    c.get()
  };

  for (const auto& r : vec) {
    EXPECT(r == c.get());
  }

  zc::ref<C> ref;
  {
    std::vector<zc::owned<C>> vec;
    vec.push_back(std::move(zc::owned<C>(new C())));
    vec.push_back(std::move(zc::owned<C>(new C())));
    vec.push_back(std::move(zc::owned<C>(new C())));
    ref = vec[1].get();
  }
  EXPECT_UAF(ref->DoThing());
}

void TestStack() {
  zc::ref<C> rc;
  {
    zc::member<C> c;
    rc = &c;
    EXPECT(rc == &c);
    c.DoThing();
  }
  EXPECT_UAF(rc->DoThing());
}

void TestMember() {
  zc::ref<C> ref;
  zc::ref<std::vector<C>> vec_ref;
  {
    X x("hello world");
    ref = x.c();
    vec_ref = x.vec_c();

    vec_ref->push_back(C());
    vec_ref->push_back(C());

    vec_ref->at(1).DoThing();
  }
  EXPECT_UAF(ref->DoThing());
  EXPECT_UAF(vec_ref->at(1).DoThing());

  {
    zc::member<X> x("foo bar");
    ref = x.c();
  }
  EXPECT_UAF(ref->DoThing());
}

#define TEST_FUNC(fn) { #fn , Test##fn }

int main() {
  struct {
    const char* name;
    void (*test)();
  } kTests[] = {
    TEST_FUNC(Reset),
    TEST_FUNC(Move),
    TEST_FUNC(Ptr),
    TEST_FUNC(Equality),
    TEST_FUNC(Nulls),
    TEST_FUNC(Vector),
    TEST_FUNC(Stack),
    TEST_FUNC(Member),
  };

  bool passed = true;
  for (const auto& test : kTests) {
    std::cout << "=== BEGIN " << test.name << " ===" << std::endl;
    try {
      test.test();
      std::cout << "+++ PASS " << test.name << " +++" << std::endl;
    } catch (const TestFailure& e) {
      passed = false;
      std::cout << "!!! FAIL " << test.name
                << ": Assertion failure: " << e.what() << " ===" << std::endl;
    }
  }

  return passed ? 0 : 1;
}
