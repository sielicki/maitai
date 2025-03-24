#include "maitai_example.hh"
#include <clang/Lex/Lexer.h>

using namespace clang::ast_matchers;
using clang::CharSourceRange;
using clang::ConditionalOperator;
using clang::Expr;
using clang::FixItHint;
using clang::Lexer;

namespace maitai {

auto ExampleCheck::registerMatchers(MatchFinder *Finder) -> void {
  Finder->addMatcher(
      expr(isExpandedFromMacro("MACRO_IMPLEMENTING_MIN")).bind("macro_min"),
      this);
}

auto ExampleCheck::check(const MatchFinder::MatchResult &Result) -> void {
  const auto *MacroExpr = Result.Nodes.getNodeAs<Expr>("macro_min");
  if (!MacroExpr)
    return;

  auto &SM = *Result.SourceManager;
  const auto &LangOpts = Result.Context->getLangOpts();

  if (const auto *Cond = dyn_cast<ConditionalOperator>(MacroExpr)) {
    const auto Arg1 =
        Lexer::getSourceText(
            CharSourceRange::getTokenRange(Cond->getLHS()->getSourceRange()),
            SM, LangOpts)
            .str();
    const auto Arg2 =
        Lexer::getSourceText(
            CharSourceRange::getTokenRange(Cond->getRHS()->getSourceRange()),
            SM, LangOpts)
            .str();

    const auto Replacement = FixItHint::CreateReplacement(
        MacroExpr->getSourceRange(), "std::min(" + Arg1 + ", " + Arg2 + ")");

    diag(MacroExpr->getBeginLoc(),
         "use std::min instead of MACRO_IMPLEMENTING_MIN macro")
        << Replacement;
  }
}
} // namespace maitai
