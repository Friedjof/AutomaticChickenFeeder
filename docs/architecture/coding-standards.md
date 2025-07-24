# Coding Standards

This document defines the coding standards for the **Automated Chicken 2.0** project.  
It is based on the official [Espressif ESP-IDF coding style](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/contribute/style-guide.html) with additional project-specific rules.

---

## 1. General Principles
- **Language:** All source code, comments, and documentation are in **English**.
- **Framework:** All code is written for the **Espressif IoT Development Framework (ESP-IDF)**.
- **Readability:** Code must be easy to read and maintain:
  - Clear, descriptive names for variables and functions.
  - Avoid "magic numbers" → use constants or enums.
  - Use consistent indentation and spacing.

---

## 2. File Organization
- **Directory Structure:**
  - `/components/<component_name>/include/` → public headers
  - `/components/<component_name>/src/` → implementation files
  - `/main/` → application entry point
- **File Naming:**
  - `snake_case` for source and header files (e.g., `feeding_service.c`, `feeding_service.h`).
- **Header Guards:** Use `#pragma once` or classic `#ifndef HEADER_NAME_H`.

---

## 3. Naming Conventions
- **Functions:** `<component>_<action>()`
  - Example: `feeding_service_start_feeding()`
- **Variables:** `snake_case`
- **Constants & Macros:** `UPPER_CASE_WITH_UNDERSCORES`
- **Types & Structs:** `CamelCase` for typedefs (e.g., `FeedSchedule_t`).

---

## 4. Comments & Documentation
- Use **Doxygen**-style comments for public functions and structures:
  ```c
  /**
   * @brief Start feeding process.
   *
   * This function triggers the servo scoop movement to dump feed.
   *
   * @return ESP_OK on success, error code otherwise.
   */
  esp_err_t feeding_service_start_feeding(void);
  ```

* Internal helper functions may use simple `//` or `/* */` comments.

---

## 5. Code Formatting

* **Indentation:** 4 spaces, no tabs.
* **Line Length:** 100 characters max.
* **Braces:** Opening brace on the same line.

  ```c
  if (condition) {
      do_something();
  } else {
      do_something_else();
  }
  ```
* **Spacing:**

  * One space after `if`, `for`, `while`.
  * One blank line between functions.

---

## 6. Error Handling

* Use ESP-IDF error codes (`esp_err_t`).
* Always check return values from system calls and library functions.
* Use `ESP_LOGx` macros for logging (x = I, W, E, D).

---

## 7. Memory Management

* Prefer static allocation when possible.
* Free dynamically allocated memory after use.
* Avoid memory leaks by reviewing all `malloc()` and `free()` calls.

---

## 8. Threading & Concurrency

* Use FreeRTOS primitives (queues, semaphores) for thread safety.
* Keep ISRs (Interrupt Service Routines) short and fast.
* Use `volatile` for variables accessed in ISRs.

---

## 9. Version Control

* All source code changes must go through version control (Git).
* Use descriptive commit messages in imperative form:

  ```
  Add servo power transistor control
  Fix feeding routine timing bug
  ```

---

## 10. Tools

* **Formatting:** `clang-format` or ESP-IDF `astyle_py` can be used for automatic formatting.
* **Linting:** (Optional) Use `cppcheck` for static analysis.

---

## 11. Future Work (TODO)

* Add specific unit testing guidelines.
* Define code coverage requirements.
