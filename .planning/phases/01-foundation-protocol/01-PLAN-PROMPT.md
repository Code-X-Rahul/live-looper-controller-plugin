<planning_context>
**Phase:** 1
**Mode:** standard

<files_to_read>
- .planning/STATE.md (Project State)
- .planning/ROADMAP.md (Roadmap)
- .planning/REQUIREMENTS.md (Requirements)
- .planning/phases/01-foundation-protocol/01-CONTEXT.md (USER DECISIONS from /gsd-discuss-phase)
- .planning/phases/01-foundation-protocol/01-RESEARCH.md (Technical Research)
- .planning/research/ARCHITECTURE.md (Architecture research)
- .planning/research/FEATURES.md (Feature research)
- .planning/research/PITFALLS.md (Pitfalls research)
- .planning/research/STACK.md (Stack research)
- .planning/research/SUMMARY.md (Research synthesis)
- .planning/PROJECT.md (Project definition)
</files_to_read>

**Phase requirement IDs (every ID MUST appear in a plan's `requirements` field):** PLUG-01, PLUG-02, PLUG-03

**Project instructions:** Read ./AGENTS.md if exists — follow project-specific guidelines

**Security enforcement:** enabled — each plan MUST include a `<threat_model>` block covering localhost UDP security, input validation for OSC messages, and error handling for protocol errors.

**Phase 1 goal:** The two-component hybrid architecture works end-to-end — plugin loads in Ableton Live, the Remote Script discovers looper devices across all tracks, and bidirectional communication over UDP is proven.

**Success criteria (from ROADMAP.md):**
1. Plugin loads as VST3 in Ableton Live without errors and passes Pluginval validation
2. Remote Script discovers tracks containing looper devices via Ableton Live API and reports them over UDP
3. Plugin receives looper device state from Remote Script and logs it (debug output or console — no UI required yet)
4. Looper devices are identified by parameter patterns, not just Ableton Looper by name — a third-party looper with matching parameter signatures is also discovered

</planning_context>

<downstream_consumer>
Output consumed by /gsd-execute-phase. Plans need:
- Frontmatter (wave, depends_on, files_modified, autonomous)
- Tasks in XML format with read_first and acceptance_criteria fields (MANDATORY on every task)
- Verification criteria
- must_haves for goal-backward verification
</downstream_consumer>

<deep_work_rules>
## Anti-Shallow Execution Rules (MANDATORY)

Every task MUST include these fields — they are NOT optional:

1. **`<read_first>`** — Files the executor MUST read before touching anything. Always include:
   - The file being modified (so executor sees current state, not assumptions)
   - Any "source of truth" file referenced in CONTEXT.md (reference implementations, existing patterns, config files, schemas)
   - Any file whose patterns, signatures, types, or conventions must be replicated or respected

2. **`<acceptance_criteria>`** — Verifiable conditions that prove the task was done correctly. Rules:
   - Every criterion must be checkable with grep, file read, test command, or CLI output
   - NEVER use subjective language ("looks correct", "properly configured", "consistent with")
   - ALWAYS include exact strings, patterns, values, or command outputs that must be present
   - Examples:
     - Code: `auth.py contains def verify_token(` / `test_auth.py exits 0`
     - Config: `.env.example contains DATABASE_URL=` / `Dockerfile contains HEALTHCHECK`
     - Docs: `README.md contains '## Installation'` / `API.md lists all endpoints`
     - Infra: `deploy.yml has rollback step` / `docker-compose.yml has healthcheck for db`

3. **`<action>`** — Must include CONCRETE values, not references. Rules:
   - NEVER say "align X with Y", "match X to Y", "update to be consistent" without specifying the exact target state
   - ALWAYS include the actual values: config keys, function signatures, SQL statements, class names, import paths, env vars, etc.
   - If CONTEXT.md has a comparison table or expected values, copy them into the action verbatim
   - The executor should be able to complete the task from the action text alone, without needing to read CONTEXT.md or reference files (read_first is for verification, not discovery)

**Why this matters:** Executor agents work from the plan text. Vague instructions like "update the config to match production" produce shallow one-line changes. Concrete instructions like "add DATABASE_URL=postgresql://... , set POOL_SIZE=20, add REDIS_URL=redis://..." produce complete work. The cost of verbose plans is far less than the cost of re-doing shallow execution.
</deep_work_rules>

<quality_gate>
- [ ] PLAN.md files created in phase directory
- [ ] Each plan has valid frontmatter
- [ ] Tasks are specific and actionable
- [ ] Every task has `<read_first>` with at least the file being modified
- [ ] Every task has `<acceptance_criteria>` with grep-verifiable conditions
- [ ] Every `<action>` contains concrete values (no "align X with Y" without specifying what)
- [ ] Dependencies correctly identified
- [ ] Waves assigned for parallel execution
- [ ] must_haves derived from phase goal
</quality_gate>

CRITICAL PLANNING CONSTRAINTS FROM CONTEXT.md:

**Locked Decisions (DO NOT CHANGE):**
- D-01: JSON with namespace/action message format `{ns, nsid, name, args}` structure
- D-02: UUID per request for request/response correlation
- D-03: Full state push on change
- D-04: Version field in every message (`version: 1`)
- D-05: Known ports with range fallback (7010-7019)
- D-06: Immediate handshake on load with exponential backoff retry
- D-07: Auto-reconnect with exponential backoff on disconnect
- D-08: Audio passthrough plugin (processBlock is no-op for audio)
- D-09: VST3 only for v1
- D-10: Minimum Ableton Live 11+

**Agent's Discretion (decide during planning):**
- Looper discovery intelligence level — whether Phase 1 starts with pattern-based match or class-name match first
- Exact JSON message field names and type definitions
- Error code structure for protocol errors
- Heartbeat/ping interval during active connection
- Whether the plugin exposes any parameters to Ableton's device panel in Phase 1 (minimal: just connection status)
- JUCE module selection beyond required set
- C++ project structure and directory layout
- Python Remote Script code organization

**This is a GREENFIELD project.** There is no existing source code. All files listed in plans will be created from scratch.

**Project structure from RESEARCH.md should be followed:**
- `src/Plugin/` — JUCE plugin entry point
- `src/Model/` — LooperState and LooperTracker
- `src/Bridge/` — BridgeClient and MessageProtocol
- `src/Shared/` — ProtocolDefs (shared constants)
- `remote-script/` — Python Remote Script files
- `tests/` — C++ tests (Catch2) and Python tests (pytest)
- `third_party/JUCE/` — Git submodule

**Build order dependency from RESEARCH.md:**
1. MessageProtocol + Shared definitions → no dependencies, defines the contract
2. Model layer → depends on protocol types only
3. BridgeClient → depends on protocol + JUCE OSC
4. Remote Script → depends on protocol
5. PluginProcessor (minimal) → depends on Model + Bridge
6. Integration testing → full system with Live running