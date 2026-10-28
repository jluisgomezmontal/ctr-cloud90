$ErrorActionPreference = 'Stop'
$ctrWork = Join-Path ([IO.Path]::GetTempPath()) ('cloud90-install-test-' + [Guid]::NewGuid().ToString('N'))
$ctrPortable = Join-Path $ctrWork 'portable'
$ctrPackage = Join-Path $ctrWork 'package'
New-Item -ItemType Directory -Force (Join-Path $ctrPortable 'bin'),(Join-Path $ctrPortable 'user'),$ctrPackage | Out-Null
try {
    [IO.File]::WriteAllText((Join-Path $ctrPortable 'run.py'),'original launcher')
    [IO.File]::WriteAllText((Join-Path $ctrPortable 'bin/sacredfire.exe'),'original runtime')
    [IO.File]::WriteAllText((Join-Path $ctrPortable 'user/save'),'original save')
    Copy-Item (Join-Path $PSScriptRoot '../packaging/windows/install-cloud90.ps1') $ctrPackage
    [IO.File]::WriteAllText((Join-Path $ctrPackage 'sacredfire-cloud90.exe'),'test runtime')
    [IO.File]::WriteAllText((Join-Path $ctrPackage 'cloud90_run.py'),'test wrapper')
    @{
        runtimeSha256 = (Get-FileHash (Join-Path $ctrPackage 'sacredfire-cloud90.exe')).Hash.ToLowerInvariant()
        wrapperSha256 = (Get-FileHash (Join-Path $ctrPackage 'cloud90_run.py')).Hash.ToLowerInvariant()
    } | ConvertTo-Json | Set-Content (Join-Path $ctrPackage 'cloud90-ctr.json')
    $ctrInstall = Join-Path $ctrPackage 'install-cloud90.ps1'
    function Get-Process { [PSCustomObject]@{ ProcessName = 'sacredfire' } }
    $ctrRefused = $false
    try { & $ctrInstall -PortablePath $ctrPortable } catch { $ctrRefused = $true }
    if (-not $ctrRefused -or (Test-Path (Join-Path $ctrPortable 'bin/sacredfire-cloud90.exe'))) { throw 'Running game was not protected' }
    function Get-Process { $null }
    & $ctrInstall -PortablePath $ctrPortable
    & $ctrInstall -PortablePath $ctrPortable
    # The ZIP is sometimes extracted directly into an existing portable.
    Copy-Item -LiteralPath $ctrInstall,(Join-Path $ctrPackage 'sacredfire-cloud90.exe'),(Join-Path $ctrPackage 'cloud90_run.py'),(Join-Path $ctrPackage 'cloud90-ctr.json') -Destination $ctrPortable
    & (Join-Path $ctrPortable 'install-cloud90.ps1') -PortablePath $ctrPortable
    [IO.File]::WriteAllText((Join-Path $ctrPortable 'cloud90_run.py'),'updated package wrapper')
    $ctrUpdated=Get-Content -Raw (Join-Path $ctrPortable 'cloud90-ctr.json') | ConvertFrom-Json
    $ctrUpdated.wrapperSha256=(Get-FileHash (Join-Path $ctrPortable 'cloud90_run.py')).Hash.ToLowerInvariant()
    $ctrUpdated | ConvertTo-Json | Set-Content (Join-Path $ctrPortable 'cloud90-ctr.json')
    & (Join-Path $ctrPortable 'install-cloud90.ps1') -PortablePath $ctrPortable
    if ([IO.File]::ReadAllText((Join-Path $ctrPortable 'cloud90_run.py')) -ne 'updated package wrapper') { throw 'Validated in-place wrapper update failed' }
    # Fresh installation from the portable must also recognize the validated
    # wrapper as package input, rather than an unknown installed module.
    $ctrFresh = Join-Path $ctrWork 'fresh-portable'
    New-Item -ItemType Directory -Force (Join-Path $ctrFresh 'bin') | Out-Null
    [IO.File]::WriteAllText((Join-Path $ctrFresh 'run.py'),'fresh launcher')
    [IO.File]::WriteAllText((Join-Path $ctrFresh 'bin/sacredfire.exe'),'fresh runtime')
    Copy-Item -LiteralPath $ctrInstall,(Join-Path $ctrPackage 'sacredfire-cloud90.exe'),(Join-Path $ctrPackage 'cloud90_run.py'),(Join-Path $ctrPackage 'cloud90-ctr.json') -Destination $ctrFresh
    & (Join-Path $ctrFresh 'install-cloud90.ps1') -PortablePath $ctrFresh
    if ([IO.File]::ReadAllText((Join-Path $ctrFresh 'bin/sacredfire-cloud90.exe')) -ne 'test runtime') { throw 'Fresh in-place install failed' }
    [IO.File]::WriteAllText((Join-Path $ctrPortable 'cloud90_run.py'),'user edit')
    $ctrRefused = $false
    try { & $ctrInstall -PortablePath $ctrPortable } catch { $ctrRefused = $true }
    if (-not $ctrRefused -or [IO.File]::ReadAllText((Join-Path $ctrPortable 'cloud90_run.py')) -ne 'user edit') { throw 'User edit was overwritten' }
    foreach ($ctrOriginal in @(@('run.py','original launcher'),@('bin/sacredfire.exe','original runtime'),@('user/save','original save'))) {
        if ([IO.File]::ReadAllText((Join-Path $ctrPortable $ctrOriginal[0])) -ne $ctrOriginal[1]) { throw 'Original file changed' }
    }
    'Cloud90 installer: running game, reinstall, user edits and original saves passed'
} finally {
    $ctrResolved = [IO.Path]::GetFullPath($ctrWork)
    $ctrTempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
    if (-not $ctrResolved.StartsWith($ctrTempRoot,[StringComparison]::OrdinalIgnoreCase) -or -not [IO.Path]::GetFileName($ctrResolved).StartsWith('cloud90-install-test-')) { throw 'Unsafe cleanup path' }
    Remove-Item -LiteralPath $ctrResolved -Recurse -Force
}
