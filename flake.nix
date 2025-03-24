{
  description = "custom clang-tidy checks example/template";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/master";
    flake-parts.url = "github:hercules-ci/flake-parts";
    treefmt-nix.url = "github:numtide/treefmt-nix";
    git-hooks-nix.url = "github:cachix/git-hooks.nix";
  };

  outputs =
    inputs @ { systems
    , flake-parts
    , ...
    }:
    let
      llvmPackageSet = [
        "18"
        "19"
        "20"
        "git"
      ];
      mkModuleFor = versionString: {
        imports = [
          inputs.flake-parts.flakeModules.easyOverlay
        ];
        perSystem =
          { config
          , pkgs
          , ...
          }:
          let
            pkg = pkgs.callPackage ./default.nix { llvmPackages = pkgs."llvmPackages_${versionString}"; };
            name = pkg.pname + "-" + versionString;
          in
          {
            packages.${name} = pkg;
            overlayAttrs.${name} = config.packages.${name};
          };
      };
    in
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [ "aarch64-darwin" "x86_64-linux" ];
      imports =
        (map mkModuleFor llvmPackageSet)
        ++ [
          inputs.git-hooks-nix.flakeModule
          inputs.treefmt-nix.flakeModule
        ];
      perSystem =
        { config
        , lib
        , pkgs
        , ...
        }: {
          treefmt = {
            programs.ruff-format.enable = true;
            programs.shfmt.enable = true;
            programs.mdformat.enable = true;
            programs.alejandra.enable = false;
            programs.alejandra.package = pkgs.alejandra;
            programs.nixpkgs-fmt.enable = true;
            programs.toml-sort.enable = true;
            programs.yamlfmt.enable = true;
            settings.global.excludes = [
              ".envrc"
              "sgconfig.yml"
              ".pre-commit-config.yaml"
            ];
            programs.typos.enable = true;
          };
          pre-commit = {
            check.enable = true;
            settings.src = ./.;
            settings.hooks = {
              treefmt = {
                packageOverrides.treefmt = config.treefmt.build.wrapper;
                enable = true;
              };

              actionlint.enable = true;
              check-toml.enable = true;
              check-vcs-permalinks.enable = true;
              check-symlinks.enable = true;
              check-yaml.enable = true;
              check-merge-conflicts.enable = true;
              check-json.enable = true;
              check-added-large-files.enable = true;
              detect-aws-credentials.enable = true;
              detect-private-keys.enable = true;
              typos.enable = true;
              ripsecrets.enable = true;
              deadnix.enable = true;
              deadnix.args = [ "--edit" ];
              trim-trailing-whitespace.enable = true;
            };
          };
          devShells.default =
            let
              llvmPackages = pkgs.llvmPackages_19;
              stdenv = llvmPackages.libcxxStdenv;
              mkShell = pkgs.mkShell.override {
                inherit stdenv;
              };
              ast-grep-config = pkgs.writeTextFile {
                name = "sgconfig.yml";
                text = builtins.toJSON (
                  let
                    mkEntry = n: d: {
                      extensions = [ n ];
                      libraryPath = d.outPath + "/parser/${n}.so";
                    };
                    validLangs = pkgs.lib.filterAttrs (n: _v: n == "nix") pkgs.vimPlugins.nvim-treesitter.grammarPlugins;
                  in
                  {
                    ruleDirs = [ "./rules" ];
                    customLanguages = lib.mapAttrs mkEntry validLangs;
                  }
                );
              };
              vscode-config = pkgs.writeTextFile {
                name = "settings.json";
                text = builtins.toJSON {
                  nix.formatterPath = (pkgs.lib.getBin pkgs.nixfmt-rfc-style) + "/bin/nixfmt";
                  astGrep.configPath = ast-grep-config;
                  clangd.path = "${pkgs.lib.getBin llvmPackages.clang-tools}/bin/clangd";
                  clangd.checkUpdates = false;
                };
              };
            in
            mkShell {
              inputsFrom = [ config.packages.maitai-19 ];
              packages = [
                pkgs.nixd
                pkgs.cppcheck
                pkgs.cmake
                pkgs.ninja
                pkgs.ast-grep

                config.treefmt.build.wrapper
                config.pre-commit.settings.package
                config.pre-commit.settings.enabledPackages
              ] ++ (builtins.attrValues config.treefmt.build.programs);
              shellHook = ''
                ${config.pre-commit.installationScript}
                rm -rf .vscode/ && mkdir -p .vscode/ && cp ${vscode-config} .vscode/settings.json;
                rm sgconfig.yml && ln -s ${ast-grep-config} sgconfig.yml
              '';
            };
        };
    };
}
