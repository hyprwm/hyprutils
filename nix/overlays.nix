{
  self,
  lib,
}: let
  mkDate = longDate: (lib.concatStringsSep "-" [
    (builtins.substring 0 4 longDate)
    (builtins.substring 4 2 longDate)
    (builtins.substring 6 2 longDate)
  ]);

  ver = lib.removeSuffix "\n" (builtins.readFile ../VERSION);
  version = ver + "+date=" + (mkDate (self.lastModifiedDate or "19700101")) + "_" + (self.shortRev or "dirty");
in {
  default = self.overlays.hyprutils;
  hyprutils = final: prev: {
    hyprutils = final.callPackage ./default.nix {
      stdenv = final.gcc15Stdenv;
      inherit version;
    };
    hyprutils-debug = final.hyprutils.override {debug = true;};
    hyprutils-with-tests = final.hyprutils-debug;
  };
}
