{
  inputs = {
    nixpkgs = {
      # Use staging-next so clangd works as expected
      # https://github.com/NixOS/nixpkgs/issues/354768
      url = "github:NixOS/nixpkgs/staging-next";
    };

    flake-utils = {
      url = "github:numtide/flake-utils";
    };
  };

  outputs = {
    nixpkgs,
    flake-utils,
    ...
  }:
    flake-utils.lib.eachDefaultSystem (
      system: let
        pkgs = nixpkgs.legacyPackages.${system};
      in {
        devShells.default = pkgs.mkShell {
          shellHook = ''
            export LUA_CPATH=./?.so
          '';
          buildInputs = with pkgs; [
            lua5_1
            luarocks
            libgit2
          ];
          nativeBuildInputs = with pkgs; [
            pkg-config
            gnumake
          ];
          packages = with pkgs; [
            clang-tools
          ];
        };
      }
    );
}
