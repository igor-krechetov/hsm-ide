# Development Process

This document describes the workflow used in the repository, including branching conventions, merge rules, and release flow.

---

## 1. Branch Structure

```mermaid
flowchart TB
    %% Main branch
    subgraph MAIN_BRANCH["main"]
        MAIN_MR["MR<br/>dev → main"]
        MAIN["main<br/>Official mainline.<br/>Receives releases.<br/>Only gets changes from dev."]
    end

    %% Development branch
    subgraph DEV_BRANCH["dev"]
        DEV_MR["MR<br/>feature/abc → dev"]
        DEV["dev<br/>Development branch.<br/><br/>Must contain only commits that passed sanity checks.<br/>All merges are squash + fast-forward."]
    end

    %% Feature branches
    subgraph FEATURE_BRANCHES["Feature branches"]
        F["feature/abc<br/>Developers branch from dev.<br/>Work in multiple commits."]
    end

    %% Feature → Dev
    DEV --> F
    F-- "Submit MR when ready" --> DEV_MR
    DEV_MR-- "squash + FF merge" --> DEV

    %% Dev → Main
    DEV -- "Manual rebase" --> MAIN
    DEV -- "Auto MR if commit contains [r] action" --> MAIN_MR
    MAIN_MR -- "FF merge" --> MAIN
```

### **main**
- The official mainline.
- Contains only stable and released code.
- Receives changes **only** from the `dev` branch.
- Merges from`dev` into `main` may happen:
  - **Manually**, or
  - **Automatically**, if a commit merged into `dev` contains `[r]` in the commit title.

### **dev**
- The branch for continuous, validated development.
- **Only allowed to contain commits that have passed sanity checks.**
- Developers do not commit directly to `dev`.  
  All changes arrive via Merge Requests from feature branches.

### Feature branches
- Named arbitrarily by developers (e.g., `feature/foo`, `fix/bar`).
- Always branch off from `dev`.
- Developers may commit freely here.

---

## 2. Development Workflow

1. **Create a feature branch**
   ```sh
   git checkout dev
   git pull
   git checkout -b feature/my-feature

---

## 3. CI/CD

| Event / Branch       | verify | build | test | autotag | docs | release    |
| -------------------- | ------ | ----- | ---- | ------- | ---- | ---------- |
| **push → main**      | ✅      | ✅     | ✅    | ❌       | ✅    | ❌/✅ optional |
| **PR → main**        | ✅      | ✅     | ✅    | ❌       | ❌    | ❌          |
| **push → dev**       | ✅      | ✅     | ✅    | ✅       | ❌    | ❌          |
| **PR → dev**         | ✅      | ✅     | ✅    | ❌       | ❌    | ❌          |
| **push → any other** | ❌      | ✅     | ✅    | ❌       | ❌    | ❌          |
