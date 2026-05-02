{
  description = "custom clang-tidy checks example/template";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/master";
    flake-parts.url = "github:hercules-ci/flake-parts";
    treefmt-nix.url = "github:numtide/treefmt-nix";
    git-hooks-nix.url = "github:cachix/git-hooks.nix";
  };

  outputs =
    inputs@{ flake-parts, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [
        "aarch64-darwin"
        "aarch64-linux"
        "x86_64-linux"
        "x86_64-darwin"
      ];

      imports = [
        inputs.flake-parts.flakeModules.easyOverlay
        inputs.git-hooks-nix.flakeModule
        inputs.treefmt-nix.flakeModule
        ./nix/modules/multi-llvm.nix
      ];

      flake.templates.default = {
        path = ./templates/default;
        description = "Minimal out-of-tree clang-tidy plugin scaffold";
        welcomeText = ''
          # Custom clang-tidy checks template

          You now have a working out-of-tree clang-tidy plugin:

          - `nix build` — builds the plugin and runs the LIT tests
          - `nix develop` — drops you into a dev shell with cmake/ninja/clang-tools
          - `new-check <kebab-case-name>` (from inside the dev shell) — scaffolds
            a new check + LIT test and wires it into `src/module.cc` and CMake

          See https://github.com/sielicki/maitai for the multi-LLVM-version
          variant of this scaffold.
        '';
      };

      perSystem =
        { config
        , lib
        , pkgs
        , ...
        }:
        {
          multiLlvm = {
            versions = [
              "18"
              "19"
              "20"
              "21"
              "22"
            ];
            package = ./default.nix;
          };

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
              llvmPackages = pkgs.llvmPackages_21;
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
                    validLangs = lib.filterAttrs
                      (
                        n: _v: n == "nix"
                      )
                      pkgs.vimPlugins.nvim-treesitter.grammarPlugins;
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
                  nix.formatterPath = (lib.getBin pkgs.nixfmt-rfc-style) + "/bin/nixfmt";
                  astGrep.configPath = ast-grep-config;
                  clangd.path = "${lib.getBin llvmPackages.clang-tools}/bin/clangd";
                  clangd.checkUpdates = false;
                };
              };
              new-check = pkgs.writeShellApplication {
                name = "new-check";
                runtimeInputs = [ pkgs.python3 ];
                text = ''exec python3 ${./scripts/new-check.py} "$@"'';
              };
            in
            mkShell {
              inputsFrom = [ config.packages.maitai-21 ];
              packages = [
                pkgs.nixd
                pkgs.cppcheck
                pkgs.cmake
                pkgs.ninja
                pkgs.ast-grep
                new-check

                config.treefmt.build.wrapper
                config.pre-commit.settings.package
                config.pre-commit.settings.enabledPackages
              ] ++ (builtins.attrValues config.treefmt.build.programs);
              shellHook = ''
                ${config.pre-commit.installationScript}
                rm -rf .vscode/ && mkdir -p .vscode/ && cp ${vscode-config} .vscode/settings.json;
                rm -f sgconfig.yml && ln -s ${ast-grep-config} sgconfig.yml
              '';
            };
        };
    };
}
