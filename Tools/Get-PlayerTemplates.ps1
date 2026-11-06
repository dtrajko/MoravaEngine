<#
.SYNOPSIS
    Downloads CI's player templates, so this machine's editor builds games for other platforms too (Build Game, Platform).

.DESCRIPTION
    The Build workflow uploads a player template per platform from its Release builds (docs/engine/Game_Engine_Runtime_Guide.html,
    11.2): the player, ScriptCore, the private .NET and the license notices. This takes them from the latest successful run on a
    branch and puts each in PlayerTemplates/<platform> beside every editor build found (build/Debug, build/Release, build), where
    Build Game looks. Needs the GitHub CLI, logged in (gh auth login).

    A template belongs to the engine of its commit: download again after master changed, so the player matches the editor.

.PARAMETER Platform
    linux-x64 (default), win-x64, osx-arm64. Only linux-x64 is built from another platform's editor so far.

.PARAMETER Branch
    The branch whose latest successful run is used (default: master).

.PARAMETER RunId
    A run's ID instead (gh run list --workflow build.yml).

.PARAMETER Destination
    Editor build folders to put the template beside (default: those under build/ that have MoravaEngine).

.EXAMPLE
    Tools/Get-PlayerTemplates.ps1
    Tools/Get-PlayerTemplates.ps1 -Branch player-platforms
#>
param(
    [string]$Platform = 'linux-x64',
    [string]$Branch = 'master',
    [string]$RunId,
    [string[]]$Destination
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot

if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
    throw 'The GitHub CLI (gh) is needed: https://cli.github.com, then gh auth login'
}

# The workflow that makes the template: Linux and Windows in build.yml, macOS in macos.yml
$workflow = if ($Platform -like 'osx-*') { 'macos.yml' } else { 'build.yml' }
if (-not $RunId) {
    $runs = gh run list --repo dtrajko/MoravaEngine --workflow $workflow --branch $Branch --status success --limit 1 --json databaseId,headSha,createdAt | ConvertFrom-Json
    if (-not $runs) { throw "No successful $workflow run on '$Branch'" }
    $RunId = $runs[0].databaseId
    Write-Host "Run $RunId of $($runs[0].createdAt), commit $($runs[0].headSha.Substring(0, 8))"
}

if (-not $Destination) {
    $Destination = @('Debug', 'Release', 'RelWithDebInfo', '.') | ForEach-Object { Join-Path $repo "build/$_" } |
        Where-Object { (Test-Path (Join-Path $_ 'MoravaEngine.exe')) -or (Test-Path (Join-Path $_ 'MoravaEngine')) }
    if (-not $Destination) { throw "No editor build under $repo/build: build MoravaEngine first, or name a -Destination" }
}

$download = Join-Path ([IO.Path]::GetTempPath()) "MoravaPlayerTemplate-$Platform-$RunId"
if (Test-Path $download) { Remove-Item -Recurse -Force $download }
gh run download $RunId --repo dtrajko/MoravaEngine --name "player-template-$Platform" --dir $download
if ($LASTEXITCODE -ne 0) { throw "The run $RunId has no artifact player-template-$Platform (expired after 90 days, or the job failed)" }

foreach ($folder in $Destination) {
    $target = Join-Path $folder "PlayerTemplates/$Platform"
    if (Test-Path $target) { Remove-Item -Recurse -Force $target }
    New-Item -ItemType Directory -Force $target | Out-Null
    Copy-Item -Recurse -Force (Join-Path $download '*') $target
    Write-Host "Player template ${Platform}: $target"
}
Remove-Item -Recurse -Force $download
