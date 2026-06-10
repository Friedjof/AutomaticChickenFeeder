{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
  buildInputs = [
    pkgs.gnumake
    pkgs.nodejs
    pkgs.python3
    pkgs.platformio
    pkgs.avrdude
  ];
}
