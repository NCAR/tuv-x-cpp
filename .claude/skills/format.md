---
description: Format C++ code with clang-format
args: "[file-or-directory]"
---

# Format C++ Code

Apply clang-format to C++ source files.

1. Change to project root: `/Users/fillmore/EarthSystem/TUV-x-cpp`

2. If a specific file or directory argument is provided:
   ```bash
   clang-format -i <file-or-directory>
   ```

   If no argument, format all project files:
   ```bash
   find include src test -name "*.hpp" -o -name "*.cpp" | xargs clang-format -i
   ```

3. Show what changed:
   ```bash
   git diff --stat
   ```

4. If there were changes, list the modified files.

## Examples

- `/format` - Format all C++ files
- `/format include/tuvx/util/array.hpp` - Format specific file
- `/format test/unit/util/` - Format all files in directory
