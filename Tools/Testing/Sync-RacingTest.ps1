# Brings the Racing test copy up to date with Projects/Racing: scenes, scripts, assets, settings and tracks. The copy keeps
# its own project file (name RacingTest: its own settings and save data, a window, not fullscreen), and its Circuit scene
# gets the test driver (CarController.TestDriver), which drives laps on its own.
#
#   .\Tools\Testing\Sync-RacingTest.ps1 [-Copy D:\Workspace\Temp\RacingTest] [-NoTestDriver]
#
# Then build its pack (MoravaEngine --build-pack <copy>\RacingTest.mproject) or its game (--build-game), and run it.
param(
    [string]$Copy = "D:\Workspace\Temp\RacingTest",
    [switch]$NoTestDriver
)
$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$source = Join-Path $repo "Projects\Racing"
if (-not (Test-Path (Join-Path $Copy "RacingTest.mproject")))
{
    throw "No test copy at '$Copy' (RacingTest.mproject): make one by copying Projects\Racing and renaming its project"
}

foreach ($folder in "Assets", "Scenes", "Settings", "Tracks", "Foliage")
{
    if (Test-Path (Join-Path $source $folder))
    {
        # /MIR: the copy's folder as the project's (files removed there go here too); robocopy's exit codes below 8 are success
        robocopy (Join-Path $source $folder) (Join-Path $Copy $folder) /MIR /NFL /NDL /NJH /NJS /NP | Out-Null
        if ($LASTEXITCODE -ge 8) { throw "robocopy $folder failed ($LASTEXITCODE)" }
    }
}
# The scripts, not their build output (bin, obj) nor the copy's own test scripts beside them. Not the script project: the
# copy's builds RacingTest.dll (its project's Script Assembly)
robocopy (Join-Path $source "Scripts") (Join-Path $Copy "Scripts") *.cs /S /XD bin obj /NFL /NDL /NJH /NJS /NP | Out-Null
if ($LASTEXITCODE -ge 8) { throw "robocopy Scripts failed ($LASTEXITCODE)" }

if (-not $NoTestDriver)
{
    $circuit = Join-Path $Copy "Scenes\Circuit.mscene"
    $text = [System.IO.File]::ReadAllText($circuit)
    $marker = "      Class: Racing.CarController`r`n      Fields:`r`n"
    if (-not $text.Contains($marker)) { throw "Circuit.mscene: CarController's fields not found" }
    if (-not $text.Contains($marker + "        TestDriver: {Bool: true}"))
    {
        $text = $text.Replace($marker, $marker + "        TestDriver: {Bool: true}`r`n")
        [System.IO.File]::WriteAllText($circuit, $text)
    }
}
Write-Host "RacingTest synced from $source$(if (-not $NoTestDriver) { ' (test driver on in Circuit)' })"

exit 0
