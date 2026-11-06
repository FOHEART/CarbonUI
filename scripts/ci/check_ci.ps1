<#
check_ci.ps1 - Query / wait for GitHub Actions CI/CD results.

Usage:
  powershell -NoProfile -ExecutionPolicy Bypass -File scripts\ci\check_ci.ps1
  powershell -NoProfile -ExecutionPolicy Bypass -File scripts\ci\check_ci.ps1 -Watch
  powershell -NoProfile -ExecutionPolicy Bypass -File scripts\ci\check_ci.ps1 -Watch -Workflow cmake-multi-platform.yml
  powershell -NoProfile -ExecutionPolicy Bypass -File scripts\ci\check_ci.ps1 -RunId 42160538216
  powershell -NoProfile -ExecutionPolicy Bypass -File scripts\ci\check_ci.ps1 -RunId 42160538216 -Watch

Modes:
  (default)          list recent workflow runs
  -Watch             wait for the latest run (or -RunId) to finish, then report verdict
  -RunId <id>        show failed-log summary of a single run (gh run view --log-failed)
  -RunId <id> -Watch wait for that specific run

Auth: uses GH_TOKEN / GITHUB_TOKEN when set; otherwise reads the github.com
credential from git credential fill (Windows Credential Manager) - the same
credential `git push` uses, so no extra `gh auth login` is needed.

Config (env): GH_CLI (full path to gh.exe), GH_REPO_DEFAULT (owner/repo).
#>

param(
    [string]$Repo = $env:GH_REPO_DEFAULT,
    [string]$Workflow = '',
    [int]$Limit = 6,
    [string]$RunId = '',
    [switch]$Watch
)

$ErrorActionPreference = 'Stop'

if ([string]::IsNullOrEmpty($Repo)) {
    # derive owner/repo from the origin remote instead of hard-coding it
    $url = git remote get-url origin 2>$null
    if ($url -match 'github\.com[:/](.+?)(\.git)?$') { $Repo = $Matches[1] }
    if ([string]::IsNullOrEmpty($Repo)) { $Repo = 'FOHEART/CarbonUI' }
}

# ---- locate gh CLI ----------------------------------------------------------
$gh = $env:GH_CLI
if ([string]::IsNullOrEmpty($gh)) { $gh = 'D:\autotools\gh_windows_amd64\bin\gh.exe' }
if (-not (Test-Path -LiteralPath $gh)) {
    # fall back to gh.exe on PATH
    $cmd = Get-Command gh.exe -ErrorAction SilentlyContinue
    if ($cmd) { $gh = $cmd.Source } else { Write-Host "[ERROR] gh.exe not found: $gh (set GH_CLI to its full path)"; exit 2 }
}

# ---- auth: GH_TOKEN > GITHUB_TOKEN > git credential fill ---------------------
if ([string]::IsNullOrEmpty($env:GH_TOKEN) -and [string]::IsNullOrEmpty($env:GITHUB_TOKEN)) {
    $credInput = Join-Path $env:TEMP 'carbonui_cred_in.txt'
    [IO.File]::WriteAllText($credInput, "protocol=https`nhost=github.com`n`n")
    $credOut = cmd /c "git credential fill < `"$credInput`""
    $tok = ($credOut | Where-Object { $_ -like 'password=*' }) -replace '^password=', ''
    if ([string]::IsNullOrEmpty($tok)) {
        Write-Host '[ERROR] No GitHub token available: run a git push once (stores the credential), or set GH_TOKEN.'
        exit 2
    }
    $env:GH_TOKEN = $tok
}

# ---- run ---------------------------------------------------------------------
if (-not [string]::IsNullOrEmpty($RunId)) {
    if ($Watch) {
        & $gh run watch $RunId --repo $Repo --exit-status --interval 30
        $code = $LASTEXITCODE
        Write-Host ($(if ($code -eq 0) { "[OK] run $RunId success" } else { "[FAIL] run $RunId failed" }))
        exit $code
    }
    & $gh run view $RunId --repo $Repo --log-failed 2>&1 | Select-Object -First 60
    exit $LASTEXITCODE
}

$listArgs = @('run', 'list', '--repo', $Repo, '--limit', "$Limit")
if (-not [string]::IsNullOrEmpty($Workflow)) { $listArgs += @('--workflow', $Workflow) }

if ($Watch) {
    $latest = & $gh @listArgs --json databaseId --jq '.[0].databaseId'
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrEmpty($latest)) { Write-Host '[ERROR] No run to wait for.'; exit 1 }
    Write-Host "[INFO] Watching run $latest ..."
    & $gh run watch $latest --repo $Repo --exit-status --interval 30
    $code = $LASTEXITCODE
    Write-Host ($(if ($code -eq 0) { "[OK] run $latest success" } else { "[FAIL] run $latest failed" }))
    & $gh @listArgs
    exit $code
}

& $gh @listArgs
exit $LASTEXITCODE
