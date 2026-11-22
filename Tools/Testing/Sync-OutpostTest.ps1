# Brings the Outpost test copy up to date with Projects/Outpost: assets, settings, levels, scripts and the Greybox scene.
# The copy keeps its own project file (name OutpostPlayerTest: its own settings, a window) and its own test scripts
# (PlayerTest.cs beside the game's), and its Greybox gets the test's two entities: the low slab the crouch test walks
# under (TestSlab) and the test driver (PlayerTest), which drives the player through a timed list of commands and logs
# PASS/FAIL (Run-Engine.ps1 counts them).
#
#   .\Tools\Testing\Sync-OutpostTest.ps1 [-Copy D:\Workspace\Temp\OutpostPlayerTest]
#
# Then build its scripts and run it in the editor with Play:
#
#   dotnet build <copy>\Scripts\Outpost.csproj -p:MoravaScriptCoreDir=D:\Workspace\MoravaEngine\build\Debug\
#   .\Tools\Testing\Run-Engine.ps1 -Editor -Play -Project <copy>\Outpost.mproject -Seconds 145 -DebugOverlay
#
# One section only (the README): $env:OUTPOST_TEST_START = "Sidearms", "Sniper" or "Sights" before the run
param(
    [string]$Copy = "D:\Workspace\Temp\OutpostPlayerTest"
)
$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$source = Join-Path $repo "Projects\Outpost"
if (-not (Test-Path (Join-Path $Copy "Outpost.mproject")) -or -not (Test-Path (Join-Path $Copy "Scripts\PlayerTest.cs")))
{
    throw "No test copy at '$Copy' (Outpost.mproject with Scripts\PlayerTest.cs)"
}
if ((Get-Content (Join-Path $Copy "Outpost.mproject") -Raw) -notmatch "Name: OutpostPlayerTest")
{
    throw "The copy's project must be named OutpostPlayerTest (else it shares the game's settings)"
}

foreach ($folder in "Assets", "Settings", "Levels")
{
    if (Test-Path (Join-Path $source $folder))
    {
        # /MIR: the copy's folder as the project's (files removed there go here too); robocopy's exit codes below 8 are success
        robocopy (Join-Path $source $folder) (Join-Path $Copy $folder) /MIR /NFL /NDL /NJH /NJS /NP | Out-Null
        if ($LASTEXITCODE -ge 8) { throw "robocopy $folder failed ($LASTEXITCODE)" }
    }
}
# The game's scripts, not their build output nor the copy's test scripts (no /MIR: PlayerTest.cs stays)
robocopy (Join-Path $source "Scripts") (Join-Path $Copy "Scripts") *.cs /S /XD bin obj /NFL /NDL /NJH /NJS /NP | Out-Null
if ($LASTEXITCODE -ge 8) { throw "robocopy Scripts failed ($LASTEXITCODE)" }

# The scene: the game's, with the test's entities added at the end of its list
$test = @"
  - ID: 1901
    Name: TestSlab
    Parent: 0
    Transform:
      Translation: [3.25, 1.5, 11.5]
      Rotation: [0, 0, 0]
      Scale: [1, 1, 1]
    Collider:
      Shape: Box
      Offset: [0, 0, 0]
      HalfExtents: [1.25, 0.1, 3]
      Radius: 1
      HalfHeight: 0
      Friction: 0.5
      Restitution: 0
      IsTrigger: false
  - ID: 1902
    Name: PlayerTest
    Parent: 0
    Transform:
      Translation: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [1, 1, 1]
    Script:
      Class: Outpost.PlayerTest
"@
$scene = [System.IO.File]::ReadAllText((Join-Path $source "Scenes\Greybox.mscene"))
if ($scene.Contains("Name: PlayerTest")) { throw "The game's Greybox already has a PlayerTest" }
$scene = $scene.TrimEnd() + "`r`n" + ($test -replace "`r?`n", "`r`n") + "`r`n"
New-Item -ItemType Directory -Force -Path (Join-Path $Copy "Scenes") | Out-Null
[System.IO.File]::WriteAllText((Join-Path $Copy "Scenes\Greybox.mscene"), $scene)
Write-Host "OutpostPlayerTest synced from $source (Greybox with TestSlab and PlayerTest)"
exit 0
