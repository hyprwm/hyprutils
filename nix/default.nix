{
  lib,
  stdenv,
  cmake,
  version ? "git",
  doCheck ? false,
}:
stdenv.mkDerivation {
  pname = "hyprutils";
  inherit version doCheck;
  src = ../.;

  nativeBuildInputs = [cmake];

  outputs = ["out" "dev"];

  meta = with lib; {
    homepage = "https://github.com/hyprwm/hyprutils";
    description = "Small C++ library for utilities used across the Hypr* ecosystem";
    license = licenses.bsd3;
    platforms = platforms.linux;
  };
}
