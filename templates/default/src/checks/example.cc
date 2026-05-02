#include "example.hh"

using namespace clang::ast_matchers;

namespace mychecks {

auto ExampleCheck::registerMatchers(MatchFinder *Finder) -> void {
  Finder->addMatcher(functionDecl(hasName("DeleteMe")).bind("fn"), this);
}

auto ExampleCheck::check(const MatchFinder::MatchResult &Result) -> void {
  const auto *Fn = Result.Nodes.getNodeAs<clang::FunctionDecl>("fn");
  if (!Fn)
    return;
  diag(Fn->getLocation(), "function 'DeleteMe' should not exist");
}

} // namespace mychecks
