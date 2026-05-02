#include <clang-tidy/ClangTidyModule.h>
#if __clang_major__ < 22
#include <clang-tidy/ClangTidyModuleRegistry.h>
#endif

#include "example.hh"

namespace mychecks {

class MyChecksModule : public clang::tidy::ClangTidyModule {
public:
  void addCheckFactories(
      clang::tidy::ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<ExampleCheck>("mychecks-example");
  }
};

static clang::tidy::ClangTidyModuleRegistry::Add<MyChecksModule>
    X("mychecks", "Adds custom clang-tidy checks.");

} // namespace mychecks
