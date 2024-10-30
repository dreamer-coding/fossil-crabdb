import os
import re


class TestRunnerGenerator:
    def __init__(self):
        self.directory = os.getcwd()

    def find_test_groups(self, extension):
        """
        Finds test groups in files matching `test_<name>.<extension>`
        within directories named `with_<language>`.
        """
        test_groups = set()
        pattern = r"FOSSIL_TEST_GROUP\((\w+)\)"

        # Directory and filename pattern to match `with_<language>/test_<name>.<ext>`
        for root, _, files in os.walk(self.directory):
            # Check if the directory matches `with_<language>`
            if os.path.basename(root).startswith("with_"):
                for file in files:
                    # Check if the file matches `test_<name>.<ext>` with the specified extension
                    if file.startswith("test_") and file.endswith(extension):
                        with open(os.path.join(root, file), "r") as f:
                            content = f.read()
                            matches = re.findall(pattern, content)
                            test_groups.update(matches)

        return list(test_groups)

    def generate_test_runner(self, test_groups, is_cpp=False):
        """
        Generates a test runner file based on the given test groups.
        If is_cpp is True, generates a C++-compatible test runner.
        """
        language_header = "C++" if is_cpp else "C"
        file_name = "unit_runner.cpp" if is_cpp else "unit_runner.c"

        # Header for the generated test runner
        header = f"""
// Generated Fossil Logic Test Runner ({language_header})
#include <fossil/unittest/framework.h>

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test List
// * * * * * * * * * * * * * * * * * * * * * * * *
"""

        extern_pools = "\n".join(
            [f"FOSSIL_TEST_EXPORT({group});" for group in test_groups]
        )

        runner = f"""
// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Runner ({language_header})
// * * * * * * * * * * * * * * * * * * * * * * * *
int main(int argc, char **argv) {{
    FOSSIL_TEST_CREATE(argc, argv);
"""

        import_pools = "\n".join(
            [f"    FOSSIL_TEST_IMPORT({group});" for group in test_groups]
        )

        footer = """
    FOSSIL_TEST_RUN();
    return FOSSIL_TEST_ERASE();
} // end of main
"""

        # Write the generated test runner to a file
        with open(file_name, "w") as file:
            file.write(header)
            file.write(extern_pools)
            file.write(runner)
            file.write(import_pools)
            file.write(footer)

    def generate_runners(self):
        # Find and generate runners for C files
        c_test_groups = self.find_test_groups(".c")
        self.generate_test_runner(c_test_groups, is_cpp=False)

        # Find and generate runners for C++ files
        cpp_test_groups = self.find_test_groups(".cpp")
        self.generate_test_runner(cpp_test_groups, is_cpp=True)


# Instantiate the generator and create runners for both C and C++
generator = TestRunnerGenerator()
generator.generate_runners()
