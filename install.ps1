# install.ps1 — Download and install a prebuilt t81 binary on Windows.
#
# Usage (run in an elevated or standard PowerShell session):
#   irm https://github.com/t81dev/t81-foundation/releases/latest/download/install.ps1 | iex
#
# Options (environment variables):
#   $env:T81_INSTALL_DIR   Target directory (default: $env:LOCALAPPDATA\t81\bin)
#   $env:T81_VERSION       Pin a specific release tag, e.g. v1.9.0  (default: latest)

$ErrorActionPreference = 'Stop'

$Repo       = 't81dev/t81-foundation'
$Platform   = 'windows-x86_64'
$InstallDir = if ($env:T81_INSTALL_DIR) { $env:T81_INSTALL_DIR } `
              else { Join-Path $env:LOCALAPPDATA 't81\bin' }

# ── Resolve release tag ───────────────────────────────────────────────────────
if ($env:T81_VERSION) {
    $Tag = $env:T81_VERSION
} else {
    Write-Host 'Fetching latest release tag ...' -NoNewline
    $Release = Invoke-RestMethod "https://api.github.com/repos/$Repo/releases/latest"
    $Tag = $Release.tag_name
    Write-Host " $Tag"
}

$Archive = "t81-$Tag-$Platform.zip"
$Url     = "https://github.com/$Repo/releases/download/$Tag/$Archive"

# ── Download ──────────────────────────────────────────────────────────────────
$Tmp = Join-Path ([System.IO.Path]::GetTempPath()) ([System.Guid]::NewGuid().ToString())
New-Item -ItemType Directory -Path $Tmp | Out-Null

try {
    Write-Host "Downloading $Archive ..."
    Invoke-WebRequest -Uri $Url -OutFile (Join-Path $Tmp $Archive) -UseBasicParsing

    # ── Install ───────────────────────────────────────────────────────────────
    New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null

    $Extracted = Join-Path $Tmp 'extracted'
    Expand-Archive -Path (Join-Path $Tmp $Archive) -DestinationPath $Extracted

    # Archive mirrors cmake --install layout: bin\t81.exe, lib\*, include\*
    Copy-Item (Join-Path $Extracted 'bin\t81.exe') -Destination (Join-Path $InstallDir 't81.exe') -Force

    Write-Host ""
    Write-Host "  t81 $Tag installed -> $InstallDir\t81.exe"
    Write-Host ""

    # ── PATH hint ─────────────────────────────────────────────────────────────
    $UserPath = [System.Environment]::GetEnvironmentVariable('PATH', 'User')
    if ($UserPath -notlike "*$InstallDir*") {
        Write-Host "  Add $InstallDir to your PATH so you can run t81 from any terminal:"
        Write-Host ""
        Write-Host "    [Environment]::SetEnvironmentVariable('PATH', `$env:PATH + ';$InstallDir', 'User')"
        Write-Host ""
        Write-Host "  Or open System Properties > Environment Variables and add it manually."
        Write-Host ""
    }

    Write-Host "  Run 't81 --version' to verify the installation."
} finally {
    Remove-Item -Recurse -Force $Tmp -ErrorAction SilentlyContinue
}
