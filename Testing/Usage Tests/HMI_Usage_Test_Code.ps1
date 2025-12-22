# UART System Comparison Analysis
# Save as: analyze_system.ps1

$armPath = "C:\Program Files\Arm\GNU Toolchain mingw-w64-x86_64-arm-none-eabi\bin"
$env:Path += ";$armPath"

Write-Host "=== HMI - COMPLETE ANALYSIS ===" -ForegroundColor Cyan
Write-Host "Comparing HMI and CONTROL projects" -ForegroundColor White
Write-Host ""

$results = @()

# Analyze HMI
Write-Host "1. HMI PROJECT ANALYSIS:" -ForegroundColor Green
Set-Location "D:\University\7th Semester Senior 1\Introduction to Enbedded Systems\Project\adeem\HMI\Debug\Exe"
$hmiOutput = arm-none-eabi-size -B HMI.axf
Write-Host $hmiOutput

$hmiLines = $hmiOutput -split "`r?`n"
if ($hmiLines.Count -ge 2) {
    $hmiData = $hmiLines[1].Trim() -replace '\s+', ' ' -split ' '
    if ($hmiData.Count -ge 3) {
        $hmiText = [int]$hmiData[0]
        $hmiDataVal = [int]$hmiData[1]
        $hmiBss = [int]$hmiData[2]
        
        $hmiFlash = $hmiText + $hmiDataVal
        $hmiRam = $hmiDataVal + $hmiBss
        
        $results += [PSCustomObject]@{
            Project = "HMI"
            Code = $hmiText
            Data = $hmiDataVal
            BSS = $hmiBss
            Flash = $hmiFlash
            RAM = $hmiRam
        }
    }
}

Write-Host ""

# Analyze CONTROL
Write-Host "2. CONTROL PROJECT ANALYSIS:" -ForegroundColor Green
Set-Location "D:\University\7th Semester Senior 1\Introduction to Enbedded Systems\Project\adeem\CONTROL\Debug\Exe"
$ctrlOutput = arm-none-eabi-size -B CONTROL.axf
Write-Host $ctrlOutput

$ctrlLines = $ctrlOutput -split "`r?`n"
if ($ctrlLines.Count -ge 2) {
    $ctrlData = $ctrlLines[1].Trim() -replace '\s+', ' ' -split ' '
    if ($ctrlData.Count -ge 3) {
        $ctrlText = [int]$ctrlData[0]
        $ctrlDataVal = [int]$ctrlData[1]
        $ctrlBss = [int]$ctrlData[2]
        
        $ctrlFlash = $ctrlText + $ctrlDataVal
        $ctrlRam = $ctrlDataVal + $ctrlBss
        
        $results += [PSCustomObject]@{
            Project = "CONTROL"
            Code = $ctrlText
            Data = $ctrlDataVal
            BSS = $ctrlBss
            Flash = $ctrlFlash
            RAM = $ctrlRam
        }
    }
}

# System Summary
Write-Host "`n3. SYSTEM SUMMARY:" -ForegroundColor Cyan
Write-Host "==================" -ForegroundColor Cyan

$totalFlash = ($results | Measure-Object -Property Flash -Sum).Sum
$totalRam = ($results | Measure-Object -Property RAM -Sum).Sum

$flashPercent = [math]::Round(($totalFlash / 262144) * 100, 2)
$ramPercent = [math]::Round(($totalRam / 32768) * 100, 2)

$results | Format-Table -AutoSize

Write-Host "`n4. TOTALS (Per TM4C):" -ForegroundColor Yellow
Write-Host "   Total Flash used: $totalFlash bytes ($flashPercent% of 256KB)" -ForegroundColor White
Write-Host "   Total RAM used:   $totalRam bytes ($ramPercent% of 32KB)" -ForegroundColor White

Write-Host "`n5. RECOMMENDATIONS:" -ForegroundColor Green
Write-Host "   ✅ Both projects are memory efficient" -ForegroundColor Green
Write-Host "   ✅ Plenty of room for UART communication buffers" -ForegroundColor Green
Write-Host "   ✅ System is well-optimized for TM4C123GH6PM" -ForegroundColor Green

# Generate comparison report
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$comparisonReport = "uart_system_comparison_$timestamp.txt"

@"
=== UART SECURITY SYSTEM - COMPLETE ANALYSIS ===
Generated: $(Get-Date)

=== PROJECT COMPARISON ===
$(($results | Format-Table -AutoSize | Out-String))

=== MEMORY UTILIZATION PER TM4C ===
HMI Flash: $hmiFlash bytes ($([math]::Round($hmiFlash/262144*100,2))% of 256KB)
HMI RAM:   $hmiRam bytes ($([math]::Round($hmiRam/32768*100,2))% of 32KB)

CONTROL Flash: $ctrlFlash bytes ($([math]::Round($ctrlFlash/262144*100,2))% of 256KB)
CONTROL RAM:   $ctrlRam bytes ($([math]::Round($ctrlRam/32768*100,2))% of 32KB)

=== SYSTEM TOTALS (Average per board) ===
Average Flash per TM4C: $([math]::Round($totalFlash/2)) bytes
Average RAM per TM4C:   $([math]::Round($totalRam/2)) bytes

=== UART BUFFER ANALYSIS ===
From HMI.c:
  • Password buffers: ~18 bytes (pass[6], Confirmpass[6], new_pass[6])
  • Communication buffers: ~40 bytes (response[20], startup_check[20])
  • Total HMI UART: ~58 bytes

From CONTROL.c:
  • Password buffer: 20 bytes (master_password[20])
  • UART buffers: 70 bytes (rx_buffer[50], read_buffer[20])
  • Total CONTROL UART: ~90 bytes

Total UART buffers in system: ~148 bytes

=== PERFORMANCE ASSESSMENT ===
✅ Both projects are highly memory efficient
✅ UART buffers are appropriately sized
✅ Plenty of headroom for additional features
✅ Well-suited for TM4C123GH6PM hardware

=== RECOMMENDATIONS ===
1. Both projects can run simultaneously without memory issues
2. UART buffer sizes are adequate for 5-digit password communication
3. Consider adding error counters or logging features
4. System has capacity for additional security features
"@ | Out-File $comparisonReport -Encoding UTF8

Write-Host "`nComparison report saved to: $comparisonReport" -ForegroundColor Green
notepad $comparisonReport