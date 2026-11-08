# Builds dist\MHW-SteamOffline-<version>.zip, laid out to be extracted into the game folder.
$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..')
$build = Join-Path $root 'build'
$version = (Select-String -LiteralPath (Join-Path $root 'CMakeLists.txt') -Pattern 'project\(MHW-SteamOffline VERSION ([0-9.]+)').Matches[0].Groups[1].Value
$stage = Join-Path $root 'dist\stage'
$zip = Join-Path $root "dist\MHW-SteamOffline-$version.zip"

if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
$plugins = New-Item -ItemType Directory -Force (Join-Path $stage 'nativePC\plugins')
Copy-Item (Join-Path $build 'MHW-SteamOffline.dll') $plugins
Copy-Item (Join-Path $root 'README.md') (Join-Path $stage 'MHW-SteamOffline-README.md')
Copy-Item (Join-Path $root 'LICENSE') (Join-Path $stage 'MHW-SteamOffline-LICENSE.txt')

New-Item -ItemType Directory -Force (Split-Path $zip) | Out-Null
if (Test-Path $zip) { Remove-Item $zip }
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip
Remove-Item -Recurse -Force $stage
Get-Item $zip | Select-Object FullName, Length
