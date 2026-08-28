# Project Instructions

## Scope

These instructions apply to the entire repository. More specific `AGENTS.md` files may refine them
for individual subdirectories.

## Source Style

- Preserve the existing project style and do not reformat unrelated code.
- Use two spaces for indentation. Do not use tabs for indentation.
- Keep modified and newly added source lines within 100 characters.
- Remove trailing whitespace from all source files that are modified.
- Keep a construct on one line when the complete line fits within 100 characters. This applies to
  declarations, definitions, assignments, expressions, function calls, and parameter lists.
- Do not split function or method declarations and definitions when they fit within 100 characters.
- In declarations and definitions, prioritize keeping the function name and its complete parameter
  list on one line. If the declaration exceeds 100 characters because of a trailing `noexcept`,
  attribute, cv/ref qualifier, `requires` clause, trailing return type, or `/*throw ...*/` comment,
  move that trailing part to the next line before splitting the parameter list.
- Split a parameter list only when the function name and complete parameter list do not fit within
  100 characters even without trailing qualifiers, attributes, or exception comments.
- Apply the same rule to template parameter and argument lists: keep the complete `<...>` list on
  one line when it fits within 100 characters, and split it only when the list cannot fit.
- Do not move call arguments to separate lines when the complete call fits within 100 characters.
- Write control statements with a space after the keyword: `if (`, `for (`, and `while (`.
- Put an empty line between the closing `}` of a completed block and a following independent
  `if (` statement. Do not add such a line before `else`, `catch`, or the `while` of `do-while`.
- Use compressed nested namespace declarations:

  ```cpp
  namespace AdServer::CampaignSvcs
  {
    // Namespace contents are indented by two spaces.
  }
  ```

- When compressing existing nested namespaces, place the new namespace declaration at the
  indentation level of the outer namespace and preserve two-space indentation for its contents.
- Use `using` instead of `typedef` in new and modified code.
- Add comments only when they explain non-obvious behavior; do not narrate straightforward code.
- Use `RequestInfoSvcs/RequestInfoManager/UserFraudProtectionContainer.cpp` as a formatting
  reference when the rules above do not settle a C++ formatting question.

## Verification

- Review the complete diff for style violations before finishing.
- Run `git diff --check` after source changes.
- Build and run the narrowest relevant tests for the changed code.
- Report tests that could not be run and the reason.
