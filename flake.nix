{
  description = "Small C++ library for utilities used across the Hypr* ecosystem";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    systems.url = "github:nix-systems/default-linux";
  };

  outputs = {
    self,
    nixpkgs,
    systems,
  }: let
    inherit (nixpkgs) lib;
    eachSystem = lib.genAttrs (import systems);
    pkgsFor = eachSystem (system:
      import nixpkgs {
        localSystem.system = system;
        overlays = with self.overlays; [hyprutils];
      });
    mkDate = longDate: (lib.concatStringsSep "-" [
      (builtins.substring 0 4 longDate)
      (builtins.substring 4 2 longDate)
      (builtins.substring 6 2 longDate)
    ]);
  in {
    overlays = {
      default = self.overlays.hyprutils;
      hyprutils = final: prev: {
        hyprutils = final.callPackage ./nix/default.nix {
          stdenv = final.gcc13Stdenv;
          version = "0.pre" + "+date=" + (mkDate (self.lastModifiedDate or "19700101")) + "_" + (self.shortRev or "dirty");
        };
        hyprutils-with-tests = final.hyprutils.override {doCheck = true;};
      };
    };

    packages = eachSystem (system: {
      default = self.packages.${system}.hyprutils;
      inherit (pkgsFor.${system}) hyprutils hyprutils-with-tests;
    });

    formatter = eachSystem (system: pkgsFor.${system}.alejandra);
  };
}
