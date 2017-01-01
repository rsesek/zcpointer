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

#ifndef ZCPOINTER_ZCPOINTER_H_
#define ZCPOINTER_ZCPOINTER_H_

#include <limits>
#include <memory>
#include <forward_list>
#include <stdexcept>
#include <utility>

namespace zc {

class UseAfterFreeError : public std::logic_error {
 public:
  using std::logic_error::logic_error;
};

#if defined(ZCPOINTER_TRACK_REFS) && ZCPOINTER_TRACK_REFS

template <typename T> class ref;

namespace internal {

enum class OwnershipBehavior {
  DELETE_POINTER,
  BORROW_POINTER,
};

template <typename T>
class OwnedPtrDeleter {
 public:
  OwnedPtrDeleter()
      : refs_(),
        finalizer_(&OwnedPtrDeleter<T>::HandleDeletePointer) {}

  ~OwnedPtrDeleter() {}

  explicit OwnedPtrDeleter(OwnershipBehavior behavior)
      : refs_(),
        finalizer_(behavior == OwnershipBehavior::BORROW_POINTER
                       ? &OwnedPtrDeleter<T>::HandleBorrowPointer
                       : &OwnedPtrDeleter<T>::HandleDeletePointer) {
  }

  OwnedPtrDeleter(OwnedPtrDeleter&& other)
      : refs_(std::move(other.refs_)),
        finalizer_(other.finalizer_) {
  }

  void operator=(const OwnedPtrDeleter& o) {
    refs_ = o.refs_;
    finalizer_ = o.finalizer_;
  }

  void operator()(T* t) const {
    for (auto& ref : refs_) {
      ref->MarkDeleted();
    }
    (this->finalizer_)(t);
  }

 protected:
  friend class ref<T>;

  void AddRef(ref<T>* ref) {
    refs_.push_front(ref);
  }

  void RemoveRef(ref<T>* ref) {
    refs_.remove(ref);
  }

  static void HandleDeletePointer(T* t) {
    delete t;
  }

  static void HandleBorrowPointer(T* t) {}

 private:
  void (*finalizer_)(T*);
  std::forward_list<ref<T>*> refs_;
};

void RaiseUseAfterFree() __attribute__((noreturn));

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
      internal::RaiseUseAfterFree();
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

template <typename T>
class member : public T {
 public:
  using T::T;

  ref<T> operator&() {
    return ptr_.get();
  }

 private:
  owned<T> ptr_ = owned<T>(this,
                           internal::OwnedPtrDeleter<T>(
                               internal::OwnershipBehavior::BORROW_POINTER));
};

#else

template <typename T>
using owned = std::unique_ptr<T>;

template <typename T>
using ref = T*;

template <typename T>
using member = T;

#endif

}  // namespace zc

#endif  // ZCPOINTER_ZCPOINTER_H_
