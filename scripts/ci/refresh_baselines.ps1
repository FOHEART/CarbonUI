<#
refresh_baselines.ps1 - Sync screenshot baselines with CI-rendered captures.

The committed baselines (tests\visual\baseline\<platform>\*.png) must be pixels
produced on the GitHub runners, otherwise cross-machine font/rasterizer noise
fails the Screenshot suite permanently (see tests/support/screenshot_support.h).
This script downloads the ui-artifacts of a (failed) run and copies the
captured screenshots over the baselines:

  windows artifact  -> tests\visual\baseline\windows
  ubuntu artifact   -> tests\visual\baseline\linux
  (diff images *-diff.png are skipped)

Usage:
  scripts\ci\refresh_baselines.bat                    latest run of CMake workflow
  scripts\ci\refresh_baselines.bat -RunId 123456      artifacts of a given run
  scripts\ci\refresh_baselines.bat -DryRun            list what would be copied

Afterwards review `git diff --stat tests/visual/baseline`, commit, push and let
CI verify the comparison is green.
#>

param(
    [string]$Repo = '',
    [string]$RunId = '',
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'ci_common.ps1')

$Repo = Resolve-Repo $Repo
Set-GhToken
$gh = Resolve-GhCli
$root = Resolve-RepoRoot

if ([string]::IsNullOrEmpty($RunId))
{
    # prefer the latest (failed) run of the CMake workflow: baseline mismatches
    # are exactly what produces fresh ui-artifacts
    $RunId = & $gh run list --repo $Repo --workflow cmake-multi-platform.yml --limit 1 --json databaseId --jq '.[0].databaseId'
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrEmpty($RunId)) { Write-Host '[ERROR] No runs of the CMake workflow found.'; exit 1 }
    Write-Host "[INFO] Using latest CMake workflow run $RunId"
}

$dest = Join-Path $root 'out\ci-artifacts'
if (Test-Path -LiteralPath $dest)
{
    Write-Host "[INFO] Cleaning existing $dest"
    Remove-Item -LiteralPath $dest -Recurse -Force
}

& $gh run download $RunId --repo $Repo --dir $dest
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$shots = Get-ChildItem $dest -Recurse -Filter *.png |
    Where-Object { $_.FullName -match 'screenshots' -and $_.Name -notlike '*-diff.png' }

if (-not $shots)
{
    Write-Host '[ERROR] No screenshot captures found in the artifacts.'
    exit 1
}

$copied = 0
foreach ($f in $shots)
{
    if ($f.FullName -match 'windows') { $platform = 'windows' }
    elseif ($f.FullName -match 'ubuntu|linux') { $platform = 'linux' }
    else { Write-Host "[SKIP] unknown platform in path: $($f.FullName)"; continue }

    $target = Join-Path (Join-Path (Join-Path $root 'tests\visual\baseline') $platform) $f.Name
    if ($DryRun)
    {
        Write-Host ("[DRY] {0} -> {1}" -f $f.FullName, $target)
    }
    else
    {
        Copy-Item $f.FullName $target -Force
        Write-Host ("[OK] {0} -> {1}" -f $f.Name, $platform)
    }
    $copied++
}
Write-Host ("[DONE] {0} capture(s) {1}" -f $copied, $(if ($DryRun) { 'listed' } else { 'copied' }))
Write-Host '[NEXT] git diff --stat tests/visual/baseline  ->  review, commit, push.'
exit 0
