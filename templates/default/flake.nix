{
  description = "my clang-tidy checks";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
  };

  outputs =
    inputs@{ flake-parts, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [
        "aarch64-darwin"
        "aarch64-linux"
        "x86_64-darwin"
        "x86_64-linux"
      ];

      imports = [ inputs.flake-parts.flakeModules.easyOverlay ];

      perSystem =
        { pkgs, ... }:
        let
          llvmPackages = pkgs.llvmPackages_21;
          pkg = pkgs.callPackage ./default.nix { inherit llvmPackages; };
          new-check = pkgs.writeShellApplication {
            name = "new-check";
            runtimeInputs = [ pkgs.python3 ];
            text = ''exec python3 ${./scripts/new-check.py} "$@"'';
          };
        in
        {
          packages.default = pkg;
          overlayAttrs.${pkg.pname} = pkg;

          devShells.default =
            (pkgs.mkShell.override { stdenv = llvmPackages.libcxxStdenv; })
              {
                inputsFrom = [ pkg ];
                packages = [
                  pkgs.cmake
                  pkgs.ninja
                  llvmPackages.clang-tools
                  new-check
                ];
              };
        };
    };
}
