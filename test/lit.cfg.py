import lit.formats
import os

config.name = "maitai"
config.test_format = lit.formats.ShTest()
config.suffixes = [".c", ".cpp"]
config.test_source_root = os.path.dirname(__file__)

config.environment["CLANG_NO_DEFAULT_CONFIG"] = "1"

clang_tidy = os.path.join(config.clang_tidy_dir, "clang-tidy")
config.substitutions.append(("%clang_tidy", clang_tidy))
config.substitutions.append(("%maitai_plugin", config.maitai_plugin))
config.substitutions.append(
    ("%FileCheck", os.path.join(config.llvm_tools_dir, "FileCheck"))
)
