# Push script for bilibili-3ds (PowerShell)
# Usage: .\push.ps1 [-Message "custom commit message"]
param(
    [string]$Message = "fix: socInit + libcurl ??????? socInit?? curl_global_init ??"
)

Write-Host "Committing: $Message" -ForegroundColor Cyan
git add -A
git commit -m $Message

Write-Host "`nPushing to fix/full-app ..." -ForegroundColor Yellow
git push origin fix/full-app --force 2>&1

if ($LASTEXITCODE -eq 0) {
    Write-Host "`nOK - pushed. Check https://github.com/lvyuchuiyi/bilibili-3ds/actions" -ForegroundColor Green
} else {
    Write-Host "`nFAILED - check git output above" -ForegroundColor Red
}
