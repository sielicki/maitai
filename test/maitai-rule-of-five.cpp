// RUN: %clang_tidy -checks='-*,maitai-rule-of-five' -load %maitai_plugin %s -- -std=c++23 > %t 2>&1 || true
// RUN: %FileCheck %s < %t

// --- rule of zero: nothing declared ----------------------------------------
struct Zero {
  int x;
  int y;
};

// --- rule of five: all five declared ---------------------------------------
struct Five {
  ~Five() = default;
  Five(const Five &) = default;
  Five &operator=(const Five &) = default;
  Five(Five &&) = default;
  Five &operator=(Five &&) = default;
};

// --- non-copyable, non-movable: still rule of five (5 declarations) --------
struct AllDeleted {
  ~AllDeleted() = default;
  AllDeleted(const AllDeleted &) = delete;
  AllDeleted &operator=(const AllDeleted &) = delete;
  AllDeleted(AllDeleted &&) = delete;
  AllDeleted &operator=(AllDeleted &&) = delete;
};

// --- violator: only destructor ---------------------------------------------
struct OnlyDtor {
  // CHECK: :[[@LINE-1]]:{{[0-9]+}}: warning: class 'OnlyDtor' declares 1 of 5 special member functions; missing copy constructor, copy assignment, move constructor, move assignment [maitai-rule-of-five]
  ~OnlyDtor() {}
};

// --- violator: copy ops only -----------------------------------------------
struct CopyOnly {
  // CHECK: :[[@LINE-1]]:{{[0-9]+}}: warning: class 'CopyOnly' declares 2 of 5 special member functions; missing destructor, move constructor, move assignment [maitai-rule-of-five]
  CopyOnly(const CopyOnly &);
  CopyOnly &operator=(const CopyOnly &);
};

// --- violator: classic rule-of-three (in C++23, missing the move pair) -----
struct RuleOfThree {
  // CHECK: :[[@LINE-1]]:{{[0-9]+}}: warning: class 'RuleOfThree' declares 3 of 5 special member functions; missing move constructor, move assignment [maitai-rule-of-five]
  ~RuleOfThree();
  RuleOfThree(const RuleOfThree &);
  RuleOfThree &operator=(const RuleOfThree &);
};

// --- template primary: still diagnosed -------------------------------------
template <typename T>
struct Wrapper {
  // CHECK: :[[@LINE-1]]:{{[0-9]+}}: warning: class 'Wrapper' declares 1 of 5 special member functions; missing copy constructor, copy assignment, move constructor, move assignment [maitai-rule-of-five]
  ~Wrapper() {}
  T value;
};

// --- template instantiation: should NOT be re-diagnosed --------------------
void use() {
  Wrapper<int> w;
  (void)w;
}
