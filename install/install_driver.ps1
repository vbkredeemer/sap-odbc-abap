<#
.SYNOPSIS
    SAP ODBC Driver — Automatische Installation

.DESCRIPTION
    Registriert den SAP ODBC-Treiber und legt eine System-DSN an.
    Kann mehrfach ausgeführt werden — für jedes SAP-System eine eigene DSN.
    Muss als Administrator ausgeführt werden.

.EXAMPLE
    .\install_driver.ps1
    Interaktive Installation — DLL auswaehlen, Parameter eingeben.

.EXAMPLE
    .\install_driver.ps1 -DSNName "SAP_DAA" -SapHost "jbklsapas1daa.jbdmn.de"
    .\install_driver.ps1 -DSNName "SAP_KAA" -SapHost "jbklsapas1kaa.jbdmn.de"
    .\install_driver.ps1 -DSNName "SAP_PAA" -SapHost "jbklsapas1paa.jbdmn.de"
    Drei DSNs fuer DAA, KAA und PAA mit demselben Treiber.
    Client (100), SysNr (10) und Lang (DE) sind vorbelegt.
    User und Passwort werden interaktiv abgefragt.
#>

param(
    [string]$DriverPath = "",
    [string]$DriverName = "SAP via Z_EXECUTE_SQL",
    [string]$DSNName    = "SAP_ODBC_ABAP",
    [string]$SapHost    = "jbklsapas1daa.jbdmn.de",
    [string]$SysNr      = "10",
    [string]$Client     = "100",
    [string]$SapUser    = "",
    [string]$Password   = "",
    [string]$Lang       = "DE",
    [int]$MaxRows       = 50000
)

# --- Admin-Check ---
if (-not ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host "FEHLER: Dieses Skript muss als Administrator ausgefuehrt werden." -ForegroundColor Red
    Write-Host "Rechtsklick auf PowerShell -> Als Administrator ausfuehren" -ForegroundColor Yellow
    exit 1
}

# --- DLL suchen wenn nicht angegeben ---
if ($DriverPath -eq "" -or -not (Test-Path $DriverPath)) {
    Write-Host ""
    Write-Host "Bitte sapodbcabap.dll auswaehlen:" -ForegroundColor Cyan
    Add-Type -AssemblyName System.Windows.Forms
    $openDlg = New-Object System.Windows.Forms.OpenFileDialog
    $openDlg.Filter = "ODBC-Treiber (*.dll)|*.dll|Alle Dateien (*.*)|*.*"
    $openDlg.Title = "sapodbcabap.dll auswaehlen"
    $openDlg.InitialDirectory = "C:\Scripts\SAP_ODBC"
    if (-not (Test-Path $openDlg.InitialDirectory)) {
        $openDlg.InitialDirectory = "C:\"
    }
    $result = $openDlg.ShowDialog()
    if ($result -ne [System.Windows.Forms.DialogResult]::OK -or $openDlg.FileName -eq "") {
        Write-Host "FEHLER: Keine DLL ausgewaehlt." -ForegroundColor Red
        exit 1
    }
    $DriverPath = $openDlg.FileName
} else {
    # Pfad aus Parameter normalisieren
    $DriverPath = $DriverPath -replace '/', '\'
}

# Registry erwartet Backslashes als Pfadtrenner
$DriverPath = $DriverPath -replace '/', '\'

Write-Host "  Treiber-DLL: $DriverPath" -ForegroundColor Green

# --- Abhaengige DLLs pruefen ---
$DllDir = Split-Path $DriverPath
$RequiredDlls = @("sapnwrfc.dll", "icudt57.dll", "icuin57.dll", "icuuc57.dll")
$MissingDlls = @()
foreach ($dll in $RequiredDlls) {
    $dllPath = Join-Path $DllDir $dll
    if (-not (Test-Path $dllPath)) {
        $MissingDlls += $dll
    }
}
if ($MissingDlls.Count -gt 0) {
    Write-Host ""
    Write-Host "FEHLER: Folgende DLLs fehlen im Verzeichnis $DllDir :" -ForegroundColor Red
    foreach ($dll in $MissingDlls) {
        Write-Host "  - $dll" -ForegroundColor Red
    }
    Write-Host ""
    Write-Host "Diese DLLs muessen ins gleiche Verzeichnis wie sapodbcabap.dll kopiert werden:" -ForegroundColor Yellow
    Write-Host "  sapnwrfc.dll         — aus dem SAP NWRFC SDK 7.50 (nwrfcsdk\bin\)" -ForegroundColor Yellow
    Write-Host "  icudt57.dll          — aus dem SAP NWRFC SDK 7.50 (nwrfcsdk\bin\)" -ForegroundColor Yellow
    Write-Host "  icuin57.dll          — aus dem SAP NWRFC SDK 7.50 (nwrfcsdk\bin\)" -ForegroundColor Yellow
    Write-Host "  icuuc57.dll          — aus dem SAP NWRFC SDK 7.50 (nwrfcsdk\bin\)" -ForegroundColor Yellow
    exit 1
}
Write-Host "  Alle Abhaengigkeiten gefunden." -ForegroundColor Green

# --- Alle DLLs nach C:\Windows\System32 kopieren ---
# Windows sucht abhaengige DLLs in System32 — dorthin kopieren,
# damit der ODBC-Treiber sie findet (Systemfehlercode 126 vermeiden).
$SystemDir = "$env:WINDIR\System32"
$AllDlls = @("sapodbcabap.dll") + $RequiredDlls
Write-Host ""
Write-Host "DLLs werden nach $SystemDir kopiert..." -ForegroundColor Cyan
foreach ($dll in $AllDlls) {
    $srcPath = Join-Path $DllDir $dll
    $dstPath = Join-Path $SystemDir $dll
    Copy-Item -Path $srcPath -Destination $dstPath -Force
    Write-Host "  $dll kopiert." -ForegroundColor Green
}
# Treiber-Pfad auf System32 umstellen
$DriverPath = Join-Path $SystemDir "sapodbcabap.dll"

