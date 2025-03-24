#pragma once

#include <clang-tidy/ClangTidyCheck.h>

namespace maitai {
using namespace clang::tidy;
using namespace clang::ast_matchers;

class ExampleCheck : public ClangTidyCheck {
public:
  ExampleCheck(clang::StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}

  void registerMatchers(MatchFinder *Finder) override;

  void check(const MatchFinder::MatchResult &Result) override;
};

} // namespace maitai
