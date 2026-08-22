## Signing the executable
To enable UIAccess and avoid the error `A referral was returned from the server`, the magnify executable must be signed and located in a trusted folder such as `Program Files`
The tool used for signing, `SignTool`, is usually located in `C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64`

## Method 1: Generate certificate file
1. Generate certificate file with password
```
$cert = New-SelfSignedCertificate -Type CodeSigning -Subject "<CERT_SUBJECT>" -CertStoreLocation Cert:\CurrentUser\My
$CertPassword = ConvertTo-SecureString -String "<CERT_PASSWORD>" -Force –AsPlainText
Export-PfxCertificate -Cert "cert:\CurrentUser\My\$($cert.Thumbprint)" -FilePath "<CERT_FILEPATH>" -Password $CertPassword
```
2. Install pfx to the local machine trusted root store
3. Sign the executable with the pfx
```
SignTool sign /fd SHA256 /v /f <CERT_FILEPATH> /p <CERT_PASSWORD> /t http://timestamp.digicert.com <EXE_FILENAME>
```

## Method 2: Generate certificate in certificate store

1. Generate a certificate in the certificate store
```
New-SelfSignedCertificate -CertStoreLocation Cert:\CurrentUser\My -Type CodeSigningCert -Subject "<CERT_SUBJECT>" -NotAfter (Get-Date).AddYears(10)
```
2. Move the certificate from Personal (My) to local machine Trusted Root Authorities (Root)
3. Sign the executable
```
SignTool sign /v /fd SHA256 /s Root /n "<CERT_SUBJECT>" /t http://timestamp.digicert.com <EXE_FILENAME>
```
