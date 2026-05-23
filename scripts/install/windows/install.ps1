param (
    # Path to build output directory (e.g. CMake build folder)
    [Parameter(Mandatory = $true)]
    [string]$BuildPath,

    # Path where packaging output should be placed
    [Parameter(Mandatory = $true)]
    [string]$InstallPath
)

# Fail fast on any error (important for CI pipelines)
$ErrorActionPreference = "Stop"

# -----------------------------
# Read version from file
# -----------------------------

# Version file is expected at:
#   <build>/gen/version
#
# It must contain a single line like:
#   0.11.2
$VersionFile = Join-Path $BuildPath "version"

if (-not (Test-Path $VersionFile)) {
    throw "Version file not found: $VersionFile"
}

# Read version string and remove whitespace/newlines
$Version = (Get-Content $VersionFile -Raw).Trim()

if ([string]::IsNullOrWhiteSpace($Version)) {
    throw "Version file is empty: $VersionFile"
}

# -----------------------------
# Derived paths
# -----------------------------

# Temporary staging directory for packaging
$DeployDir = Join-Path $InstallPath "hsmide"

# Versioned output ZIP file
$ZipFile = Join-Path $InstallPath "hsmide-$Version-windows-x64-portable.zip"

# Path to built executable (adjust if your build config differs)
$ExePath = Join-Path $BuildPath "src\Release\hsm_ide.exe"

# -----------------------------
# Cleanup old artifacts
# -----------------------------

if (Test-Path $DeployDir) {
    Remove-Item $DeployDir -Recurse -Force
}

if (Test-Path $ZipFile) {
    Remove-Item $ZipFile -Force
}

# -----------------------------
# Create staging directory
# -----------------------------

New-Item -ItemType Directory -Path $DeployDir | Out-Null

# -----------------------------
# Copy executable
# -----------------------------

Copy-Item $ExePath -Destination $DeployDir

# -----------------------------
# Deploy Qt runtime dependencies
# -----------------------------

# windeployqt copies required Qt DLLs/plugins next to executable
& windeployqt `
    --no-translations `
    --no-quick-import `
    --no-opengl-sw `
    (Join-Path $DeployDir "hsm_ide.exe")

# TODO: check if D3Dcompiler_47.dll is needed
# TODO: remove gif plugin

# -----------------------------
# Create ZIP archive
# -----------------------------

Compress-Archive `
    -Path (Join-Path $DeployDir "*") `
    -DestinationPath $ZipFile `
    -Force

# -----------------------------
# Cleanup staging directory
# -----------------------------

Remove-Item $DeployDir -Recurse -Force

# -----------------------------
# Output result
# -----------------------------

Write-Host "Packaging complete: $ZipFile"