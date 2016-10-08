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

#include "zcpointer.h"

class C {
 public:
  ~C() {
    std::cout << "~C" << std::endl;
  }

  void DoThing() {
    std::cout << "DoThing" << std::endl;
  }
};

void TestReset() {
  zc::owned<C> c(new C());
  zc::ref<C> owned = c.get();
  zc::ref<C> owned2 = owned;
  c.reset();
  owned2->DoThing();
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
  owned->DoThing();
}

void PtrHelper(zc::ref<C>* out) {
  zc::owned<C> c(new C());
  *out = c.get();
}

void TestPtr() {
  zc::ref<C> ref;
  PtrHelper(&ref);
  ref->DoThing();
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
  };

  for (const auto& test : kTests) {
    std::cout << "=== BEGIN " << test.name << " ===" << std::endl;
    try {
      test.test();
      std::cout << "=== FAIL " << test.name
                << ": Did not receive UseAfterFreeException ===" << std::endl;
    } catch (zc::UseAfterFreeError) {
      std::cout << "=== PASS " << test.name << " ===" << std::endl;
    }
  }
}
