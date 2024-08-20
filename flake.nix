{
  description = "Textract: A CMake-based project";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            cmake
            llvm
            openssl
            tesseract
            leptonica
            folly
            gtest
            gflags
            fmt
            boost
            double-conversion
            libevent
            lz4
            zlib
            zstd
            libxml2
          ];

          shellHook = ''
            export TESSDATA_PREFIX=${pkgs.tesseract}/share/tessdata
          '';
        };

        packages.default = pkgs.stdenv.mkDerivation {
          name = "textract";
          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            llvm
            pkg-config
          ];

          buildInputs = with pkgs; [
            openssl
            tesseract
            leptonica
            folly
            gtest
            gflags
            llvmPackages.openmp
            fmt
            boost
            double-conversion
            libevent
            lz4
            zlib
            zstd
            libxml2
          ];

          cmakeFlags = [
            "-DCMAKE_BUILD_TYPE=Release"
            "-DCMAKE_CXX_FLAGS=-fopenmp"
          ];

          NIX_CFLAGS_COMPILE = "-fopenmp";
          NIX_LDFLAGS = "-L${pkgs.llvmPackages.openmp}/lib -lomp";

          preConfigure = ''
            # Ensure all source files are available
            cp -r $src/* .
          '';

          buildPhase = ''
            cmake .
            make
          '';

          installPhase = ''
            mkdir -p $out/bin
            cp bin/main $out/bin/textract
          '';
        };
      }
    );
}
