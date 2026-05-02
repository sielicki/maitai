#include "maitai_avoid_memset.hh"
#include <clang/Lex/Lexer.h>

using namespace clang::ast_matchers;
using clang::CallExpr;
using clang::CharSourceRange;
using clang::Expr;
using clang::FixItHint;
using clang::Lexer;

namespace maitai {

auto AvoidMemsetCheck::registerMatchers(MatchFinder *Finder) -> void {
  Finder->addMatcher(
      callExpr(callee(functionDecl(hasName("memset")))).bind("memset_call"),
      this);
}

auto AvoidMemsetCheck::check(const MatchFinder::MatchResult &Result) -> void {
  const auto *Call = Result.Nodes.getNodeAs<CallExpr>("memset_call");
  if (!Call || Call->getNumArgs() != 3)
    return;

  auto &SM = *Result.SourceManager;
  const auto &LangOpts = Result.Context->getLangOpts();
  const auto srcText = [&](const Expr *E) {
    return Lexer::getSourceText(
               CharSourceRange::getTokenRange(E->getSourceRange()), SM,
               LangOpts)
        .str();
  };

  const auto Replacement = FixItHint::CreateReplacement(
      Call->getSourceRange(),
      "std::fill_n(" + srcText(Call->getArg(0)) + ", " +
          srcText(Call->getArg(2)) + ", " + srcText(Call->getArg(1)) + ")");

  diag(Call->getBeginLoc(), "use std::fill_n instead of memset")
      << Replacement;
}

} // namespace maitai
