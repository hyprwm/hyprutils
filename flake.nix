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
  in {
    overlays = import ./nix/overlays.nix {inherit self lib;};

    packages = eachSystem (system: {
      default = self.packages.${system}.hyprutils;
      inherit (pkgsFor.${system}) hyprutils hyprutils-debug hyprutils-with-tests;
    });

    formatter = eachSystem (system: pkgsFor.${system}.alejandra);
  };
}
