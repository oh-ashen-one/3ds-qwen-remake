# User-facing wrapper. The official devkitPro rules do not safely handle source
# paths containing spaces, so tools/build_3ds.py stages the cross-build when needed.

PYTHON      ?= python3
HOST_CXX    ?= c++
HOST_BUILD  := build-host
TARGET      := elden-ring-3ds-demake

.PHONY: all assets validate-assets test-host run clean

all: assets
	@$(PYTHON) tools/build_3ds.py

assets:
	@$(PYTHON) tools/generate_original_assets.py

validate-assets: assets
	@$(PYTHON) tools/validate_assets.py

test-host: $(HOST_BUILD)/core_tests
	@$(HOST_BUILD)/core_tests

$(HOST_BUILD)/core_tests: source/core.cpp tests/core_tests.cpp include/demake/core.hpp
	@mkdir -p $(HOST_BUILD)
	$(HOST_CXX) -std=c++17 -Wall -Wextra -Werror -pedantic -Iinclude source/core.cpp tests/core_tests.cpp -o $@

run: all
	@test -n "$(IP)" || (echo "Usage: make run IP=<3DS-IP>" && exit 2)
	3dslink $(TARGET).3dsx -a $(IP)

clean:
	@$(PYTHON) tools/build_3ds.py --clean
