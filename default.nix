{ lib
, cmake
, ninja
, libffi
, zlib
, libxml2
, llvmPackages
, python3Packages
, python3
, writeText
,
}:
let
  stdenv = llvmPackages.stdenv;
  python3WithLit = python3.withPackages (ps: [ python3Packages.lit ]);
  litScript = writeText "lit.py" ''
    from lit.main import main
    main()
  '';
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
      ./test
    ];
    root = ./.;
  };
  depsBuildBuild = [
    cmake
    ninja
    stdenv.cc
  ];

  doCheck = true;
  checkTarget = "check-maitai";

  cmakeFlags = [
    "-DLLVM_EXTERNAL_LIT=${litScript}"
    "-DPython3_EXECUTABLE=${python3WithLit}/bin/python3"
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
