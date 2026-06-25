param(
    [Parameter(Mandatory = $true)]
    [string]$DllPath,

    [Parameter(Mandatory = $true)]
    [string]$OutDef
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $DllPath)) {
    throw "DLL not found: $DllPath"
}

$dumpbin = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
if (-not $dumpbin) {
    throw "dumpbin.exe not found. Run this from a Visual Studio Developer Command Prompt."
}

$outDir = Split-Path -Parent $OutDef
if ($outDir -eq "") {
    $outDir = "."
}

if (-not (Test-Path -LiteralPath $outDir)) {
    New-Item -ItemType Directory -Path $outDir | Out-Null
}

$dllFileName = [System.IO.Path]::GetFileName($DllPath)

$dump = & dumpbin.exe /nologo /exports $DllPath
if ($LASTEXITCODE -ne 0) {
    throw "dumpbin.exe failed for: $DllPath"
}

$exports = @()

foreach ($line in $dump) {
    if ($line -match '^\s*\d+\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]+\s+(\S+)') {
        $name = $Matches[1]

        # Skip malformed or pseudo entries defensively.
        if ($name -and $name -notmatch '^\[') {
            $exports += $name
        }
    }
}

if ($exports.Count -eq 0) {
    throw "No exports found in DLL: $DllPath"
}

$defLines = @()
$defLines += "LIBRARY $dllFileName"
$defLines += "EXPORTS"
$defLines += $exports | Sort-Object -Unique | ForEach-Object { "    $_" }

Set-Content -LiteralPath $OutDef -Encoding ASCII -Value $defLines