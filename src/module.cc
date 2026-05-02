#include <clang-tidy/ClangTidyModule.h>
#if __clang_major__ < 22
#include <clang-tidy/ClangTidyModuleRegistry.h>
#endif

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
    X("maitai", "Add all maitai checks.");

} // namespace maitai
