# ============================================================================
# package_source.ps1 -- 打可分发源码 ZIP（打包逻辑唯一实现，Windows/Linux 共用）
# 用法：
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\package_source.ps1
#       [-OutputDir out\pack] [-Version 1.0.0]
# 行为：
#   1) 剥离内部资料/构建产物后复制到临时 stage 目录（robocopy，非 Windows 回退自写复制）
#   2) 泄漏校验：客户包不得含 git 痕迹
#   3) 7z 压缩（无 7z 回退 Compress-Archive），产物 out\pack\CarbonUI_v<版本>.zip
# ============================================================================
param(
    [string]$OutputDir = '',
    [string]$Version = ''
)

$ErrorActionPreference = 'Stop'

# ---- 工程根 = 本脚本所在目录的上一级 ---------------------------------------
$repoRoot = Split-Path -Parent $PSScriptRoot

# ---- 版本：参数 > Doxyfile 的 PROJECT_NUMBER > 0.0.0 ------------------------
if (-not $Version) {
    $doxyfile = Join-Path $repoRoot 'Doxyfile'
    if (Test-Path -LiteralPath $doxyfile) {
        $m = Select-String -LiteralPath $doxyfile -Pattern '^\s*PROJECT_NUMBER\s*=\s*"?([^"\r\n]+)"?' | Select-Object -First 1
        if ($m) { $Version = $m.Matches[0].Groups[1].Value.Trim() }
    }
}
if (-not $Version) { $Version = '0.0.0' }

# ---- 输出与 stage 目录 ------------------------------------------------------
if (-not $OutputDir) { $OutputDir = Join-Path $repoRoot 'out/pack' }
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

$topDirName = "CarbonUI_v$Version"
# stage 必须放在源码树之外（临时目录），否则输出目录在输入目录内会引发 robocopy 自递归。
# 临时目录必须跨平台取：Linux 上 $env:TEMP 为空（是 TMPDIR），直接 Join-Path 会因 Path 为 null 抛错。
$tempRoot = $env:TEMP
if (-not $tempRoot) { $tempRoot = $env:TMPDIR }
if (-not $tempRoot) { $tempRoot = [System.IO.Path]::GetTempPath() }
if (-not $tempRoot) { throw '无法确定临时目录（TEMP/TMPDIR 均为空）' }
$stageParent = Join-Path $tempRoot "carbonui_pack_stage_$PID"
$stageDir = Join-Path $stageParent $topDirName
if (Test-Path -LiteralPath $stageParent) { Remove-Item -LiteralPath $stageParent -Recurse -Force }
New-Item -ItemType Directory -Force -Path $stageDir | Out-Null

# ---- 排除规则 ---------------------------------------------------------------
$excludeDirs = @(
    '.git', '.github', '.vs', '.vscode', '.idea',
    'out', 'build', 'build-test', 'docs', 'lib',
    'cmake-build-debug', 'cmake-build-release',
    'Testing', 'CMakeFiles', '__pycache__', 'venv', 'ui-artifacts'
)
$excludeFiles = @(
    '.git*', '.gitignore', '.gitattributes', '.gitmodules',
    '*.log', '*.zip', '*.7z', '*.tar', '*.tar.gz', '*.tgz',
    '*.user', '*.suo', '*.pdb', '*.bak', '*.tmp', '*.swp',
    'Thumbs.db', '.DS_Store', '~$*'
)
# 内部脚本不进客户包（git_pull_latest.bat 内容含 git 命令，会被泄漏校验拦截）。
# 用 / 分隔：Windows 与 Linux 上 Join-Path 都能得到正确路径（反斜杠在 Linux 上是字面字符，会匹配失败）
$excludeAbsolute = @((Join-Path $repoRoot 'scripts/git_pull_latest.bat'))

# 开发专用目录不进客户包（WSL 联调脚本只服务本机预验证，客户拿到也没用）；
# 截图基线是内部视觉回归数据，客户环境没有对应的字体引擎，带了只会让测试全部 skip
$excludeAbsoluteDirs = @(
    (Join-Path $repoRoot 'scripts/wsl'),
    (Join-Path $repoRoot 'tests/visual/baseline')
)

