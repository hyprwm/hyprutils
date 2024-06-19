{
  lib,
  stdenv,
  cmake,
  pkg-config,
  pixman,
  version ? "git",
  doCheck ? false,
}:
stdenv.mkDerivation {
  pname = "hyprutils";
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

  cmakeBuildType = "RelWithDebInfo";

  dontStrip = true;

  meta = with lib; {
    homepage = "https://github.com/hyprwm/hyprutils";
    description = "Small C++ library for utilities used across the Hypr* ecosystem";
    license = licenses.bsd3;
    platforms = platforms.linux;
  };
}
