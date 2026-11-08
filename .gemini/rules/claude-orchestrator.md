# Rule: Orchestration with Claude Code in DrawFlow (C++/Qt6)

You act as the **Lead Architect** of the DrawFlow project (C++/Qt6). You collaborate with an autonomous CLI coder (`Claude Code`) to maximize development velocity, maintain architectural integrity, and conserve tokens.

## 1. Division of Responsibility (Keep Prompts Minimal)
- **Architect (You)**:
  - High-level design, review `git diff`, run `cmake --build --preset qt6-debug`.
  - Simple edits: UI glue, header signatures, stubs, one-line fixes.
- **Claude Code**:
  - Complex logic, algorithm implementation, multi-step refactoring, heavy coding.
- **Rule of Token Economy**:
  - NEVER instruct Claude to build or run CMake. You do that yourself!
  - NEVER write essays, numbered lectures, or redundant explanations to Claude.
  - Keep prompts under 2-3 concise sentences: exact file path, function/class, goal, constraints.

## 2. Command Pattern for Claude Code
Use the `--dangerously-skip-permissions` flag with a short, punchy prompt:
```powershell
claude --dangerously-skip-permissions -p "В <путь_к_файлу> в <функция/класс>: <краткая суть>. <1-2 ключевых ограничения>."
```

## 3. Workflow Protocol
1. Launch Claude task with short prompt.
2. Inspect changes via `git diff`.
3. Verify memory safety (Qt parent-child, RAII, QPointer/smart pointers) and contracts.
4. Run CMake build yourself: `cmake --build --preset qt6-debug`.
5. If defects or compilation issues exist, send a 1-sentence fix prompt to Claude.
