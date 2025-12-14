#include <containers/container_dbg.h>
using namespace voxory::containers::debug;

 void container_base_dbg::release_proxy() noexcept {
  std::lock_guard<multithreading::spin_lock> guard(sl_);
  for (auto& elem : list_)
    elem->proxy_ = nullptr;
  list_.clear();
}

 void container_base_dbg::swap_proxies(container_base_dbg& o) {
  list tmp = std::move(list_);
  list_ = std::move(o.list_);
  o.list_ = std::move(list_);
  o.ver_.fetch_add(1, std::memory_order::memory_order_release);
  ver_.fetch_add(1, std::memory_order::memory_order_release);
};

 void container_base_dbg::move_proxies(container_base_dbg& o) {
  o.lock();
  list_ = std::move(o.list_);
  o.ver_.fetch_add(1, std::memory_order::memory_order_release);
  ver_.fetch_add(1, std::memory_order::memory_order_release);
  o.unlock();
};
 container_base_dbg& container_base_dbg::operator=(container_base_dbg&& o) {
   if (this == std::addressof(o)) return *this;
   lock();
   list_ = std::move(o.list_);
   unlock();
   return *this;
 };


 void container_base_dbg::lock() {
  uint32_t old_ver = ver_.load(std::memory_order_acquire);
  sl_.lock();
  ASSERT_ABORT(old_ver == ver_.load(std::memory_order_acquire), "");
};

 void container_base_dbg::unlock() {
  sl_.unlock();
};

iterator_base_dbg::iterator_base_dbg(const iterator_base_dbg& o) : container_(o.container_) {
  register_to_container(container_);
};

iterator_base_dbg::iterator_base_dbg(iterator_base_dbg&& o) : container_(o.container_) {
  register_to_container(container_);
};

iterator_base_dbg::iterator_base_dbg(container_base_dbg* container) : container_(container) {
  register_to_container(container);
}

iterator_base_dbg::iterator_base_dbg(const container_base_dbg* container) : container_(container) {
  register_to_container(container);
}

 void iterator_base_dbg::release_on_destroy() {
  if (!proxy_) return;
  auto nonconst_container = const_cast<container_base_dbg*>(container_);
  nonconst_container->lock();
  if (proxy_)//may change
  {
    nonconst_container->list_.pop_at(proxy_);
    proxy_ = nullptr;
  }
  nonconst_container->unlock();
}

void iterator_base_dbg::register_to_container(const container_base_dbg* container) {
  auto nonconst_container = const_cast<container_base_dbg*>(container);
  nonconst_container->lock();
  proxy_ = nonconst_container->list_.push_back(const_cast<iterator_base_dbg*>(this));
  nonconst_container->unlock();
}
