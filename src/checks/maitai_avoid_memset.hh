#pragma once

#include <clang-tidy/ClangTidyCheck.h>

namespace maitai {

class AvoidMemsetCheck : public clang::tidy::ClangTidyCheck {
public:
  AvoidMemsetCheck(clang::StringRef Name, clang::tidy::ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}

  void registerMatchers(clang::ast_matchers::MatchFinder *Finder) override;

  void check(const clang::ast_matchers::MatchFinder::MatchResult &Result) override;
};

} // namespace maitai
