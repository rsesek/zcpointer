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

#ifndef ZCPOINTER_TEST_HELPERS_H_
#define ZCPOINTER_TEST_HELPERS_H_

#include <string>
#include <vector>

#include "zcpointer.h"

class C {
 public:
  C();
  ~C();
  void DoThing();
};

class X {
 public:
  X(const char* v);
  ~X();

  zc::ref<C> c() { return &c_; }

  zc::ref<std::vector<C>> vec_c() { return &vec_c_; }

  size_t GetCount() const;

  std::string DoString();

 private:
  std::string foo_;
  zc::member<C> c_;
  zc::member<std::vector<C>> vec_c_;
};

#endif  // ZCPOINTER_TEST_HELPERS_H_
