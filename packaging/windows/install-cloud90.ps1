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
function Test-CtrSamePath([string]$Source,[string]$Destination) {
    return [string]::Equals([IO.Path]::GetFullPath($Source),[IO.Path]::GetFullPath($Destination),[StringComparison]::OrdinalIgnoreCase)
}
function Copy-CtrPackageFile([string]$Source,[string]$Destination) {
    if (-not (Test-CtrSamePath $Source $Destination)) {
        Copy-Item -LiteralPath $Source -Destination $Destination -Force
    }
}
$ctrWrapperIsPackage=Test-CtrSamePath (Join-Path $PSScriptRoot 'cloud90_run.py') $ctrWrapper
# Refuse to overwrite files belonging to a different package or a user edit.
if ((Test-Path -LiteralPath $ctrRuntime) -or ((Test-Path -LiteralPath $ctrWrapper) -and -not $ctrWrapperIsPackage)) {
    if (-not (Test-Path -LiteralPath $ctrManifest)) { throw 'Hay archivos Cloud90 sin manifiesto; consérvalos antes de instalar.' }
    $ctrOld=Get-Content -LiteralPath $ctrManifest -Raw | ConvertFrom-Json
    foreach ($ctrPair in @(@($ctrRuntime,$ctrOld.runtimeSha256),@($ctrWrapper,$ctrOld.wrapperSha256))) {
        # When extracted into the portable, this is the already validated
        # package wrapper. The installer neither copies nor changes that file.
        if ($ctrWrapperIsPackage -and (Test-CtrSamePath $ctrPair[0] $ctrWrapper)) { continue }
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
    Copy-CtrPackageFile $ctrSource $ctrRuntime
    Copy-CtrPackageFile (Join-Path $PSScriptRoot 'cloud90_run.py') $ctrWrapper
    Copy-CtrPackageFile (Join-Path $PSScriptRoot 'cloud90-ctr.json') $ctrManifest
} catch {
    foreach ($ctrDestination in @($ctrRuntime,$ctrWrapper,$ctrManifest)) {
        if ($ctrBackup.ContainsKey($ctrDestination)) { [IO.File]::WriteAllBytes($ctrDestination,$ctrBackup[$ctrDestination]) }
        elseif (Test-Path -LiteralPath $ctrDestination) { Remove-Item -LiteralPath $ctrDestination -Force }
    }
    throw
}
Write-Output 'CTR Cloud90 instalado. Abre Cloud90 Game Link y elige esta carpeta en la pestaña CTR Nitro-Fueled.'
