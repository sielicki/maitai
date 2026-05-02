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

// Member-function counterpart to `callToFunction`. Matches calls of the form
// `obj.foo(...)` or `ptr->foo(...)` where `foo` is one of the given names.
// Note that this only matches *non-static* member calls; static methods
// invoked unqualified will fall under `callToFunction` instead.
template <typename... Names>
[[nodiscard]] inline auto callToMemberFunction(Names &&...names) {
  using namespace clang::ast_matchers;
  return cxxMemberCallExpr(
      callee(cxxMethodDecl(hasAnyName(std::forward<Names>(names)...))));
}

// Variadic-argument format-string family. Pairs with checks that nudge toward
// `std::print` / `std::format` (C++23) or `fmt::print`.
[[nodiscard]] inline auto printfFamilyCall() {
  return callToFunction("printf", "fprintf", "sprintf", "snprintf", "vprintf",
                        "vfprintf", "vsprintf", "vsnprintf", "wprintf",
                        "fwprintf", "swprintf", "vswprintf");
}

// Manual C-style allocation and deallocation. Usually paired with checks that
// recommend RAII containers or `std::make_unique` / `std::make_shared`.
[[nodiscard]] inline auto cMemoryFunctionCall() {
  return callToFunction("malloc", "calloc", "realloc", "reallocarray",
                        "aligned_alloc", "free");
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

// Class with at least one *directly declared* virtual method. Classes that
// are polymorphic only by inheritance won't match — combine with
// `isDerivedFrom(polymorphicClass())` if you need that case too.
[[nodiscard]] inline auto polymorphicClass() {
  using namespace clang::ast_matchers;
  return cxxRecordDecl(hasMethod(isVirtual()));
}

// Public non-static data members. `fieldDecl` already excludes static members
// (which are `varDecl`s in the AST). Useful for encapsulation checks.
[[nodiscard]] inline auto publicDataMember() {
  using namespace clang::ast_matchers;
  return fieldDecl(isPublic());
}

// Single-argument constructor that hasn't been marked `explicit` and isn't
// a copy or move constructor — i.e. the kind that participates in implicit
// conversions. Pairs naturally with checks that recommend `explicit`.
[[nodiscard]] inline auto nonExplicitConvertingCtor() {
  using namespace clang::ast_matchers;
  return cxxConstructorDecl(parameterCountIs(1), unless(isExplicit()),
                            unless(isCopyConstructor()),
                            unless(isMoveConstructor()));
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

// Class-template specializations of the standard sequence/associative
// containers, plus `std::span`, `std::string`, and `std::string_view`. Use
// as `hasType(stdContainerType())` to spot accidental copies, by-value
// parameters that should be by-reference, etc.
[[nodiscard]] inline auto stdContainerType() {
  using namespace clang::ast_matchers;
  return qualType(hasDeclaration(classTemplateSpecializationDecl(hasAnyName(
      "std::vector", "std::array", "std::deque", "std::list",
      "std::forward_list", "std::set", "std::multiset", "std::map",
      "std::multimap", "std::unordered_set", "std::unordered_multiset",
      "std::unordered_map", "std::unordered_multimap", "std::stack",
      "std::queue", "std::priority_queue", "std::span", "std::basic_string",
      "std::basic_string_view"))));
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

// C-style explicit cast `(T)expr`, excluding the common `(void)x` discard
// idiom. Pairs with checks that nudge toward `static_cast`,
// `reinterpret_cast`, or `std::bit_cast`.
[[nodiscard]] inline auto cStyleCast() {
  using namespace clang::ast_matchers;
  return cStyleCastExpr(unless(hasType(qualType(asString("void")))));
}

// `typedef T U;` declarations (the C-style spelling). Excludes implicit /
// system-supplied typedefs. C++11 `using U = T;` is a different AST node
// (`typeAliasDecl`) and won't match — that's intentional, since this matcher
// is meant to flag the legacy form for replacement.
[[nodiscard]] inline auto legacyTypedefDecl() {
  using namespace clang::ast_matchers;
  return typedefDecl(unless(isImplicit()));
}

// Variable-length array declarations — a C99 feature not part of standard
// C++. Useful for portability checks targeting MSVC or strict-conformance
// builds.
[[nodiscard]] inline auto vlaDecl() {
  using namespace clang::ast_matchers;
  return varDecl(hasType(variableArrayType()));
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

// Assignment used as the condition of `if` / `while` / `for` / `do-while`.
// Often a typo for `==`. Compilers usually warn, but a check can give a
// targeted diagnostic and a one-character fix.
[[nodiscard]] inline auto assignmentInCondition() {
  using namespace clang::ast_matchers;
  const auto Assign =
      ignoringParenImpCasts(binaryOperator(isAssignmentOperator()));
  return stmt(anyOf(ifStmt(hasCondition(Assign)),
                    whileStmt(hasCondition(Assign)),
                    forStmt(hasCondition(Assign)),
                    doStmt(hasCondition(Assign))));
}

// `reinterpret_cast<T>(x)` — almost always a code smell, since safe uses
// (round-tripping a pointer through `uintptr_t`, low-level serialization)
// are rare and typically warrant explicit review.
[[nodiscard]] inline auto reinterpretCast() {
  using namespace clang::ast_matchers;
  return cxxReinterpretCastExpr();
}

// `const_cast<T>(x)` that strips `const` (rather than adding it). Adding
// `const` is benign; stripping it is the case worth flagging.
[[nodiscard]] inline auto constStrippingCast() {
  using namespace clang::ast_matchers;
  return cxxConstCastExpr(
      hasSourceExpression(hasType(qualType(isConstQualified()))),
      unless(hasType(qualType(isConstQualified()))));
}

} // namespace maitai::matchers
