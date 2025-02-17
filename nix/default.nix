{
  lib,
  stdenv,
  stdenvAdapters,
  cmake,
  pkg-config,
  pixman,
  version ? "git",
  doCheck ? false,
  debug ? false,
}: let
  inherit (builtins) foldl';
  inherit (lib.lists) flatten;
  inherit (lib.strings) optionalString;

  adapters = flatten [
    stdenvAdapters.useMoldLinker
    (lib.optional debug stdenvAdapters.keepDebugInfo)
  ];

  customStdenv = foldl' (acc: adapter: adapter acc) stdenv adapters;
in
  customStdenv.mkDerivation {
    pname = "hyprutils" + optionalString debug "-debug";
    inherit version doCheck;
    src = ../.;

    nativeBuildInputs = [
      cmake
      pkg-config
    ];

    buildInputs = [
      pixman
    ];

    outputs = ["out" "dev"];

    cmakeBuildType =
      if debug
      then "Debug"
      else "RelWithDebInfo";

    meta = with lib; {
      homepage = "https://github.com/hyprwm/hyprutils";
      description = "Small C++ library for utilities used across the Hypr* ecosystem";
      license = licenses.bsd3;
      platforms = platforms.linux;
    };
  }
