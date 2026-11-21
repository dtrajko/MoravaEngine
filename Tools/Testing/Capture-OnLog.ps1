<#
.SYNOPSIS
    Captures a window each time a line matching a pattern appears in a log: a test driver's moments (a freeze for a
    photo, a pose to see) photographed as they happen.

.DESCRIPTION
    Watches the log file (as the engine writes it, line by line) until the process ends or -Count captures are taken.
    Each match waits -Delay seconds (the frame shown), then saves the window as <Prefix>_<n>.png in -Folder. Start it
    in the background before the run, or beside Run-Engine.ps1:

.EXAMPLE
    Start-Job { & D:\Workspace\MoravaEngine\Tools\Testing\Capture-OnLog.ps1 -Name MoravaEngine -Pattern "PlayerTest: photo" -Prefix getup }
    .\Run-Engine.ps1 -Editor -Play -Project D:\Workspace\Temp\OutpostPlayerTest\Outpost.mproject -Seconds 64
#>
param(
    [string]$Log = (Join-Path $PSScriptRoot "Output\MoravaEngine.log"),
    [string]$Name = "MoravaEngine",              # the process whose window is captured (Capture-Window.ps1's -Name)
    [Parameter(Mandatory)][string]$Pattern,      # a regular expression
    [string]$Folder = (Join-Path $PSScriptRoot "Output"),
    [string]$Prefix = "photo",
    [double]$Delay = 0.4,
    [int]$Count = 20,
    [int]$Timeout = 300,                          # seconds to wait in all
    [int[]]$Crop
)
$ErrorActionPreference = "Stop"
$started = Get-Date
# Wait for the run to start a new log (an old one may be there from the last run)
$since = $started.AddSeconds(-1)
while (-not (Test-Path $Log) -or (Get-Item $Log).LastWriteTime -lt $since)
{
    if (((Get-Date) - $started).TotalSeconds -gt 60) { throw "No new log at $Log" }
    Start-Sleep -Milliseconds 200
}
$read = 0
$taken = 0
while ($taken -lt $Count -and ((Get-Date) - $started).TotalSeconds -lt $Timeout)
{
    $lines = @()
    try
    {
        $stream = [System.IO.File]::Open($Log, "Open", "Read", "ReadWrite")
        $reader = New-Object System.IO.StreamReader $stream
        $lines = $reader.ReadToEnd() -split "`r?`n"
        $reader.Close()
    }
    catch { Start-Sleep -Milliseconds 100; continue }
    for ($i = $read; $i -lt $lines.Count - 1; $i++)
    {
        if ($lines[$i] -match $Pattern)
        {
            Start-Sleep -Seconds $Delay
            $taken++
            $out = Join-Path $Folder ("{0}_{1}.png" -f $Prefix, $taken)
            $arguments = @{ Name = $Name; Out = $out }
            if ($Crop) { $arguments.Crop = $Crop }
            & (Join-Path $PSScriptRoot "Capture-Window.ps1") @arguments | Out-Null
            Write-Output "$out  <- $($lines[$i].Trim())"
        }
    }
    $read = [Math]::Max($read, $lines.Count - 1)
    if (-not (Get-Process -Name $Name -ErrorAction SilentlyContinue) -and ((Get-Date) - $started).TotalSeconds -gt 30) { break }
    Start-Sleep -Milliseconds 100
}
