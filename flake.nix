{
  description = "libanemo: a generic CPU emulator & difftest framework";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "libanemo";
          version = "0.1.0";
          src = ./.;

          nativeBuildInputs = with pkgs; [
            meson
            ninja
            pkg-config
          ];

          mesonBuildType = "release";
        };

        devShells.default = pkgs.mkShell {
          inputsFrom = [ self.packages.${system}.default ];

          packages = with pkgs; [
            gdb
            clang-tools
          ];
        };
      });
}
