# Recovery Protocol

1. Stop changing code when a regression boundary is unclear.
2. Update `CURRENT_STATUS.md` with symptoms, reproduction and last known-good commit.
3. Preserve diagnostics and uncommitted work in a patch/safety commit/branch as appropriate.
4. Reproduce with the smallest relevant test.
5. Compare against the last pushed mini-milestone.
6. Recover with a corrective commit, `git revert`, or a safety branch.
7. Never use `git reset --hard` or destructive checkout to erase unknown work.
8. Re-run the milestone gate before pushing recovery.

