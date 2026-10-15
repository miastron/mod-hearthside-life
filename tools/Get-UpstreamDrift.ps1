<#
.SYNOPSIS
    Lists what changed upstream since each Hearthside module was last verified.

.DESCRIPTION
    Reads the per-module table under "## Upstream baseline" in this repo's CLAUDE.md, fetches the
    local clones of the core (../azerothcore-wotlk-pb) and mod-playerbots (../mod-playerbots), and
    for each module prints the commit count from its verified SHA to the target, plus the changed
    files it likely depends on: files sharing a base name with a header the module includes
    (City and PvP also inherit Life's includes), and anything under mod-playerbots' conf/.

.PARAMETER Module
    Modules to report (life, chat, city, pvp). Default: all rows in the table.

.PARAMETER NoFetch
    Skip `git fetch`; compare against the remote-tracking refs as they are.

.PARAMETER Commits
    Also list the commits touching the matched files.

.EXAMPLE
    ./tools/Get-UpstreamDrift.ps1 -Module city -Commits
#>
[CmdletBinding()]
param(
    [string[]]$Module,
    [switch]$NoFetch,
    [switch]$Commits
)

$ErrorActionPreference = 'Stop'
$lifeRoot = Split-Path $PSScriptRoot -Parent
$suramar  = Split-Path $lifeRoot -Parent

$upstreams = [ordered]@{
    Core       = @{ Repo = Join-Path $suramar 'azerothcore-wotlk-pb'; Ref = 'origin/Playerbot' }
    Playerbots = @{ Repo = Join-Path $suramar 'mod-playerbots';       Ref = 'origin/master' }
}

# Parse "| city | `sha` | `sha` | verified | date |" rows from the baseline section.
$claude = Get-Content (Join-Path $lifeRoot 'CLAUDE.md')
$start = [array]::IndexOf($claude, '## Upstream baseline')
if ($start -lt 0) { throw 'No "## Upstream baseline" section in CLAUDE.md.' }
$rows = @{}
foreach ($line in $claude[($start + 1)..($claude.Count - 1)]) {
    if ($line -like '## *') { break }
    if ($line -match '^\|\s*(\w+)\s*\|\s*`([0-9a-f]{7,40})`\s*\|\s*`([0-9a-f]{7,40})`\s*\|\s*([^|]+?)\s*\|\s*([^|]+?)\s*\|') {
        $rows[$Matches[1]] = @{ Core = $Matches[2]; Playerbots = $Matches[3]; Verified = $Matches[4]; Date = $Matches[5] }
    }
}
if (-not $rows.Count) { throw 'No module rows parsed from the Upstream baseline table.' }
if (-not $Module) { $Module = 'life', 'chat', 'city', 'pvp' | Where-Object { $rows.ContainsKey($_) } }

if (-not $NoFetch) {
    foreach ($u in $upstreams.Values) { git -C $u.Repo fetch --quiet origin; if ($LASTEXITCODE) { throw "fetch failed: $($u.Repo)" } }
}

function Get-IncludeNames([string]$name) {
    $dirs = @(Join-Path $suramar "mod-hearthside-$name/src")
    if ($name -in 'city', 'pvp') { $dirs += Join-Path $lifeRoot 'src' }
    $names = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    Get-ChildItem $dirs -Recurse -File -Include *.h, *.hpp, *.cpp |
        Select-String -Pattern '#include\s+"([^"]+)"' -AllMatches |
        ForEach-Object { foreach ($m in $_.Matches) { [void]$names.Add([IO.Path]::GetFileNameWithoutExtension($m.Groups[1].Value)) } }
    $names
}

foreach ($name in $Module) {
    $row = $rows[$name]
    if (-not $row) { Write-Warning "No baseline row for '$name'."; continue }
    $includes = Get-IncludeNames $name
    Write-Host "`n=== $name (verified: $($row.Verified), $($row.Date))" -ForegroundColor Cyan

    foreach ($key in $upstreams.Keys) {
        $u = $upstreams[$key]
        $range = "$($row[$key])..$($u.Ref)"
        $count = [int](git -C $u.Repo rev-list --count $range)
        Write-Host "  $key $range : $count commit(s)"
        if (-not $count) { continue }

        $hits = git -C $u.Repo diff --name-status $range | ForEach-Object {
            $status, $path = $_ -split "`t", 2
            $path = ($path -split "`t")[-1]
            $base = [IO.Path]::GetFileNameWithoutExtension($path)
            if ($includes.Contains($base) -or ($key -eq 'Playerbots' -and $path -like 'conf/*')) { "    $status $path" }
        }
        if ($hits) { $hits } else { Write-Host '    (no changed files match this module''s includes)' }

        if ($Commits -and $hits) {
            $paths = $hits | ForEach-Object { ($_.Trim() -split '\s+', 2)[1] }
            git -C $u.Repo log --oneline $range -- $paths | ForEach-Object { "      $_" }
        }
    }
}
