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

#include <memory>
#include <forward_list>

namespace zc {

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

 protected:
  friend class ref<T>;

  T* GetRawPointer() const {
    return get();
  }

 private:
  T* get() const {
    return this->std::unique_ptr<T, Deleter>::get();
  }
};

template <typename T>
class ref {
 public:
  ref() : ptr_(nullptr), deleter_(nullptr), deleted_(true) {}

  explicit ref(owned<T>& o) : ptr_(o.GetRawPointer()), deleter_(&o.get_deleter()) {
    deleter_->AddRef(this);
  }

  ref(const ref<T>& o) {
    *this = o;
  }

  ref<T>& operator=(const ref<T>& o) {
    ptr_ = o.ptr_;
    deleter_ = o.deleter_;
    deleted_ = o.deleted_;
    if (!deleted_) {
      deleter_->AddRef(this);
    }
    return *this;
  }

  ~ref() {
    MarkDeleted();
    deleter_->RemoveRef(this);
  }

  T* operator->() const {
    CheckDeleted();
    return ptr_;
  }

 protected:
  friend class internal::OwnedPtrDeleter<T>;

  void MarkDeleted() {
    deleted_ = true;
  }

 private:
  void CheckDeleted() const {
    if (deleted_) {
      internal::RaiseUseAfterFree("attempt to access deleted pointer");
    }
  }

  T* ptr_;
  internal::OwnedPtrDeleter<T>* deleter_;
  bool deleted_ = false;
};

#else

template <typename T>
using owned = std::unique_ptr<T>;

template <typename T>
using ref = T*;

#endif

}  // namespace zc
