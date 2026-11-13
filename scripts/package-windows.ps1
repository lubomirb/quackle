[CmdletBinding()]
param(
	[string]$BuildDirectory = "out\build\quacker-release",
	[string]$OutputPath = "out\Quackle-Windows-x64.zip"
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

if (-not [System.IO.Path]::IsPathRooted($BuildDirectory)) {
	$BuildDirectory = Join-Path $repoRoot $BuildDirectory
}
if (-not [System.IO.Path]::IsPathRooted($OutputPath)) {
	$OutputPath = Join-Path $repoRoot $OutputPath
}

$BuildDirectory = (Resolve-Path -LiteralPath $BuildDirectory).Path
$executable = Join-Path $BuildDirectory "Quackle.exe"
if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
	throw "Quackle.exe was not found in build directory: $BuildDirectory"
}

$dlls = @(Get-ChildItem -LiteralPath $BuildDirectory -Filter "*.dll" -File)
if ($dlls.Count -eq 0) {
	throw "No runtime DLLs found beside Quackle.exe. Build and deploy the Qt application first."
}

$requiredDirectories = @("platforms", "data\alphabets", "data\lexica", "data\strategy", "data\themes")
foreach ($relativePath in $requiredDirectories) {
	$sourcePath = Join-Path $repoRoot $relativePath
	if ($relativePath -eq "platforms") {
		$sourcePath = Join-Path $BuildDirectory $relativePath
	}
	if (-not (Test-Path -LiteralPath $sourcePath -PathType Container)) {
		throw "Required runtime directory was not found: $sourcePath"
	}
}

$readmePath = Join-Path $repoRoot "README.Quackle"
if (-not (Test-Path -LiteralPath $readmePath -PathType Leaf)) {
	throw "Required Quackle readme was not found: $readmePath"
}

$outputDirectory = Split-Path -Parent $OutputPath
if (-not (Test-Path -LiteralPath $outputDirectory -PathType Container)) {
	New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
}

$stagingDirectory = Join-Path ([System.IO.Path]::GetTempPath()) ("Quackle-package-" + [guid]::NewGuid().ToString("N"))
$packageRoot = Join-Path $stagingDirectory "Quackle"
New-Item -ItemType Directory -Path $packageRoot -Force | Out-Null

try {
	Copy-Item -LiteralPath $executable -Destination $packageRoot
	foreach ($dll in $dlls) {
		Copy-Item -LiteralPath $dll.FullName -Destination $packageRoot
	}
	Copy-Item -LiteralPath (Join-Path $repoRoot "LICENSE") -Destination $packageRoot
	Copy-Item -LiteralPath $readmePath -Destination (Join-Path $packageRoot "README.Quackle.txt")

	foreach ($directoryName in @("platforms", "imageformats", "styles")) {
		$sourceDirectory = Join-Path $BuildDirectory $directoryName
		if (Test-Path -LiteralPath $sourceDirectory -PathType Container) {
			Copy-Item -LiteralPath $sourceDirectory -Destination $packageRoot -Recurse
		}
	}

	foreach ($directoryName in @("alphabets", "lexica", "strategy", "themes")) {
		$sourceDirectory = Join-Path $repoRoot (Join-Path "data" $directoryName)
		$destinationDirectory = Join-Path $packageRoot (Join-Path "data" $directoryName)
		Get-ChildItem -LiteralPath $sourceDirectory -File -Recurse |
			Where-Object { $_.Name -ne "CMakeLists.txt" } |
			ForEach-Object {
				$relativePath = $_.FullName.Substring($sourceDirectory.Length).TrimStart([char[]]@('\', '/'))
				$destinationPath = Join-Path $destinationDirectory $relativePath
				$parentDirectory = Split-Path -Parent $destinationPath
				if (-not (Test-Path -LiteralPath $parentDirectory -PathType Container)) {
					New-Item -ItemType Directory -Path $parentDirectory -Force | Out-Null
				}
				Copy-Item -LiteralPath $_.FullName -Destination $destinationPath
			}
	}

	Compress-Archive -Path (Join-Path $packageRoot "*") -DestinationPath $OutputPath -Force
	Write-Host "Created Quackle ZIP: $OutputPath"
}
finally {
	if (Test-Path -LiteralPath $stagingDirectory -PathType Container) {
		Remove-Item -LiteralPath $stagingDirectory -Recurse -Force
	}
}
