<#
.SYNOPSIS
    Saves a window of a running process as a PNG, optionally a cropped and enlarged part of it.

.DESCRIPTION
    Uses PrintWindow (flag 2: the window's own rendering), so the window may be covered by others. The image is the whole
    window, title bar included, at its size on screen in pixels.

.EXAMPLE
    .\Capture-Window.ps1 -Name MoravaPlayer -Out player.png
.EXAMPLE
    .\Capture-Window.ps1 -Id 1234 -Out graph.png -Crop 972,86,302,46 -Zoom 4
#>
param(
    [int]$Id = 0,                   # the process (else -Name)
    [string]$Name = "MoravaPlayer", # the newest process of this name with a window
    [Parameter(Mandatory)][string]$Out,
    [int[]]$Crop,                   # x, y, width, height in the window's pixels
    [int]$Zoom = 1                  # the cropped part enlarged this many times, pixels kept sharp
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing
if (-not ("MoravaCapture" -as [type]))
{
    Add-Type @"
using System; using System.Runtime.InteropServices;
public class MoravaCapture
{
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr window, IntPtr dc, uint flags);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr window, out Rect rect);
    public struct Rect { public int Left, Top, Right, Bottom; }
}
"@
}

$process = if ($Id) { Get-Process -Id $Id } else
{
    Get-Process $Name | Where-Object { $_.MainWindowHandle -ne 0 } | Sort-Object StartTime | Select-Object -Last 1
}
if (-not $process -or $process.MainWindowHandle -eq 0) { throw "No window to capture (process $Id$Name)" }

$rect = New-Object MoravaCapture+Rect
[MoravaCapture]::GetWindowRect($process.MainWindowHandle, [ref]$rect) | Out-Null
$window = New-Object System.Drawing.Bitmap ($rect.Right - $rect.Left), ($rect.Bottom - $rect.Top)
$graphics = [System.Drawing.Graphics]::FromImage($window)
$dc = $graphics.GetHdc()
[MoravaCapture]::PrintWindow($process.MainWindowHandle, $dc, 2) | Out-Null
$graphics.ReleaseHdc($dc)
$graphics.Dispose()

$image = $window
if ($Crop)
{
    $part = New-Object System.Drawing.Rectangle $Crop[0], $Crop[1], $Crop[2], $Crop[3]
    $image = New-Object System.Drawing.Bitmap ($part.Width * $Zoom), ($part.Height * $Zoom)
    $graphics = [System.Drawing.Graphics]::FromImage($image)
    $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
    $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
    $graphics.DrawImage($window, (New-Object System.Drawing.Rectangle 0, 0, $image.Width, $image.Height), $part,
        [System.Drawing.GraphicsUnit]::Pixel)
    $graphics.Dispose()
    $window.Dispose()
}
if (-not [System.IO.Path]::IsPathRooted($Out)) { $Out = Join-Path (Get-Location) $Out }
$image.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
"Saved $Out ($($image.Width)x$($image.Height))"
$image.Dispose()
