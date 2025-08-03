param(
    [switch]$Zip
)

# === CONFIG ===
$projectRoot = "$PSScriptRoot"
$exeName     = "SerialTerminalApp.exe"
$qtBinPath   = "C:\Qt\6.9.1\mingw_64\bin"  # your Qt path
$version     = "v1.0.0"                    # change if versioning
$timestamp   = Get-Date -Format "yyyyMMdd_HHmmss"
$targetDir   = Join-Path $projectRoot "SerialTerminal"
$zipName     = "SerialTerminal_${version}_${timestamp}.zip"
$zipPath     = Join-Path $projectRoot $zipName

# === CLEAN TARGET FOLDER ===
if (Test-Path $targetDir) {
    Remove-Item -Recurse -Force $targetDir
}
New-Item -ItemType Directory -Path $targetDir | Out-Null

# === COPY EXE FIRST ===
$buildDir = Get-ChildItem -Directory "$projectRoot\build" | Where-Object { $_.Name -like "*Release*" } | Select-Object -First 1
$exeSource = Join-Path $buildDir.FullName $exeName
#$exeSource = Join-Path $projectRoot "build\Desktop_Qt_6_9_1_MinGW_64_bit-Release\$exeName"
$exeTarget = Join-Path $targetDir $exeName
Copy-Item $exeSource $exeTarget

# === TEMP windeployqt OUTPUT ===
$tempDir = Join-Path $projectRoot "dist"
if (Test-Path $tempDir) {
    Remove-Item -Recurse -Force $tempDir
}
New-Item -ItemType Directory -Path $tempDir | Out-Null
Copy-Item $exeSource $tempDir

$windeployqt = Join-Path $qtBinPath "windeployqt.exe"
if (-not (Test-Path $windeployqt)) {
    Write-Error "windeployqt not found at $windeployqt"
    exit 1
}
& $windeployqt --release --no-translations --no-opengl-sw --dir "$tempDir" "$tempDir\$exeName"

# === KEEP-ONLY STRATEGY ===
$filesToKeep = @(
    "Qt6Core.dll", "Qt6Gui.dll", "Qt6Widgets.dll",
    "Qt6SerialPort.dll", "Qt6Network.dll",
	"Qt6Qml.dll", "Qt6QmlMeta.dll", "Qt6QmlModels.dll", "Qt6QmlWorkerScript.dll",
    "libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll",
    $exeName
)
$foldersToKeep = @("platforms", "styles")

foreach ($file in $filesToKeep) {
    $src = Join-Path $tempDir $file
    if (Test-Path $src) {
        Copy-Item $src -Destination $targetDir
    } else {
        Write-Warning "Missing: $file"
    }
}

foreach ($folder in $foldersToKeep) {
    $src = Join-Path $tempDir $folder
    $dst = Join-Path $targetDir $folder
    if (Test-Path $src) {
        Copy-Item $src -Destination $dst -Recurse
    } else {
        Write-Warning "Missing folder: $folder"
    }
}

Remove-Item -Recurse -Force $tempDir

# === OPTIONAL ZIP ===
if ($Zip) {
    if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
    Compress-Archive -Path "$targetDir\\*" -DestinationPath $zipPath
    Write-Host "Zipped to: $zipPath"
}

Write-Host "Deployment complete: $targetDir"
