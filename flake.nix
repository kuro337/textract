{
  description = "textract: a cmake-based project";
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
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
            llvmPackages_latest.llvm
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
        packages.default = pkgs.llvmPackages_latest.libcxxStdenv.mkDerivation {
          name = "textract";
          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            llvmPackages_latest.llvm
            pkg-config
          ];

          buildInputs = with pkgs; [
            curl
            openssl
            tesseract
            leptonica
            folly
            gtest
            gflags
            llvmPackages_latest.openmp
            fmt
            boost
            double-conversion
            libevent
            lz4
            zlib
            zstd
            libxml2
          ];

          cmakeFlags = [ "-DCMAKE_CXX_FLAGS=-stdlib=libc++" ];

          preConfigure = ''
            echo 'Preparing build environment:'
            ls -R
          '';
          buildPhase = ''
            echo 'building from:'
            pwd
            ls
            ls -R
            cmake -DBUILD_TESTING=OFF ..

            cmake --build . --verbose

          '';
          doCheck = false;
        };
      }
    );
}
