#!/usr/bin/env pwsh
# Run all timing tests (TDD Cycles 1, 2, 3)
#
# Executes all three test executables sequentially
# Returns 0 if all pass, 1 if any fail

$ErrorActionPreference = "Stop"
$TestDir = "test\test_timing\build\Debug"

Write-Host "================================" -ForegroundColor Cyan
Write-Host "Running All Timing Tests" -ForegroundColor Cyan
Write-Host "================================" -ForegroundColor Cyan
Write-Host ""

$TotalTests = 0
$PassedTests = 0
$FailedTests = 0

# Test 1: Timestamp Query (TDD Cycle 1)
Write-Host ">>> Cycle 1: Timestamp Query (DES-I-001)" -ForegroundColor Yellow
& ".\$TestDir\test_timestamp_query.exe"
if ($LASTEXITCODE -eq 0) {
    $PassedTests += 5
} else {
    $FailedTests += 5
}
$TotalTests += 5
Write-Host ""

# Test 2: RTC Health (TDD Cycle 2)
Write-Host ">>> Cycle 2: RTC Health Status (DES-I-002)" -ForegroundColor Yellow
& ".\$TestDir\test_rtc_health.exe"
if ($LASTEXITCODE -eq 0) {
    $PassedTests += 6
} else {
    $FailedTests += 6
}
$TotalTests += 6
Write-Host ""

# Test 3: Time Sync (TDD Cycle 3)
Write-Host ">>> Cycle 3: Time Synchronization (DES-I-003)" -ForegroundColor Yellow
& ".\$TestDir\test_time_sync.exe"
if ($LASTEXITCODE -eq 0) {
    $PassedTests += 6
} else {
    $FailedTests += 6
}
$TotalTests += 6
Write-Host ""

# Summary
Write-Host "================================" -ForegroundColor Cyan
Write-Host "Test Summary" -ForegroundColor Cyan
Write-Host "================================" -ForegroundColor Cyan
Write-Host "Total Tests: $TotalTests" -ForegroundColor White
Write-Host "Passed: $PassedTests" -ForegroundColor Green
Write-Host "Failed: $FailedTests" -ForegroundColor $(if ($FailedTests -eq 0) { "Green" } else { "Red" })

if ($FailedTests -eq 0) {
    Write-Host ""
    Write-Host "✅ ALL TESTS PASSED!" -ForegroundColor Green
    exit 0
} else {
    Write-Host ""
    Write-Host "❌ SOME TESTS FAILED" -ForegroundColor Red
    exit 1
}
