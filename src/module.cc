#include <clang-tidy/ClangTidyModule.h>
#if __clang_major__ < 22
#include <clang-tidy/ClangTidyModuleRegistry.h>
#endif

#include "maitai_avoid_memset.hh"
#include "maitai_example.hh"
#include "maitai_rule_of_five.hh"

namespace maitai {

class ExampleTidyModule : public clang::tidy::ClangTidyModule {
public:
  void addCheckFactories(
      clang::tidy::ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<ExampleCheck>("maitai-example");
    CheckFactories.registerCheck<AvoidMemsetCheck>("maitai-avoid-memset");
    CheckFactories.registerCheck<RuleOfFiveCheck>("maitai-rule-of-five");
  }
};

static clang::tidy::ClangTidyModuleRegistry::Add<ExampleTidyModule>
    X("maitai", "Add all maitai checks.");

} // namespace maitai
