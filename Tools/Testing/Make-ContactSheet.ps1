<#
.SYNOPSIS
    Puts images side by side in one PNG (a contact sheet): a clip's preview frames, a series of captures.

.EXAMPLE
    .\Make-ContactSheet.ps1 -Pattern "D:\Workspace\Temp\OutpostPoses\sprint\Shuffle_Left_Loop_front_*.png" -Columns 9 -Out Output\shuffle.png

.EXAMPLE
    # Each image cropped to a part of it (x, y, width, height) and scaled down by half
    .\Make-ContactSheet.ps1 -Pattern "Output\frame_*.png" -Crop 400,100,800,600 -Scale 0.5 -Out Output\frames.png
#>
param(
    [Parameter(Mandatory = $true)] [string] $Pattern,   # a wildcard path; the files are taken in name order
    [Parameter(Mandatory = $true)] [string] $Out,
    [int] $Columns = 8,
    [int[]] $Crop,                                      # x, y, width, height of each image (none: all of it)
    [double] $Scale = 1.0,
    [switch] $Labels                                    # each image's file name under it
)

Add-Type -AssemblyName System.Drawing
$files = @(Get-ChildItem -Path $Pattern | Sort-Object Name)
if ($files.Count -eq 0) { Write-Error "No files match $Pattern"; exit 1 }

$first = [System.Drawing.Image]::FromFile($files[0].FullName)
$sourceWidth = if ($Crop) { $Crop[2] } else { $first.Width }
$sourceHeight = if ($Crop) { $Crop[3] } else { $first.Height }
$first.Dispose()
$cellWidth = [int]($sourceWidth * $Scale)
$cellHeight = [int]($sourceHeight * $Scale)
$labelHeight = if ($Labels) { 16 } else { 0 }
$cols = [Math]::Min($Columns, $files.Count)
$rows = [Math]::Ceiling($files.Count / $cols)

$sheet = New-Object System.Drawing.Bitmap ($cellWidth * $cols), (($cellHeight + $labelHeight) * $rows)
$graphics = [System.Drawing.Graphics]::FromImage($sheet)
$graphics.Clear([System.Drawing.Color]::White)
$graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$font = New-Object System.Drawing.Font "Segoe UI", 8
for ($i = 0; $i -lt $files.Count; $i++) {
    $image = [System.Drawing.Image]::FromFile($files[$i].FullName)
    $source = if ($Crop) { New-Object System.Drawing.Rectangle $Crop[0], $Crop[1], $Crop[2], $Crop[3] } else { New-Object System.Drawing.Rectangle 0, 0, $image.Width, $image.Height }
    $left = ($i % $cols) * $cellWidth
    $top = [Math]::Floor($i / $cols) * ($cellHeight + $labelHeight)
    $graphics.DrawImage($image, (New-Object System.Drawing.Rectangle $left, $top, $cellWidth, $cellHeight), $source, [System.Drawing.GraphicsUnit]::Pixel)
    if ($Labels) { $graphics.DrawString($files[$i].BaseName, $font, [System.Drawing.Brushes]::Black, $left + 2, $top + $cellHeight) }
    $image.Dispose()
}
$Out = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Out) # relative to PowerShell's folder, not .NET's
$folder = Split-Path -Parent $Out
if (-not (Test-Path $folder)) { New-Item -ItemType Directory -Path $folder | Out-Null }
$sheet.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
$graphics.Dispose(); $sheet.Dispose()
Write-Output "$($files.Count) images -> $Out ($($cols) x $($rows))"
