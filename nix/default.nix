{
  lib,
  stdenv,
  stdenvAdapters,
  cmake,
  pkg-config,
  gtest,
  pixman,
  version ? "git",
  debug ? false,
  # whether to use the mold linker
  # disable this for older machines without SSE4_2 and AVX2 support
  withMold ? true,
}:
let
  inherit (builtins) foldl';
  inherit (lib.lists) flatten optional;
  inherit (lib.strings) optionalString;

  adapters = flatten [
    (lib.optional withMold stdenvAdapters.useMoldLinker)
    (lib.optional debug stdenvAdapters.keepDebugInfo)
  ];

  customStdenv = foldl' (acc: adapter: adapter acc) stdenv adapters;
in
customStdenv.mkDerivation {
  pname = "hyprutils" + optionalString debug "-debug";
  inherit version;
  src = ../.;

  doCheck = debug;

  nativeBuildInputs = [
    cmake
    pkg-config
  ];

  buildInputs = flatten [
    (optional debug gtest)
    pixman
  ];

  outputs = [
    "out"
    "dev"
  ];

  cmakeBuildType = if debug then "Debug" else "RelWithDebInfo";

  meta = with lib; {
    homepage = "https://github.com/hyprwm/hyprutils";
    description = "Small C++ library for utilities used across the Hypr* ecosystem";
    license = licenses.bsd3;
    platforms = platforms.linux;
  };
}
