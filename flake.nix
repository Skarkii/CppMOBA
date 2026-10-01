{
  description = "CppMOBA – multiplayer MOBA in C++23 with raylib";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";

    # FetchContent deps — pinned to the same versions as CMakeLists.txt
    enet-src = {
      url = "github:lsalzman/enet/v1.3.18";
      flake = false;
    };
    raylib-src = {
      url = "github:raysan5/raylib/6.0";
      flake = false;
    };
    lua-src = {
      url = "github:lua/lua/v5.4.8";
      flake = false;
    };
    sol2-src = {
      url = "github:ThePhD/sol2/v3.5.0";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, enet-src, raylib-src, lua-src, sol2-src }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};

      raylibDeps = with pkgs; [
        # X11 + OpenGL
        libGL
        libx11
        libxrandr
        libxinerama
        libxcursor
        libxi
        libxext
        libxfixes

        # wayland support
        wayland
        wayland-protocols
        libxkbcommon
      ];
    in
    {
      devShells.${system}.default = pkgs.mkShell {
        nativeBuildInputs = with pkgs; [
          cmake
          ninja
          pkg-config
          gcc14
        ];

        buildInputs = raylibDeps;
      };

      packages.${system}.default = pkgs.stdenv.mkDerivation {
        pname = "CppMOBA";
        version = "0.1.0";
        src = self;

        nativeBuildInputs = with pkgs; [
          cmake
          ninja
          pkg-config
        ];

        buildInputs = raylibDeps;

        cmakeFlags = [
          "-DMOBA_BUILD_SERVER=ON"
          "-DMOBA_BUILD_CLIENT=ON"
          "-DFETCHCONTENT_FULLY_DISCONNECTED=ON"
          "-DFETCHCONTENT_SOURCE_DIR_ENET=${enet-src}"
          "-DFETCHCONTENT_SOURCE_DIR_RAYLIB=${raylib-src}"
          "-DFETCHCONTENT_SOURCE_DIR_LUA=${lua-src}"
          "-DFETCHCONTENT_SOURCE_DIR_SOL2=${sol2-src}"
        ];

        installPhase = ''
          mkdir -p $out/bin
          cp MOBA_server $out/bin/ 2>/dev/null || true
          cp MOBA_client $out/bin/ 2>/dev/null || true
          cp -r $src/assets $out/bin/assets
        '';
      };
    };
}
