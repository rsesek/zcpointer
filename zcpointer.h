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

#include <limits>
#include <memory>
#include <forward_list>
#include <stdexcept>

namespace zc {

class UseAfterFreeError : public std::logic_error {
 public:
  using std::logic_error::logic_error;
};

#if defined(ZCPOINTER_TRACK_REFS) && ZCPOINTER_TRACK_REFS

template <typename T> class ref;

namespace internal {

template <typename T>
class OwnedPtrDeleter {
 public:
  OwnedPtrDeleter() {}
  ~OwnedPtrDeleter() {}

  OwnedPtrDeleter(OwnedPtrDeleter&& other) : refs_(std::move(other.refs_)) {
  }

  void operator=(const OwnedPtrDeleter& o) {
    refs_ = o.refs_;
  }

  void operator()(T* t) const {
    for (auto& ref : refs_) {
      ref->MarkDeleted();
    }
    delete t;
  }

 protected:
  friend class ref<T>;

  void AddRef(ref<T>* ref) {
    refs_.push_front(ref);
  }

  void RemoveRef(ref<T>* ref) {
    refs_.remove(ref);
  }

 private:
  std::forward_list<ref<T>*> refs_;
};

void RaiseUseAfterFree(const char* error) __attribute__((noreturn));

}  // namespace internal

template <typename T>
class owned : public std::unique_ptr<T, internal::OwnedPtrDeleter<T>> {
 private:
  using Deleter = internal::OwnedPtrDeleter<T>;

 public:
  using std::unique_ptr<T, Deleter>::unique_ptr;

  ref<T> get() {
    return ref<T>(*this);
  }

 private:
  T* get() const {
    return this->std::unique_ptr<T, Deleter>::get();
  }
};

template <typename T>
class ref {
 public:
  ref() : ptr_(nullptr) {}

  ref(std::nullptr_t) : ref() {}

  explicit ref(owned<T>& o) : ptr_(nullptr) {
    if (o != nullptr) {
      ptr_ = &o;
      ptr_->get_deleter().AddRef(this);
    }
  }

  ref(const ref<T>& r) {
    *this = r;
  }

  ref<T>& operator=(const ref<T>& o) {
    ptr_ = o.ptr_;
    if (ptr_ != nullptr && !IsDeleted()) {
      ptr_->get_deleter().AddRef(this);
    }
    return *this;
  }

  ~ref() {
    if (ptr_ != nullptr && !IsDeleted()) {
      ptr_->get_deleter().RemoveRef(this);
    }
    MarkDeleted();
  }

  T* operator->() const {
    CheckDeleted();
    return ptr_->operator->();
  }

  bool operator==(const ref<T>& r) const {
    if (ptr_ == nullptr) {
      return r.ptr_ == nullptr;
    } else {
      return ptr_ == r.ptr_ && *ptr_ == *r.ptr_;
    }
  }

  bool operator==(std::nullptr_t) const {
    return ptr_ == nullptr;
  }

  bool operator!=(const ref<T>& r) const {
    return !(*this == r);
  }

 protected:
  friend class internal::OwnedPtrDeleter<T>;

  void MarkDeleted() {
    ptr_ = DeletedSentinel();
  }

 private:
  void CheckDeleted() const {
    if (IsDeleted()) {
      internal::RaiseUseAfterFree("attempt to access deleted pointer");
    }
  }

  bool IsDeleted() const {
    return ptr_ == DeletedSentinel();
  }

  inline static owned<T>* DeletedSentinel() {
    return reinterpret_cast<owned<T>*>(std::numeric_limits<uintptr_t>::max());
  }

  owned<T>* ptr_;
};

#else

template <typename T>
using owned = std::unique_ptr<T>;

template <typename T>
using ref = T*;

#endif

}  // namespace zc
