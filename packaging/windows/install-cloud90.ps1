param([Parameter(Mandatory=$true)][string]$PortablePath)
$ErrorActionPreference='Stop'
$ctrPortable=[IO.Path]::GetFullPath($PortablePath)
if (-not (Test-Path -LiteralPath (Join-Path $ctrPortable 'run.py')) -or
    -not (Test-Path -LiteralPath (Join-Path $ctrPortable 'bin\sacredfire.exe'))) {
    throw 'Elige la carpeta del portable con run.py y bin\sacredfire.exe, no los datos PS4.'
}
if (Get-Process sacredfire,sacredfire-cloud90 -ErrorAction SilentlyContinue) { throw 'Cierra CTR antes de instalar.' }
$ctrPackage=Get-Content -LiteralPath (Join-Path $PSScriptRoot 'cloud90-ctr.json') -Raw | ConvertFrom-Json
$ctrSource=Join-Path $PSScriptRoot 'sacredfire-cloud90.exe'
if ((Get-FileHash -LiteralPath $ctrSource -Algorithm SHA256).Hash.ToLowerInvariant() -ne $ctrPackage.runtimeSha256) { throw 'El paquete no coincide con su manifiesto.' }
if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'cloud90_run.py') -Algorithm SHA256).Hash.ToLowerInvariant() -ne $ctrPackage.wrapperSha256) { throw 'El iniciador no coincide con su manifiesto.' }
$ctrManifest=Join-Path $ctrPortable 'bin\cloud90-ctr.json'
$ctrRuntime=Join-Path $ctrPortable 'bin\sacredfire-cloud90.exe'
$ctrWrapper=Join-Path $ctrPortable 'cloud90_run.py'
# Refuse to overwrite files belonging to a different package or a user edit.
if ((Test-Path -LiteralPath $ctrRuntime) -or (Test-Path -LiteralPath $ctrWrapper)) {
    if (-not (Test-Path -LiteralPath $ctrManifest)) { throw 'Hay archivos Cloud90 sin manifiesto; consérvalos antes de instalar.' }
    $ctrOld=Get-Content -LiteralPath $ctrManifest -Raw | ConvertFrom-Json
    foreach ($ctrPair in @(@($ctrRuntime,$ctrOld.runtimeSha256),@($ctrWrapper,$ctrOld.wrapperSha256))) {
        if (-not (Test-Path -LiteralPath $ctrPair[0]) -or (Get-FileHash -LiteralPath $ctrPair[0] -Algorithm SHA256).Hash.ToLowerInvariant() -ne $ctrPair[1]) {
            throw 'El módulo instalado cambió; no se sobrescribirá automáticamente.'
        }
    }
}
$ctrBackup=@{}
foreach ($ctrDestination in @($ctrRuntime,$ctrWrapper,$ctrManifest)) {
    if (Test-Path -LiteralPath $ctrDestination) { $ctrBackup[$ctrDestination]=[IO.File]::ReadAllBytes($ctrDestination) }
}
try {
    Copy-Item -LiteralPath $ctrSource -Destination $ctrRuntime -Force
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'cloud90_run.py') -Destination $ctrWrapper -Force
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'cloud90-ctr.json') -Destination $ctrManifest -Force
} catch {
    foreach ($ctrDestination in @($ctrRuntime,$ctrWrapper,$ctrManifest)) {
        if ($ctrBackup.ContainsKey($ctrDestination)) { [IO.File]::WriteAllBytes($ctrDestination,$ctrBackup[$ctrDestination]) }
        elseif (Test-Path -LiteralPath $ctrDestination) { Remove-Item -LiteralPath $ctrDestination -Force }
    }
    throw
}
Write-Output 'CTR Cloud90 instalado. Abre Cloud90 Game Link y elige esta carpeta en la pestaña CTR Nitro-Fueled.'
