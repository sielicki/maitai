#include "maitai_rule_of_five.hh"
#include "maitai_matchers.hh"

#include <clang/AST/DeclCXX.h>

#include <string>

using namespace clang::ast_matchers;

namespace maitai {

namespace {

struct SpecialMembers {
  bool destructor = false;
  bool copy_ctor = false;
  bool copy_assign = false;
  bool move_ctor = false;
  bool move_assign = false;

  [[nodiscard]] constexpr auto count(bool include_move) const noexcept -> int {
    const auto base = static_cast<int>(destructor) +
                      static_cast<int>(copy_ctor) +
                      static_cast<int>(copy_assign);
    if (!include_move)
      return base;
    return base + static_cast<int>(move_ctor) + static_cast<int>(move_assign);
  }
};

[[nodiscard]] auto inspect(const clang::CXXRecordDecl *Record) -> SpecialMembers {
  return {
      .destructor = Record->hasUserDeclaredDestructor(),
      .copy_ctor = Record->hasUserDeclaredCopyConstructor(),
      .copy_assign = Record->hasUserDeclaredCopyAssignment(),
      .move_ctor = Record->hasUserDeclaredMoveConstructor(),
      .move_assign = Record->hasUserDeclaredMoveAssignment(),
  };
}

auto missingMembers(const SpecialMembers &m, bool include_move) -> std::string {
  std::string out;
  const auto add = [&](bool present, const char *name) {
    if (present)
      return;
    if (!out.empty())
      out += ", ";
    out += name;
  };
  add(m.destructor, "destructor");
  add(m.copy_ctor, "copy constructor");
  add(m.copy_assign, "copy assignment");
  if (include_move) {
    add(m.move_ctor, "move constructor");
    add(m.move_assign, "move assignment");
  }
  return out;
}

} // namespace

auto RuleOfFiveCheck::registerMatchers(MatchFinder *Finder) -> void {
  Finder->addMatcher(
      matchers::nonImplicitClassDefinition(matchers::asWritten()).bind("class"),
      this);
}

auto RuleOfFiveCheck::check(const MatchFinder::MatchResult &Result) -> void {
  const auto *Record = Result.Nodes.getNodeAs<clang::CXXRecordDecl>("class");
  if (Record == nullptr)
    return;

  const auto Members = inspect(Record);
  const bool IncludeMove = Result.Context->getLangOpts().CPlusPlus11;
  const int Expected = IncludeMove ? 5 : 3;
  const int Observed = Members.count(IncludeMove);

  if (Observed == 0 || Observed == Expected)
    return;

  diag(Record->getLocation(),
       "class %0 declares %1 of %2 special member functions; missing %3")
      << Record << Observed << Expected << missingMembers(Members, IncludeMove);
}

} // namespace maitai
