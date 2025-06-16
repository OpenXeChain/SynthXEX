{ stdenv
, cmake
, lib
, ninja
, pkg-config
}:

stdenv.mkDerivation {
  name = "synthxex";
  allowSubstitutes = false;
  src = ./.;
  nativeBuildInputs = [ cmake pkg-config ninja ];

  postUnpack = ''
    chmod -R +w $sourceRoot
  '';

  installPhase = ''
    mkdir -p $out/bin
    cp -v synthxex $out/bin/synthxex
  '';
}