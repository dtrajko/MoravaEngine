<#
.SYNOPSIS
    Runs the editor or the player unattended for a while: its log in a file, a screenshot of its window, and a summary of
    what went wrong (Vulkan validation messages, script errors, files that couldn't be loaded).

.DESCRIPTION
    Starts build\<Config>\MoravaPlayer.exe or MoravaEngine.exe from the repository's root (the engine's working
    directory) with --exit-after, waits until it closes and prints the summary. The log goes to a file through cmd's
    redirection: PowerShell's own (Start-Process -RedirectStandardOutput) slows the engine down to a crawl.

    Output (logs, screenshots) goes to Tools\Testing\Output unless -Log or -Capture give a path; that folder is ignored
    by Git. Relative -Project and -Scene paths are resolved from the current folder.

    A project with Fullscreen: true takes over the screen: test with a windowed copy (see the README).

.EXAMPLE
    .\Run-Engine.ps1 -Project ..\..\Projects\Sample\Sample.mproject -Scene ..\..\Projects\Sample\Scenes\Erika.mscene -DebugOverlay -Capture erika.png
.EXAMPLE
    .\Run-Engine.ps1 -Editor -Project D:\Workspace\Temp\Sample\Sample.mproject -Play -Maximize -Seconds 40 -Capture editor.png
#>
param(
    [switch]$Editor,               # the editor (MoravaEngine.exe); else the player
    [string]$Project,              # --project
    [string]$Scene,                # --scene
    [int]$Seconds = 20,            # --exit-after: the time in the main loop, after the startup
    [switch]$Play,                 # the editor: --play
    [switch]$Simulate,             # the editor: --simulate
    [switch]$DebugOverlay,         # --debug-overlay (F3)
    [switch]$Maximize,             # maximizes the window once it is shown
    [string]$Capture,              # a screenshot of the window, saved as this PNG
    [int]$CaptureAfter = 0,        # seconds after the window appeared (0: three seconds before it closes)
    [int[]]$Crop,                  # x, y, width, height of the screenshot to keep (see Capture-Window.ps1)
    [int]$Zoom = 1,
    [string]$Config = "Debug",
    [string]$Log,                  # the log file (default: Output\<exe>.log)
    [string[]]$Extra = @()         # further options, as they are passed (e.g. --no-ray-tracing, --debug-view, 8)
)

$ErrorActionPreference = "Stop"
$repo = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$outputFolder = Join-Path $PSScriptRoot "Output"
New-Item -ItemType Directory -Force $outputFolder | Out-Null

function Get-FullPath([string]$path, [string]$base)
{
    if ([System.IO.Path]::IsPathRooted($path)) { return $path }
    return [System.IO.Path]::GetFullPath((Join-Path $base $path))
}

$exeName = if ($Editor) { "MoravaEngine" } else { "MoravaPlayer" }
$exe = Join-Path $repo "build\$Config\$exeName.exe"
if (-not (Test-Path $exe)) { throw "Not built: $exe" }
$Log = if ($Log) { Get-FullPath $Log (Get-Location) } else { Join-Path $outputFolder "$exeName.log" }

$arguments = @()
if ($Project) { $arguments += "--project", "`"$(Get-FullPath $Project (Get-Location))`"" }
if ($Scene) { $arguments += "--scene", "`"$(Get-FullPath $Scene (Get-Location))`"" }
if ($Play) { $arguments += "--play" }
if ($Simulate) { $arguments += "--simulate" }
if ($DebugOverlay) { $arguments += "--debug-overlay" }
$arguments += "--exit-after", $Seconds
$arguments += $Extra

# The engine as cmd's child, its output redirected by cmd
$started = Get-Date
$command = "`"`"$exe`" $($arguments -join ' ') > `"$Log`" 2>&1`""
"Running $exeName $($arguments -join ' ')"
$shell = Start-Process cmd.exe -ArgumentList "/c", $command -WorkingDirectory $repo -WindowStyle Hidden -PassThru

# The engine's own process: the newest of its name started after this script started it
$engine = $null
for ($i = 0; $i -lt 100 -and -not $engine; $i++)
{
    Start-Sleep -Milliseconds 200
    $engine = Get-Process $exeName -ErrorAction SilentlyContinue | Where-Object { $_.StartTime -ge $started } |
        Sort-Object StartTime | Select-Object -Last 1
}
if (-not $engine) { throw "$exeName didn't start (see $Log)" }

# Its window, then maximized and captured while it runs
if ($Maximize -or $Capture)
{
    for ($i = 0; $i -lt 300 -and $engine.MainWindowHandle -eq 0 -and -not $engine.HasExited; $i++)
    {
        Start-Sleep -Milliseconds 200
        $engine.Refresh()
    }
}
$windowShown = Get-Date
if ($Maximize -and -not $engine.HasExited)
{
    if (-not ("MoravaWindow" -as [type]))
    {
        Add-Type @"
using System; using System.Runtime.InteropServices;
public class MoravaWindow { [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr window, int command); }
"@
    }
    [MoravaWindow]::ShowWindow($engine.MainWindowHandle, 3) | Out-Null # SW_MAXIMIZE
}
if ($Capture)
{
    # --exit-after counts the main loop's time, which starts about when the window appears
    $after = if ($CaptureAfter -gt 0) { $CaptureAfter } else { [Math]::Max($Seconds - 3, 1) }
    $wait = $after - ((Get-Date) - $windowShown).TotalSeconds
    if ($wait -gt 0) { Start-Sleep -Seconds $wait }
    if ($engine.HasExited) { Write-Warning "$exeName closed before the screenshot" }
    else
    {
        $capturePath = if ([System.IO.Path]::IsPathRooted($Capture) -or $Capture.Contains("\") -or $Capture.Contains("/")) {
            Get-FullPath $Capture (Get-Location) } else { Join-Path $outputFolder $Capture }
        $captureArguments = @{ Id = $engine.Id; Out = $capturePath; Zoom = $Zoom }
        if ($Crop) { $captureArguments.Crop = $Crop }
        & (Join-Path $PSScriptRoot "Capture-Window.ps1") @captureArguments
    }
}

$shell.WaitForExit()
$elapsed = ((Get-Date) - $started).TotalSeconds

# The summary
$lines = Get-Content $Log
$validation = @($lines | Select-String "VulkanDebugCallback")
$problems = @($lines | Select-String -Pattern "Script error|self-test failed|could not be opened|could not be loaded|Could not load texture|Failed to load|doesn't exist|exception" |
    Where-Object { $_.Line -notmatch "Could not find uniform" })
"{0} ran {1:N0} s, log: {2} ({3} lines)" -f $exeName, $elapsed, $Log, $lines.Count
"Vulkan validation messages: $($validation.Count)"
$validation | Select-Object -First 5 | ForEach-Object { "  $($_.Line)" }
"Problems: $($problems.Count)"
$problems | Select-Object -First 10 | ForEach-Object { "  $($_.Line)" }
$lines | Select-String -Pattern "^\[.*\] MORAVA: (Stop:|Game loop:|Closing after)" | ForEach-Object { $_.Line }
if ($validation.Count -gt 0 -or $problems.Count -gt 0) { exit 1 }
exit 0
