{ lib
, cmake
, ninja
, libffi
, zlib
, libxml2
, llvmPackages
, python3Packages
,
}:
let
  stdenv = llvmPackages.libcxxStdenv;
  applyPassthrus = drv:
    drv.overrideAttrs (prev: prev // {
      meta = with lib; {
        description = "my personal clang-tidy checks";
        license = llvmPackages.clang-tools.meta.license;
        platforms = platforms.unix;
      };
    });
in
applyPassthrus (stdenv.mkDerivation {
  name = "maitai-${lib.versions.major llvmPackages.llvm.version}";
  pname = "maitai";
  version = lib.versions.major llvmPackages.llvm.version;
  src = lib.fileset.toSource {
    fileset = lib.fileset.unions [
      ./CMakeLists.txt
      ./src
    ];
    root = ./.;
  };
  depsBuildBuild = [
    cmake
    ninja
    stdenv.cc
  ];

  buildInputs =
    (with llvmPackages; [
      clang-tools
      clang-unwrapped
      clang-unwrapped.dev
      libclang
      libcxx
      llvm
      clang
      clang-tools
      clang-unwrapped
      clang-unwrapped.dev
      libclang.dev
      libcxx
      libcxx.dev
      llvm.dev
      llvm.lib
    ])
    ++ [
      python3Packages.lit
      libffi
      zlib
      libxml2
    ];
})
