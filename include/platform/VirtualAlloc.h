#pragma once
#include <windows.h>
static bool EnableLockMemoryPrivilege() {
  HANDLE hToken = nullptr;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
    std::cerr << "[lock] OpenProcessToken failed: " << GetLastError() << "\n";
    return false;
  }

  TOKEN_PRIVILEGES tp = { 0 };
  LUID luid;
  if (!LookupPrivilegeValueA(nullptr, "SeLockMemoryPrivilege", &luid)) {
    std::cerr << "[lock] LookupPrivilegeValueA failed: " << GetLastError() << "\n";
    CloseHandle(hToken);
    return false;
  }

  tp.PrivilegeCount = 1;
  tp.Privileges[0].Luid = luid;
  tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

  if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr)) {
    std::cerr << "[lock] AdjustTokenPrivileges failed: " << GetLastError() << "\n";
    CloseHandle(hToken);
    return false;
  }

  DWORD err = GetLastError();
  CloseHandle(hToken);
  if (err == ERROR_NOT_ALL_ASSIGNED) {
    std::cerr << "[lock] SeLockMemoryPrivilege not assigned to this account.\n";
    return false;
  }

  return true;
}

static bool IncreaseProcessWorkingSet(SIZE_T extraBytes) {
  // Query current working set
  SIZE_T minWS = 0, maxWS = 0;
  if (!GetProcessWorkingSetSize(GetCurrentProcess(), &minWS, &maxWS)) {
    std::cerr << "[lock] GetProcessWorkingSetSize failed: " << GetLastError() << "\n";
    return false;
  }

  // Add extra to minWS (keep some margin)
  const SIZE_T margin = 64 * 1024 * 1024; // 64MB safety margin
  SIZE_T newMin = minWS + extraBytes + margin;
  SIZE_T newMax = maxWS + extraBytes + margin;

  // Clamp (avoid insane values). You might want to check physical RAM+pagefile.
  // Try to set new working set
  if (!SetProcessWorkingSetSize(GetCurrentProcess(), newMin, newMax)) {
    std::cerr << "[lock] SetProcessWorkingSetSize failed: " << GetLastError() << "\n";
    return false;
  }

  std::cerr << "[lock] Increased working set from (" << minWS << "," << maxWS << ") to ("
    << newMin << "," << newMax << ")\n";
  return true;
}

// Try to VirtualLock a buffer; do best-effort: try privilege -> increase WS -> lock -> fallback
static bool TryVirtualLock(void* ptr, SIZE_T bytes, bool& locked_out) {
  locked_out = false;

  if (ptr == nullptr || bytes == 0) return false;

  // Quick attempt first: maybe user already has privilege or region small
  if (VirtualLock(ptr, bytes)) {
    locked_out = true;
    return true;
  }

  DWORD err = GetLastError();
  std::cerr << "[lock] VirtualLock initial failed: " << err << "\n";

  // Attempt 1: enable privilege (may fail)
  if (EnableLockMemoryPrivilege()) {
    if (VirtualLock(ptr, bytes)) {
      locked_out = true;
      return true;
    }
    else {
      std::cerr << "[lock] VirtualLock failed even after enabling privilege: " << GetLastError() << "\n";
    }
  }
  else {
    std::cerr << "[lock] Could not enable SeLockMemoryPrivilege (probably not assigned).\n";
  }

  // Attempt 2: increase working set size
  if (IncreaseProcessWorkingSet(bytes)) {
    if (VirtualLock(ptr, bytes)) {
      locked_out = true;
      return true;
    }
    else {
      std::cerr << "[lock] VirtualLock failed after SetProcessWorkingSetSize: " << GetLastError() << "\n";
    }
  }
  else {
    std::cerr << "[lock] IncreaseProcessWorkingSet failed or not permitted.\n";
  }

  // final: give up gracefully
  std::cerr << "[lock] Giving up: memory will NOT be locked (performance jitter possible).\n";
  return false;
}