# --- Interaktive Abfrage wenn SapUser leer ---
if ($SapUser -eq "") {
    Write-Host ""
    Write-Host "=== SAP ODBC Driver Installation ===" -ForegroundColor Cyan
    Write-Host ""
    $input = Read-Host "DSN-Name [$DSNName]"
    if ($input -ne "") { $DSNName = $input }
    $input = Read-Host "SAP Applikationsserver (Host) [$SapHost]"
    if ($input -ne "") { $SapHost = $input }
    $input = Read-Host "Systemnummer (SysNr) [$SysNr]"
    if ($input -ne "") { $SysNr = $input }
    $input = Read-Host "Mandant (Client) [$Client]"
    if ($input -ne "") { $Client = $input }
    $SapUser = Read-Host "SAP-Benutzer"
    $SecurePassword = Read-Host "Passwort" -AsSecureString
    $Password = [Runtime.InteropServices.Marshal]::PtrToStringAuto(
        [Runtime.InteropServices.Marshal]::SecureStringToBSTR($SecurePassword)
    )
    $input = Read-Host "Sprache (Lang) [$Lang]"
    if ($input -ne "") { $Lang = $input }
}

$Registry = "HKLM:\SOFTWARE\ODBC"

Write-Host ""
Write-Host "Treiber wird registriert..." -ForegroundColor Cyan

# --- ODBC-Treiber registrieren (nur einmal noetig, mehrfach idempotent) ---
$DriverKey = "$Registry\ODBCINST.INI\$DriverName"
if (-not (Test-Path $DriverKey)) {
    New-Item -Path $DriverKey -Force | Out-Null
}
Set-ItemProperty -Path $DriverKey -Name "Driver" -Value $DriverPath
Set-ItemProperty -Path $DriverKey -Name "Setup" -Value $DriverPath
Set-ItemProperty -Path $DriverKey -Name "APIS" -Value "2"
Set-ItemProperty -Path $DriverKey -Name "ConnectFunctions" -Value "YYY"
Set-ItemProperty -Path $DriverKey -Name "DriverODBCVer" -Value "03.80"
Set-ItemProperty -Path $DriverKey -Name "SQLLevel" -Value "0"
Set-ItemProperty -Path $DriverKey -Name "FileUsage" -Value "0"
Set-ItemProperty -Path $DriverKey -Name "Platform" -Value "x64"

$DriversList = "$Registry\ODBCINST.INI\ODBC Drivers"
if (-not (Test-Path $DriversList)) {
    New-Item -Path $DriversList -Force | Out-Null
}
Set-ItemProperty -Path $DriversList -Name $DriverName -Value "Installed"

Write-Host "  Treiber '$DriverName' registriert." -ForegroundColor Green

# --- System-DSN anlegen ---
Write-Host "Datenquelle (System-DSN) wird angelegt..." -ForegroundColor Cyan

$DSNKey = "$Registry\ODBC.INI\$DSNName"
if (-not (Test-Path $DSNKey)) {
    New-Item -Path $DSNKey -Force | Out-Null
}
Set-ItemProperty -Path $DSNKey -Name "Driver"   -Value $DriverPath
Set-ItemProperty -Path $DSNKey -Name "Host"     -Value $SapHost
Set-ItemProperty -Path $DSNKey -Name "SysNr"    -Value $SysNr
Set-ItemProperty -Path $DSNKey -Name "Client"   -Value $Client
Set-ItemProperty -Path $DSNKey -Name "User"     -Value $SapUser
Set-ItemProperty -Path $DSNKey -Name "Password" -Value $Password
Set-ItemProperty -Path $DSNKey -Name "Lang"     -Value $Lang
Set-ItemProperty -Path $DSNKey -Name "MaxRows"  -Value $MaxRows

$DSNList = "$Registry\ODBC.INI\ODBC Data Sources"
if (-not (Test-Path $DSNList)) {
    New-Item -Path $DSNList -Force | Out-Null
}
Set-ItemProperty -Path $DSNList -Name $DSNName -Value $DriverName

Write-Host "  System-DSN '$DSNName' angelegt." -ForegroundColor Green

# --- Fertig ---
Write-Host ""
Write-Host "Installation abgeschlossen!" -ForegroundColor Green
Write-Host ""
Write-Host "Treiber:  $DriverName" -ForegroundColor White
Write-Host "DSN:      $DSNName" -ForegroundColor White
Write-Host "Host:     $SapHost" -ForegroundColor White
Write-Host "Client:   $Client" -ForegroundColor White
Write-Host ""
Write-Host "Test in Excel/Power BI mit:" -ForegroundColor Cyan
Write-Host "  SELECT * FROM MARA LIMIT 100" -ForegroundColor White
Write-Host ""
Write-Host "Verbindungsstring:" -ForegroundColor Cyan
Write-Host "  DSN=$DSNName;Host=$SapHost;SysNr=$SysNr;Client=$Client;User=$SapUser;Password=********;Lang=$Lang" -ForegroundColor White
Write-Host ""
Write-Host "Weitere DSNs anlegen:" -ForegroundColor Cyan
Write-Host "  .\install_driver.ps1 -DSNName 'SAP_KAA' -SapHost 'jbklsapas1kaa.jbdmn.de'" -ForegroundColor White
Write-Host "  .\install_driver.ps1 -DSNName 'SAP_PAA' -SapHost 'jbklsapas1paa.jbdmn.de'" -ForegroundColor White