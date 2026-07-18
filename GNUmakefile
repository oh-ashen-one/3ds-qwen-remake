# User-facing wrapper. The official devkitPro rules do not safely handle source
# paths containing spaces, so tools/build_3ds.py stages the cross-build when needed.

PYTHON      ?= python3
HOST_CXX    ?= c++
HOST_BUILD  := build-host
TARGET      := elden-ring-3ds-demake
HOST_SOURCES := source/core.cpp source/asset_registry.cpp source/rigid_animation.cpp source/scene_assets.cpp source/zone_resources.cpp tests/core_tests.cpp
GENERATED_REGISTRY := include/demake/generated/asset_registry_data.hpp
GENERATED_SCENE := include/demake/generated/scene_asset_data.hpp
GENERATED_SCENE_STAMP := include/demake/generated/scene_assets.stamp
SCENE_BLOBS := romfs/zones/interior.bin romfs/zones/vista.bin romfs/zones/arena.bin
TEXTURE_ATLAS := data/environment_atlas.t3x
TEX3DS ?= tex3ds
REGISTRY_INPUTS := assets/manifest.json assets/animation_clips.json \
                   assets/zones/interior.json assets/zones/vista.json assets/zones/arena.json \
                   tools/generate_asset_registry.py
SCENE_INPUTS := assets/scene_source.json tools/convert_scene_assets.py
TEXTURE_INPUTS := gfx/environment.t3s gfx/environment.ppm

.PHONY: all assets validate-assets audit-repo test-host verify-build package-sd run clean

all: assets
	@$(PYTHON) tools/build_3ds.py

assets: $(GENERATED_REGISTRY) $(GENERATED_SCENE_STAMP) $(TEXTURE_ATLAS)
	@$(PYTHON) tools/generate_original_assets.py

$(GENERATED_REGISTRY): $(REGISTRY_INPUTS)
	@$(PYTHON) tools/generate_asset_registry.py

$(GENERATED_SCENE_STAMP): $(SCENE_INPUTS)
	@$(PYTHON) tools/convert_scene_assets.py
	@touch $@

$(TEXTURE_ATLAS): $(TEXTURE_INPUTS)
	@mkdir -p data
	@command -v $(TEX3DS) >/dev/null || (echo "tex3ds is required to build the original texture atlas" && exit 2)
	@cd gfx && $(TEX3DS) --atlas -i environment.t3s -o ../$(TEXTURE_ATLAS)

validate-assets: assets
	@$(PYTHON) tools/validate_assets.py

audit-repo:
	@$(PYTHON) tools/audit_repository.py

test-host: $(HOST_BUILD)/core_tests
	@$(HOST_BUILD)/core_tests

$(HOST_BUILD)/core_tests: $(HOST_SOURCES) include/demake/core.hpp \
                         include/demake/asset_registry.hpp include/demake/rigid_animation.hpp \
                         include/demake/scene_assets.hpp include/demake/zone_resources.hpp \
                         $(GENERATED_REGISTRY) $(GENERATED_SCENE_STAMP)
	@mkdir -p $(HOST_BUILD)
	$(HOST_CXX) -std=c++17 -Wall -Wextra -Werror -pedantic -Iinclude $(HOST_SOURCES) -o $@

run: all
	@test -n "$(IP)" || (echo "Usage: make run IP=<3DS-IP>" && exit 2)
	3dslink $(TARGET).3dsx -a $(IP)

verify-build: all
	@$(PYTHON) tools/verify_build.py

package-sd: verify-build
	@$(PYTHON) tools/package_sd.py

clean:
	@$(PYTHON) tools/build_3ds.py --clean
