#pragma once

#include <clang/ASTMatchers/ASTMatchers.h>

#include <utility>

namespace maitai::matchers {

// =============================================================================
// Call / macro shapes
// =============================================================================

template <typename... Names>
[[nodiscard]] inline auto callToFunction(Names &&...names) {
  using namespace clang::ast_matchers;
  return callExpr(
      callee(functionDecl(hasAnyName(std::forward<Names>(names)...))));
}

[[nodiscard]] inline auto expandedFromMacroNamed(llvm::StringRef name) {
  using namespace clang::ast_matchers;
  return expr(isExpandedFromMacro(name.str()));
}

// Convenience wrapper around `callToFunction(...)` enumerating a canonical
// set of legacy C string functions. Tweak the names if you want a stricter
// or looser definition for your check.
[[nodiscard]] inline auto cStringLegacyCall() {
  return callToFunction("strcpy", "strcat", "strncpy", "strncat", "sprintf",
                        "vsprintf", "gets", "scanf");
}

// =============================================================================
// Source-location filters
// =============================================================================

// Skips code expanded from a system header. Use as a top-level filter on any
// Decl/Stmt/TypeLoc matcher to keep diagnostics scoped to user code.
[[nodiscard]] inline auto inUserCode() {
  using namespace clang::ast_matchers;
  return unless(isExpansionInSystemHeader());
}

// Filters out template instantiations and explicit specializations, leaving
// only declarations as written by the user (including primary templates and
// partial specializations).
[[nodiscard]] inline auto asWritten() {
  using namespace clang::ast_matchers;
  return allOf(unless(isInstantiated()),
               unless(isExplicitTemplateSpecialization()));
}

// =============================================================================
// Class / declaration shapes
// =============================================================================

// Variadic so call sites can layer additional sub-matchers (e.g.
// `nonImplicitClassDefinition(asWritten(), inUserCode())`).
template <typename... Extras>
[[nodiscard]] inline auto nonImplicitClassDefinition(Extras &&...extras) {
  using namespace clang::ast_matchers;
  return cxxRecordDecl(isDefinition(), unless(isImplicit()), unless(isLambda()),
                       std::forward<Extras>(extras)...);
}

// Mutable variables with global or static storage. Excludes `const`/`constexpr`
// declarations, which represent shared *immutable* state.
[[nodiscard]] inline auto mutableGlobal() {
  using namespace clang::ast_matchers;
  return varDecl(hasGlobalStorage(), unless(isConstexpr()),
                 unless(hasType(isConstQualified())));
}

// Heuristic for an "output" parameter: a non-const lvalue reference or a
// pointer to non-const. Real-world output parameters are a strict subset of
// what this matches, so checks built on it usually need to follow up with
// use-analysis to confirm the parameter is written-to but never read-from.
[[nodiscard]] inline auto outParameter() {
  using namespace clang::ast_matchers;
  return parmVarDecl(anyOf(
      hasType(lValueReferenceType(pointee(unless(isConstQualified())))),
      hasType(pointerType(pointee(unless(isConstQualified()))))));
}

[[nodiscard]] inline auto pureVirtualMethod() {
  using namespace clang::ast_matchers;
  return cxxMethodDecl(isVirtual(), isPure());
}

// =============================================================================
// Type patterns
// =============================================================================

// Class-template specializations of the standard owning/observing smart
// pointers. Use as `hasType(smartPointerType())` on variable / parameter /
// member matchers.
[[nodiscard]] inline auto smartPointerType() {
  using namespace clang::ast_matchers;
  return qualType(hasDeclaration(classTemplateSpecializationDecl(
      hasAnyName("std::unique_ptr", "std::shared_ptr", "std::weak_ptr"))));
}

// =============================================================================
// Modernization candidates
// =============================================================================

// `T arr[N]` declarations. Doesn't filter on storage duration; combine with
// `hasLocalStorage()` or `hasGlobalStorage()` if you want to scope.
[[nodiscard]] inline auto cStyleArrayDecl() {
  using namespace clang::ast_matchers;
  return varDecl(hasType(arrayType()));
}

// Bare `new T(...)` not enclosed in a smart-pointer factory. Pairs naturally
// with a diagnostic that nudges toward `std::make_unique` / `std::make_shared`.
[[nodiscard]] inline auto bareNewExpr() {
  using namespace clang::ast_matchers;
  return cxxNewExpr(unless(hasAncestor(callExpr(callee(functionDecl(hasAnyName(
      "std::make_unique", "std::make_shared", "std::allocate_shared",
      "std::make_unique_for_overwrite")))))));
}

// Lambda with an empty capture list. Pairs with checks that recommend the
// C++23 `[]() static {}` form, since the `static` call operator is only legal
// when the lambda captures nothing.
[[nodiscard]] inline auto nonCapturingLambda() {
  using namespace clang::ast_matchers;
  return lambdaExpr(unless(hasAnyCapture(anything())));
}

// Unscoped `enum X { ... }` (i.e., not `enum class`).
[[nodiscard]] inline auto oldStyleEnum() {
  using namespace clang::ast_matchers;
  return enumDecl(unless(isScoped()));
}

// =============================================================================
// Loop / control-flow patterns
// =============================================================================

// `while(true)`, `while(1)`, `do { ... } while(true)`, and `for(;;)`. Note
// that this is purely syntactic — a body with `break`/`return` will still
// match. Useful as a candidate filter; refine in the check body.
[[nodiscard]] inline auto infiniteLoop() {
  using namespace clang::ast_matchers;
  return stmt(anyOf(
      whileStmt(hasCondition(anyOf(cxxBoolLiteral(equals(true)),
                                   integerLiteral(equals(1))))),
      doStmt(hasCondition(anyOf(cxxBoolLiteral(equals(true)),
                                integerLiteral(equals(1))))),
      forStmt(unless(hasCondition(expr())))));
}

// =============================================================================
// Anti-pattern shapes
// =============================================================================

// `catch (...) {}` and friends — handlers that swallow the exception silently.
// Matches via the empty-CompoundStmt child of the catch, so a body containing
// only comments still flags (the AST has no notion of comment-only bodies).
[[nodiscard]] inline auto emptyCatchHandler() {
  using namespace clang::ast_matchers;
  return cxxCatchStmt(has(compoundStmt(statementCountIs(0))));
}

// Equality / inequality on floating-point operands — almost never what the
// author wants, due to representation error.
[[nodiscard]] inline auto floatEqualityComparison() {
  using namespace clang::ast_matchers;
  return binaryOperator(
      hasAnyOperatorName("==", "!="),
      hasEitherOperand(hasType(realFloatingPointType())));
}

// `x = x` for some variable `x`. Uses bound-node matching internally under
// the id `__sa`; if you want to bind sub-nodes from a check, avoid that name.
[[nodiscard]] inline auto selfAssignment() {
  using namespace clang::ast_matchers;
  return binaryOperator(
      hasOperatorName("="),
      hasLHS(declRefExpr(to(varDecl().bind("__sa")))),
      hasRHS(ignoringParenImpCasts(
          declRefExpr(to(varDecl(equalsBoundNode("__sa")))))));
}

} // namespace maitai::matchers
