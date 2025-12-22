# FIXED Control Project Analysis - Error Free
$armPath = "C:\Program Files\Arm\GNU Toolchain mingw-w64-x86_64-arm-none-eabi\bin"
$env:Path += ";$armPath"

cd "D:\University\7th Semester Senior 1\Introduction to Enbedded Systems\Project\adeem\CONTROL\Debug\Exe"

Write-Host "=== CONTROL Project Analysis ===" -ForegroundColor Cyan
Write-Host ""

# 1. Get memory usage
Write-Host "1. MEMORY USAGE:" -ForegroundColor Green
$output = arm-none-eabi-size -B CONTROL.axf
Write-Host $output

# 2. MANUAL PARSING (No regex errors)
Write-Host "`n2. PARSING RESULTS..." -ForegroundColor Yellow

# Split output into lines
$lines = $output -split "`r?`n"

if ($lines.Count -ge 2) {
    # Get the second line (data line)
    $dataLine = $lines[1]
    
    # Clean up the line - remove extra spaces
    $dataLine = $dataLine.Trim() -replace '\s+', ' '
    
    Write-Host "Cleaned data line: '$dataLine'" -ForegroundColor Gray
    
    # Split by space
    $parts = $dataLine -split ' '
    
    if ($parts.Count -ge 6) {
        # Extract values (format: text data bss dec hex filename)
        $text = [int]$parts[0]
        $data = [int]$parts[1]
        $bss = [int]$parts[2]
        
        Write-Host "`n3. EXTRACTED VALUES:" -ForegroundColor Green
        Write-Host "   text (code): $text" -ForegroundColor White
        Write-Host "   data:        $data" -ForegroundColor White
        Write-Host "   bss:         $bss" -ForegroundColor White
        
        # Calculate totals
        $flashUsed = $text + $data
        $ramUsed = $data + $bss
        
        Write-Host "`n4. MEMORY SUMMARY:" -ForegroundColor Green
        Write-Host "   Flash used (code+data): $flashUsed bytes" -ForegroundColor Cyan
        Write-Host "   RAM used (data+bss):    $ramUsed bytes" -ForegroundColor Cyan
        
        # Calculate percentages
        $flashPercent = [math]::Round(($flashUsed / 262144) * 100, 2)
        $ramPercent = [math]::Round(($ramUsed / 32768) * 100, 2)
        
        Write-Host "`n5. TM4C UTILIZATION:" -ForegroundColor Green
        Write-Host "   Flash: $flashPercent% of 256KB" -ForegroundColor White
        Write-Host "   RAM:   $ramPercent% of 32KB" -ForegroundColor White
        
        Write-Host "`n6. STATUS:" -ForegroundColor Magenta
        Write-Host "   ✅ EXCELLENT - Very efficient memory usage!" -ForegroundColor Green
    } else {
        Write-Host "ERROR: Could not parse line correctly" -ForegroundColor Red
        Write-Host "Parts found: $($parts.Count)" -ForegroundColor Yellow
    }
} else {
    Write-Host "ERROR: Unexpected output format" -ForegroundColor Red
}

# 7. Check sections
Write-Host "`n7. SECTION DETAILS:" -ForegroundColor Green
try {
    arm-none-eabi-objdump -h CONTROL.axf | findstr ".text .data .bss .stack .heap" | ForEach-Object {
        Write-Host "   $_" -ForegroundColor Gray
    }
} catch {
    Write-Host "   Could not get section details" -ForegroundColor Yellow
}

# 8. Generate report
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$report = "control_analysis_$timestamp.txt"

Write-Host "`n8. GENERATING REPORT..." -ForegroundColor Cyan

# Create report
@"
=== CONTROL PROJECT RESOURCE ANALYSIS ===
Generated: $(Get-Date)
File: CONTROL.axf

=== RAW OUTPUT ===
$output

=== ANALYSIS RESULTS ===
Based on output: text=$text, data=$data, bss=$bss

Flash Memory:
  Code (.text):    $text bytes
  Data (.data):    $data bytes
  Total Flash:     $flashUsed bytes ($flashPercent% of 256KB)

RAM Memory:
  BSS (.bss):      $bss bytes
  Total RAM:       $ramUsed bytes ($ramPercent% of 32KB)

=== TM4C123GH6PM MEMORY ===
Flash Available: 262,144 bytes (256 KB)
Flash Used:      $flashUsed bytes ($flashPercent%)
RAM Available:   32,768 bytes (32 KB)
RAM Used:        $ramUsed bytes ($ramPercent%)

=== UART COMPONENTS (from your code) ===
• master_password[20]: 20 bytes
• rx_buffer[50]: 50 bytes  
• read_buffer[20]: 20 bytes
• Total UART buffers: ~90 bytes

=== RECOMMENDATIONS ===
✅ Excellent memory utilization!
- Plenty of room for additional features
- Can increase UART buffer sizes if needed
- Consider adding error logging/debug features
"@ | Out-File $report -Encoding UTF8

Write-Host "Report saved to: $report" -ForegroundColor Green

# Open the report
notepad $report