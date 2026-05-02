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
in
stdenv.mkDerivation {
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

  nativeBuildInputs = [
    cmake
    ninja
  ];

  doCheck = true;
  checkTarget = "check-maitai";

  cmakeFlags = [
    "-DLLVM_EXTERNAL_LIT=${litScript}"
    "-DPython3_EXECUTABLE=${python3WithLit}/bin/python3"
  ];

  nativeCheckInputs = [
    python3Packages.lit
  ];

  buildInputs =
    (with llvmPackages; [
      clang
      clang-tools
      clang-unwrapped
      clang-unwrapped.dev
      libclang
      libclang.dev
      libcxx
      libcxx.dev
      llvm
      llvm.dev
      llvm.lib
    ])
    ++ [
      libffi
      zlib
      libxml2
    ];

  meta = with lib; {
    description = "my personal clang-tidy checks";
    license = llvmPackages.clang-tools.meta.license;
    platforms = platforms.unix;
  };
}