# ---- 复制：优先 robocopy（Windows），否则自写复制 ---------------------------
function Copy-TreeWithExcludes {
    param($Source, $Dest, $ExcludeDirNames, $ExcludeFilePatterns, $ExcludeAbsolutePaths, $ExcludeAbsoluteDirs)

    foreach ($item in Get-ChildItem -LiteralPath $Source -Recurse -Force) {
        $rel = $item.FullName.Substring($Source.Length).TrimStart('\', '/')

        # 1) 绝对路径排除（内部脚本等）
        $skip = $false
        foreach ($abs in $ExcludeAbsolutePaths) {
            if ($item.FullName -eq $abs) { $skip = $true; break }
        }
        if ($skip) { continue }

        # 1b) 绝对目录及其子树排除（开发专用目录）
        foreach ($absDir in $ExcludeAbsoluteDirs) {
            if ($item.FullName -eq $absDir) { $skip = $true; break }
            foreach ($sep in @('\', '/')) {
                if ($item.FullName.StartsWith($absDir + $sep)) { $skip = $true; break }
            }
            if ($skip) { break }
        }
        if ($skip) { continue }

        # 2) 任意一级祖先目录命中排除名即跳过（对齐 robocopy /XD 语义）
        $segs = $rel -split '[\\/]'
        for ($s = 0; $s -lt $segs.Count - 1; $s++) {
            if ($ExcludeDirNames -contains $segs[$s]) { $skip = $true; break }
        }
        if ($skip) { continue }

        # 3) 目录名 / 文件名通配排除
        if ($item.PSIsContainer) {
            if ($ExcludeDirNames -contains $item.Name) { continue }
        } else {
            foreach ($pat in $ExcludeFilePatterns) {
                if ($item.Name -like $pat) { $skip = $true; break }
            }
        }
        if ($skip) { continue }

        $target = Join-Path $Dest $rel
        if ($item.PSIsContainer) {
            New-Item -ItemType Directory -Force -Path $target | Out-Null
        } else {
            $parent = Split-Path -Parent $target
            if (-not (Test-Path -LiteralPath $parent)) { New-Item -ItemType Directory -Force -Path $parent | Out-Null }
            Copy-Item -LiteralPath $item.FullName -Destination $target -Force
        }
    }
}

Write-Host '  -> 复制工程文件到 stage（排除内部资料与构建产物）...'
$robocopy = Get-Command robocopy -ErrorAction SilentlyContinue
if ($robocopy) {
    $robocopyArgs = @($repoRoot, $stageDir, '/E', '/NFL', '/NDL', '/NJH', '/NJS', '/NP')
    foreach ($d in $excludeDirs) { $robocopyArgs += @('/XD', $d) }
    foreach ($f in $excludeFiles) { $robocopyArgs += @('/XF', $f) }
    foreach ($a in $excludeAbsolute) { $robocopyArgs += @('/XF', $a) }
    foreach ($d in $excludeAbsoluteDirs) { $robocopyArgs += @('/XD', $d) }
    & robocopy @robocopyArgs | Out-Null
    if ($LASTEXITCODE -ge 8) { throw "复制工程文件失败（robocopy 退出码 $LASTEXITCODE）" }
} else {
    Write-Host '  未找到 robocopy（非 Windows），改用 PowerShell 复制'
    Copy-TreeWithExcludes -Source $repoRoot -Dest $stageDir `
        -ExcludeDirNames $excludeDirs `
        -ExcludeFilePatterns $excludeFiles `
        -ExcludeAbsolutePaths $excludeAbsolute `
        -ExcludeAbsoluteDirs $excludeAbsoluteDirs
}

# ---- 泄漏校验：客户包不得含 git 痕迹 ----------------------------------------
Write-Host '  -> 泄漏校验（git 痕迹）...'
$leaks = @()
foreach ($item in Get-ChildItem -LiteralPath $stageDir -Recurse -Force) {
    if ($item.Name -like '.git*') { $leaks += $item.FullName }
}
if ($leaks.Count -gt 0) { throw "发现 git 痕迹泄漏：" + ($leaks -join '; ') }

# ---- 压缩：7z 优先（探测不碰 ProgramFiles 之外的假设），回退 Compress-Archive
Write-Host '  -> 压缩...'
$zipPath = Join-Path $OutputDir "$topDirName.zip"
if (Test-Path -LiteralPath $zipPath) { Remove-Item -LiteralPath $zipPath -Force }

$isWindowsPs = (-not $PSVersionTable.Platform) -or ($PSVersionTable.Platform -eq 'Win32NT')
$sevenZip = $null
if ($isWindowsPs) {
    $sevenZip = (Get-Command 7z -ErrorAction SilentlyContinue).Source
    if (-not $sevenZip) {
        $candidates = @(
            (Join-Path $env:ProgramFiles '7-Zip\7z.exe'),
            'D:\autotools\7z\7za.exe'
        )
        foreach ($c in $candidates) { if ($c -and (Test-Path -LiteralPath $c)) { $sevenZip = $c; break } }
    }
} else {
    $sevenZip = (Get-Command 7z -ErrorAction SilentlyContinue).Source
    if (-not $sevenZip) { $sevenZip = (Get-Command 7za -ErrorAction SilentlyContinue).Source }
}

if ($sevenZip) {
    & $sevenZip a -tzip $zipPath (Join-Path $stageDir '*') | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "压缩失败（7z 退出码 $LASTEXITCODE）" }
} else {
    Write-Host '  未找到 7z，回退 Compress-Archive'
    Compress-Archive -Path $stageDir -DestinationPath $zipPath -Force
}

# ---- 清理 stage、校验产物 ----------------------------------------------------
Remove-Item -LiteralPath $stageParent -Recurse -Force
if (-not (Test-Path -LiteralPath $zipPath)) { throw "压缩产物缺失：$zipPath" }
$size = (Get-Item -LiteralPath $zipPath).Length
if ($size -lt 1KB) { throw "压缩产物过小（$size 字节），疑似打包失败：$zipPath" }

Write-Host ''
Write-Host "[OK] 源码分发包：$zipPath（$([math]::Round($size / 1KB, 1)) KB，版本 $Version）"
exit 0
