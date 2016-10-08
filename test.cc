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

#define ZCPOINTER_TRACK_REFS 1

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
  T* unwrap = t.get();
}

void TestMove() {
  zc::owned<C> c(new C());
  zc::ref<C> owned = c.get();

  zc::owned<C> c2(std::move(c));
  owned->DoThing();

  c2.reset();
  owned->DoThing();
}

int main() {
  TestMove();
}
