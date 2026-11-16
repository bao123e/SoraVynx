$ErrorActionPreference='Stop'
try {
 Add-Type -AssemblyName System.Runtime.WindowsRuntime
 [Windows.Media.Ocr.OcrEngine,Windows.Foundation,ContentType=WindowsRuntime] | Out-Null
 [Windows.Globalization.Language,Windows.Globalization,ContentType=WindowsRuntime] | Out-Null
 $languages=[Windows.Media.Ocr.OcrEngine]::AvailableRecognizerLanguages
 Write-Output ('OCR languages: '+(($languages | ForEach-Object {$_.LanguageTag}) -join ','))
 $language=[Windows.Globalization.Language]::new('en-US')
 $engine=[Windows.Media.Ocr.OcrEngine]::TryCreateFromLanguage($language)
 if($null -eq $engine){throw 'English OCR unavailable'}
 Write-Output ('OCR engine ready; max size='+[Windows.Media.Ocr.OcrEngine]::MaxImageDimension)
} catch { Write-Output ('OCR probe failed: '+$_.Exception.Message);exit 1 }
