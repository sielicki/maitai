#include <clang-tidy/ClangTidy.h>
#include <clang-tidy/ClangTidyModule.h>
#include <clang-tidy/ClangTidyModuleRegistry.h>

#include "maitai_example.hh"

namespace maitai {

class ExampleTidyModule : public clang::tidy::ClangTidyModule {
public:
  void addCheckFactories(
      clang::tidy::ClangTidyCheckFactories &CheckFactories) override {
    CheckFactories.registerCheck<ExampleCheck>("maitai-example");
  }
};

static clang::tidy::ClangTidyModuleRegistry::Add<ExampleTidyModule>
    X("maitai-examples", "Add all maitai example checks.");

} // namespace maitai
