[CmdletBinding(SupportsShouldProcess = $true, ConfirmImpact = 'High')]
param(
	[Parameter(Mandatory = $true)]
	[string]$Tag,
	[string]$ZipPath = 'out\Quackle-Windows-x64.zip',
	[string]$Title = '',
	[string]$Notes = '',
	[switch]$Prerelease
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')).Path
$repository = 'lubomirb/quackle'

if (-not [System.IO.Path]::IsPathRooted($ZipPath)) {
	$ZipPath = Join-Path $repoRoot $ZipPath
}
$zip = Get-Item -LiteralPath $ZipPath -ErrorAction Stop
if ($zip.PSIsContainer -or $zip.Extension -ne '.zip' -or $zip.Length -eq 0) {
	throw "The release asset must be a non-empty ZIP file: $ZipPath"
}
if ($zip.Name -match '^source code\.(zip|tar\.gz)$') {
	throw 'A source archive cannot be used as the release asset.'
}

& git -C $repoRoot check-ref-format "refs/tags/$Tag"
if ($LASTEXITCODE -ne 0 -or $Tag.StartsWith('-')) {
	throw "Invalid Git tag: $Tag"
}
$commit = & git -C $repoRoot rev-parse --verify 'HEAD^{commit}'
if ($LASTEXITCODE -ne 0) {
	throw 'Cannot determine the current commit.'
}
$changes = @( & git -C $repoRoot status --porcelain --untracked-files=no )
if ($LASTEXITCODE -ne 0) {
	throw 'Cannot inspect the Git working tree.'
}
if ($changes.Count -gt 0) {
	throw 'Commit or discard tracked changes before publishing so the tag identifies the source used for the build.'
}

$remote = & git -C $repoRoot remote get-url --push origin
if ($LASTEXITCODE -ne 0 -or $remote -notmatch '^(https://github\.com/|git@github\.com:|ssh://git@github\.com/)lubomirb/quackle(\.git)?$') {
	throw "The origin push URL must point to github.com/$repository. Actual URL: $remote"
}
& git -C $repoRoot show-ref --verify --quiet "refs/tags/$Tag"
if ($LASTEXITCODE -eq 0) {
	throw "Local tag already exists: $Tag"
}
if ($LASTEXITCODE -ne 1) {
	throw "Could not check local tag: $Tag"
}

if (-not $Title) { $Title = "Quackle $Tag" }
if (-not $Notes) { $Notes = "Windows x64 release $Tag." }

if (-not $PSCmdlet.ShouldProcess("github.com/$repository", "Create tag $Tag at $commit and publish $($zip.Name)")) {
	return
}

$remoteTag = @( & git -C $repoRoot ls-remote --exit-code --tags origin "refs/tags/$Tag" )
if ($LASTEXITCODE -eq 0 -and $remoteTag.Count -gt 0) {
	throw "Remote tag already exists: $Tag"
}
if ($LASTEXITCODE -ne 2) {
	throw "Could not check whether remote tag exists: $Tag"
}

$credentialLines = @('protocol=https', 'host=github.com', '') | & git -C $repoRoot credential fill
if ($LASTEXITCODE -ne 0) {
	throw 'Git Credential Manager could not supply GitHub credentials.'
}
$credentialPassword = @($credentialLines | Where-Object { $_ -like 'password=*' })
if ($credentialPassword.Count -ne 1 -or $credentialPassword[0].Length -le 9) {
	throw 'Git Credential Manager did not return a GitHub access token.'
}
$headers = @{
	Authorization = 'Bearer ' + $credentialPassword[0].Substring(9)
	Accept = 'application/vnd.github+json'
	'User-Agent' = 'Quackle-release-script'
	'X-GitHub-Api-Version' = '2022-11-28'
}

$repoInfo = Invoke-RestMethod -Method Get -Uri "https://api.github.com/repos/$repository" -Headers $headers
if (-not $repoInfo.permissions.push) {
	throw "The GitHub credentials do not have write access to $repository."
}

& git -C $repoRoot tag -a $Tag $commit -m "Release $Tag"
if ($LASTEXITCODE -ne 0) {
	throw "Could not create local tag: $Tag"
}
& git -C $repoRoot push origin "refs/tags/${Tag}:refs/tags/${Tag}"
if ($LASTEXITCODE -ne 0) {
	throw "Could not push tag $Tag. The local tag remains; inspect it before retrying."
}

$body = @{
	tag_name = $Tag
	name = $Title
	body = $Notes
	draft = $true
	prerelease = [bool]$Prerelease
} | ConvertTo-Json
try {
	$release = Invoke-RestMethod -Method Post -Uri "https://api.github.com/repos/$repository/releases" -Headers $headers -ContentType 'application/json' -Body $body
	$assetName = [uri]::EscapeDataString($zip.Name)
	$uploadUrl = "https://uploads.github.com/repos/$repository/releases/$($release.id)/assets?name=$assetName"
	Invoke-RestMethod -Method Post -Uri $uploadUrl -Headers $headers -ContentType 'application/zip' -InFile $zip.FullName | Out-Null
	Invoke-RestMethod -Method Patch -Uri "https://api.github.com/repos/$repository/releases/$($release.id)" -Headers $headers -ContentType 'application/json' -Body '{"draft":false}' | Out-Null
}
catch {
	throw "Release publishing failed after tag $Tag was pushed. Check GitHub for a draft release and the uploaded asset before retrying. Error: $_"
}

Write-Host "Published: $($release.html_url)"
