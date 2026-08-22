{
  description = "sve4: Vulkan-hardware accelerated video editor";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs =
    { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f nixpkgs.legacyPackages.${system});
    in
    {
      devShells = forAllSystems (
        pkgs:
        let
          pythonDocs = pkgs.python3.withPackages (
            ps: with ps; [
              jinja2
              pygments
            ]
          );
        in
        {
          default = pkgs.mkShell.override { stdenv = pkgs.clangStdenv; } {
            packages = with pkgs; [
              # build
              cmake
              gnumake
              clang
              clang-tools # clang-tidy for the clang-tidy preset
              pkg-config

              # dependencies
              vulkan-headers
              vulkan-loader
              vulkan-validation-layers
              vulkan-volk # the Vulkan meta-loader (not GNU Radio's "volk")
              libwebp
              ffmpeg-full
              glfw3

              # tooling
              valgrind # ctest memorycheck
              lcov # coverage preset
              pre-commit

              # docs target (BUILD_MCSS_DOCS=ON)
              doxygen
              pythonDocs
            ];

            # volk dlopen()s the loader at runtime instead of linking it,
            # so the dynamic linker needs some help finding it on NixOS.
            shellHook = ''
              export LD_LIBRARY_PATH="${pkgs.vulkan-loader}/lib''${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
              export VK_LAYER_PATH="${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d"
              export CMAKE_COLOR_DIAGNOSTICS=ON
              export CMAKE_EXPORT_COMPILE_COMMANDS=ON
            '';
          };
        }
      );
    };
}
