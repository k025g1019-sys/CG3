<#
.SYNOPSIS
    Merge the engine branch into the current game branch.

.DESCRIPTION
    Run this on the game branch (the branch that has Game/Game.vcxproj).
    It merges the engine branch and removes the engine-only test app (Sandbox/)
    from the merge, so the game branch keeps only the engine and the game.

    Usage (from anywhere inside the repository):
        powershell -ExecutionPolicy Bypass -File tools/MergeEngine.ps1
        powershell -ExecutionPolicy Bypass -File tools/MergeEngine.ps1 -EngineBranch CG3_develop

    (This file is ASCII only on purpose: Windows PowerShell 5.1 misreads
     non-ASCII characters in scripts saved as UTF-8 without BOM.)
#>
param(
    [string]$EngineBranch = "CG3_develop"
)

# Work from the repository root
Set-Location (git rev-parse --show-toplevel)

if (-not (Test-Path "Game/Game.vcxproj")) {
    Write-Host "This is not the game branch (Game/Game.vcxproj was not found)." -ForegroundColor Red
    exit 1
}

# Uncommitted changes would be mixed into the merge commit
$dirty = git status --porcelain --untracked-files=no
if ($dirty) {
    Write-Host "Commit or stash your changes first:" -ForegroundColor Red
    $dirty
    exit 1
}

$gameBranch = git rev-parse --abbrev-ref HEAD
Write-Host "Merging '$EngineBranch' into '$gameBranch'..."

# Merge without committing yet (conflicts are handled below).
# Rename detection is turned off on purpose: Game/Game.vcxproj and Game/main.cpp look like
# renamed copies of the Sandbox ones, so with renames on, engine-side edits to Sandbox/
# would be merged into the game project instead of being dropped.
git merge --no-ff --no-commit -X no-renames $EngineBranch

if (-not (git rev-parse -q --verify MERGE_HEAD)) {
    Write-Host "Nothing to merge."
    exit 0
}

# The engine test app (Sandbox/) never goes into the game branch.
# This also resolves "modified in engine / deleted in game" conflicts under Sandbox/.
git rm -r -q --ignore-unmatch -- Sandbox | Out-Null

$conflicts = git diff --name-only --diff-filter=U
if ($conflicts) {
    Write-Host "Conflicts remain. Fix these files, then run 'git add <file>' and 'git commit':" -ForegroundColor Yellow
    $conflicts
    exit 1
}

git commit --no-edit
if ($LASTEXITCODE -ne 0) {
    exit 1
}
Write-Host "Merged '$EngineBranch' into '$gameBranch'." -ForegroundColor Green
