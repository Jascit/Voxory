#include <containers/containers_dbg.h>
using namespace voxory::containers::debug;

void container_base_dbg::_release_proxy() noexcept {
  std::lock_guard<multithreading::spin_lock> guard(_sl);
  for (auto& elem : _list)
    elem->_proxy = nullptr;
  _list.clear();
}

void container_base_dbg::_swap_proxies(container_base_dbg& o) {
  list tmp = std::move(_list);
  _list = std::move(o._list);
  o._list = std::move(_list);
  o._ver.fetch_add(1, std::memory_order::memory_order_release);
  _ver.fetch_add(1, std::memory_order::memory_order_release);
};

void container_base_dbg::_move_proxies(container_base_dbg& o) {
  o._lock();
  _list = std::move(o._list);
  o._ver.fetch_add(1, std::memory_order::memory_order_release);
  _ver.fetch_add(1, std::memory_order::memory_order_release);
  o._unlock();
};

void container_base_dbg::_lock() noexcept {
  uint32_t old_ver = _ver.load(std::memory_order_acquire);
  _sl.lock();
#ifdef DEBUG
  ASSERT_ABORT(old_ver == _ver.load(std::memory_order_acquire), "");
#endif // DEBUG_ITERATORS
};

void container_base_dbg::_unlock() noexcept {
  _sl.unlock();
};

container_base_dbg::container_base_dbg(container_base_dbg&& o) noexcept(noexcept(std::declval<list&>() = std::move(std::declval<list&>()))) : _list{} {
   o._lock();
   _list = std::move(o._list);
   o._ver.fetch_add(1, std::memory_order::memory_order_release);
   o._unlock();
};


iterator_base_dbg::iterator_base_dbg(const iterator_base_dbg& o) : _container(o._container) {
  _register_to_container(_container);
};

iterator_base_dbg::iterator_base_dbg(iterator_base_dbg&& o) : _container(o._container) {
  _register_to_container(_container);
};

iterator_base_dbg::iterator_base_dbg(container_base_dbg* container) : _container(container) {
  _register_to_container(container);
}

iterator_base_dbg::iterator_base_dbg(const container_base_dbg* container) : _container(container) {
  _register_to_container(container);
}

void iterator_base_dbg::_copy_proxy(const iterator_base_dbg& o) {
  _release();
  _container = o._container;
  _register_to_container(_container);
}

void iterator_base_dbg::_release() {
  if (!_proxy) return;
  auto nonconst_container = const_cast<container_base_dbg*>(_container);
  nonconst_container->_lock();
  if (_proxy)//may change
  {
    nonconst_container->_list.pop_at(_proxy);
    _proxy = nullptr;
  }
  nonconst_container->_unlock();
}

void iterator_base_dbg::_register_to_container(const container_base_dbg* container) {
  auto nonconst_container = const_cast<container_base_dbg*>(container);
  nonconst_container->_lock();
  _proxy = nonconst_container->_list.push_back(const_cast<iterator_base_dbg*>(this));
  nonconst_container->_unlock();
}
