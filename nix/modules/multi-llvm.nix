{ lib, flake-parts-lib, ... }:
let
  inherit (lib) mkOption types;
  inherit (flake-parts-lib) mkPerSystemOption;
in
{
  options.perSystem = mkPerSystemOption (
    { ... }:
    {
      options.multiLlvm = {
        versions = mkOption {
          type = types.listOf types.str;
          default = [ ];
          example = [ "20" "21" ];
          description = "LLVM major versions (matching `llvmPackages_<v>`) to build packages for.";
        };
        package = mkOption {
          type = types.path;
          description = "Path to a derivation expression accepting an `llvmPackages` argument.";
        };
        callPackageArgs = mkOption {
          type = types.attrs;
          default = { };
          description = "Extra arguments forwarded to `pkgs.callPackage`.";
        };
      };
    }
  );

  config.perSystem =
    { config, pkgs, ... }:
    let
      cfg = config.multiLlvm;
      mkPkg =
        version:
        let
          pkg = pkgs.callPackage cfg.package (
            { llvmPackages = pkgs."llvmPackages_${version}"; } // cfg.callPackageArgs
          );
        in
        {
          name = "${pkg.pname}-${version}";
          value = pkg;
        };
      byName = lib.listToAttrs (map mkPkg cfg.versions);
    in
    {
      packages = byName;
      overlayAttrs = lib.mapAttrs (n: _: config.packages.${n}) byName;
    };
}
