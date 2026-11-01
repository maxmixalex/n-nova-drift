PYTHON ?= python3
CMAKE ?= cmake
BUILD_TYPE ?= Release
.PHONY: all host converter ndless test package clean
all: host ndless
host:
	$(CMAKE) -S . -B build/host -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)
	$(CMAKE) --build build/host --parallel
ndless:
	$(PYTHON) tools/build_ndless.py
converter:
	$(CMAKE) -S . -B build/host -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)
	$(CMAKE) --build build/host --target nova-converter --parallel
test: host
	ctest --test-dir build/host --output-on-failure
	$(PYTHON) -m unittest discover -s tests -p 'test_*.py'
package: ndless
	$(PYTHON) tools/package.py
clean:
	$(CMAKE) -E rm -rf build dist
