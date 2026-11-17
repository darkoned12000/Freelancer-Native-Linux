#pragma once
#include "linux_port/win32_compat.h"
namespace Microsoft { namespace WRL {
template <typename T>
class ComPtr {
 public:
  ComPtr() : ptr_(nullptr) {}
  ComPtr(T* p) : ptr_(p) { if (ptr_) ptr_->AddRef(); }
  ~ComPtr() { if (ptr_) ptr_->Release(); }
  T* Get() const { return ptr_; }
  T** GetAddressOf() { return &ptr_; }
  T* operator->() const { return ptr_; }
  T& operator*() const { return *ptr_; }
  void Reset() { if (ptr_) { ptr_->Release(); ptr_ = nullptr; } }
  void Attach(T* p) { if (ptr_) ptr_->Release(); ptr_ = p; }
  T* Detach() { T* p = ptr_; ptr_ = nullptr; return p; }
  bool operator==(T* p) const { return ptr_ == p; }
  bool operator!=(T* p) const { return ptr_ != p; }
 private:
  T* ptr_;
};
} } // namespace