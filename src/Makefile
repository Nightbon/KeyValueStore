.PHONY: all tests hash_table self_balancing_binary_search_tree cli gcov_report style cppcheck valgrind check clean dist

BUILD_DIR = ../build
BUILD_COV_DIR = ../build_coverage
CMAKE = cmake
GTEST_DIR = ../third_party/googletest
GTEST_VERSION = v1.15.2

all:    $(BUILD_DIR)/Makefile
	$(CMAKE) --build $(BUILD_DIR) --target hash_table self_balancing_binary_search_tree key_value_store

hash_table: $(BUILD_DIR)/Makefile
	$(CMAKE) --build $(BUILD_DIR) --target hash_table

self_balancing_binary_search_tree: $(BUILD_DIR)/Makefile
	$(CMAKE) --build $(BUILD_DIR) --target self_balancing_binary_search_tree

cli:    $(BUILD_DIR)/Makefile
	$(CMAKE) --build $(BUILD_DIR) --target key_value_store

tests:  $(BUILD_DIR)/Makefile
	$(CMAKE) --build $(BUILD_DIR) --target test_model && ./$(BUILD_DIR)/test_model

$(BUILD_DIR)/Makefile:
	mkdir -p $(BUILD_DIR)
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release

gcov_report: $(BUILD_COV_DIR)/Makefile
	$(CMAKE) --build $(BUILD_COV_DIR) --target test_model
	./$(BUILD_COV_DIR)/test_model
	mkdir -p $(BUILD_COV_DIR)/report
	gcovr -r ./model/ \
		--object-directory $(BUILD_COV_DIR)/CMakeFiles/test_model.dir \
		$(BUILD_COV_DIR)/CMakeFiles/hash_table.dir \
		$(BUILD_COV_DIR)/CMakeFiles/key_value_store.dir \
		$(BUILD_COV_DIR)/CMakeFiles/self_balancing_binary_search_tree.dir \
		--html-details -o $(BUILD_COV_DIR)/report/index.html \
		--print-summary \
		--exclude ".*test*" \
		--exclude ".*\.h" \
		--exclude-throw-branches \
		--exclude-unreachable-branches
	@echo "Отчет успешно сгенерирован: $(BUILD_COV_DIR)/report/index.html"

$(BUILD_COV_DIR)/Makefile:
	mkdir -p $(BUILD_COV_DIR)
	$(CMAKE) -S . -B $(BUILD_COV_DIR) \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_CXX_FLAGS="--coverage -g" \
		-DCMAKE_EXE_LINKER_FLAGS="--coverage"

clean:
	rm -rf $(BUILD_DIR) $(BUILD_COV_DIR) ../dist

dist:
	mkdir -p ../dist
	cp -r . ../dist/src
	cd ../dist && tar -czf ../dist/KeyValueStore.tar.gz src
	rm -rf ../dist/src

style:
	find . \( -name "*.cpp" -o -name "*.h" \) | xargs clang-format -n --style=Google

cppcheck:
	cppcheck --enable=all --suppress=missingIncludeSystem --suppress=missingInclude --suppress=unusedFunction --suppress=unusedStructMember -i model/tests/ model/

valgrind: tests
	cd $(BUILD_DIR) && valgrind --leak-check=full ./test_model

check: clean gcov_report style cppcheck valgrind
	@echo "All checks passed!"