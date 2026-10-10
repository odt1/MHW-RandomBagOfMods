# Copies the built plugin into the game for testing, or removes it again (game must be closed).
#   deploy.ps1          install build\MHW-SteamOffline.dll and .pdb
#   deploy.ps1 -Remove  delete them again (the log stays)
param(
    [string]$GameDir = 'F:\SteamLibrary\steamapps\common\Monster Hunter World',
    [switch]$Remove
)
$ErrorActionPreference = 'Stop'
$plugins = Join-Path $GameDir 'nativePC\plugins'

if ($Remove) {
    foreach ($p in @("$plugins\MHW-SteamOffline.dll", "$plugins\MHW-SteamOffline.pdb")) {
        if (Test-Path -LiteralPath $p) { Remove-Item -LiteralPath $p -Force; "removed $p" }
    }
    return
}

$build = Join-Path $PSScriptRoot '..\build'
Copy-Item -LiteralPath (Join-Path $build 'MHW-SteamOffline.dll') $plugins -Force
Copy-Item -LiteralPath (Join-Path $build 'MHW-SteamOffline.pdb') $plugins -Force
"installed into $plugins"
