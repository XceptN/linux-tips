**TODO: RENAME EVERYTHING TO GENERIC**

You have a ActiveDirectory server providing LDAP service an you want to use it on Linux. The following basic ldapsearch command works without SSL/TLS:
```
$ ldapsearch -x -S -LLL -H ldap://192.168.2.245  -D "CN=administrator,CN=Users,dc=bisoft,dc=net" -w '1234-Asd' -b "dc=bisoft,dc=net" "(&(objectCategory=User)(memberOf=cn=postgresl,cn=Users,dc=bisoft,dc=net))"
...
```
But it fails with SSL/TLS. Both as:
```
$ ldapsearch -x -S -LLL -H ldaps://192.168.2.245  -D "CN=administrator,CN=Users,dc=bisoft,dc=net" -w '1234-Asd' -b "dc=bisoft,dc=net" "(&(objectCategory=User)(memberOf=cn=postgresl,cn=Users,dc=bisoft,dc=net))"
ldap_sasl_bind(SIMPLE): Can't contact LDAP server (-1)
```
and
```
$ ldapsearch -x -S -LLL -ZZ -H ldap://192.168.2.245  -D "CN=administrator,CN=Users,dc=bisoft,dc=net" -w '1234-Asd' -b "dc=bisoft,dc=net" "(&(objectCategory=User)(memberOf=cn=postgresl,cn=Users,dc=bisoft,dc=net))"
ldap_sasl_bind(SIMPLE): Can't contact LDAP server (-1)
```
Checking the service with OpenSSL:
```
$ openssl s_client -servername  bisoft-dc01.bisoft.net -connect bisoft-dc01.bisoft.net:636
```
...
```
$ openssl s_client -connect 192.168.2.245:636
CONNECTED(00000003)
write:errno=104
---
no peer certificate available
---
No client certificate CA names sent
---
SSL handshake has read 0 bytes and written 289 bytes
Verification: OK
---
New, (NONE), Cipher is (NONE)
Secure Renegotiation IS NOT supported
Compression: NONE
Expansion: NONE
No ALPN negotiated
Early data was not sent
Verify return code: 0 (ok)
---
```
Running `ldapsearch` without requiring certificate works fine:
```
$ env LDAPTLS_REQCERT=never ldapsearch -x -S -LLL  -H ldaps://192.168.2.245  -D "CN=administrator,CN=Users,dc=bisoft,dc=net" -w '1234-Asd' -b "dc=bisoft,dc=net" "(&(objectCategory=User)(memberOf=cn=postgresl,cn=Users,dc=bisoft,dc=net))"
```
At this point we need to get the trusted CA certificate from ActiveDirectory and import it on Linux:

## Export Trusted CA Certificate from Windows/ActiveDirectory
Use MMC on Windows to export the certificate in DER format (*.cer)
1. Run command, type MMC
1. Browse to "Trusted Certificates"
1. On right pane, right click on the desired certificate > Advanced Tasks > Export
1. Select DER format
1. Provide new file name (e.g. ADTrustedCACert)


Maybe instead we can use `openssl -showcerts` to get the server certificate without dealing with Windows.  
```
$ openssl s_client -showcerts -servername bisoft-dc01.bisoft.net -connect 192.168.2.245:636
```
This may list one or more certificates. Copy the certificate (including BEGIN and AND) that you need and create a new *.pem file with it. That way you will not need to do coversion too.

YA DAAA:
```
echo  | openssl s_client -showcerts -connect erisim.bisoft.com.tr:443 2>/dev/null | sed -ne '/-----BEGIN\ CERTIFICATE-----/,/-----END\ CERTIFICATE-----/ p'
```

### Copy certificate to Linux
1. Copy the *.cer file to Linux

## Convert Exported Certificate to PEM
1. Convert file to *.pem using openssl

```
$ sudo openssl x509 -inform der -in frommmc0.cer -out frommmc0.pem
```
## Add converted certificate as trusted to Linux
Add AD certificate file to trusted certificates
### Alma/Rocky/OL/RHEL/CentOS
```
$ sudo cp frommmc0.pem /etc/pki/ca-trust/source/anchors/
$ sudo update-ca-trust
```
You can check if the certificate is added as trusted below:
```
$ sudo head --lines 23 /etc/pki/ca-trust/extracted/openssl/ca-bundle.trust.crt
...
# bisoft-BISOFT-DC01-CA
-----BEGIN TRUSTED CERTIFICATE-----
MIIDdTCCAl2gAwIB...
...
...
Q0E=
-----END TRUSTED CERTIFICATE-----
...
```
### Ubuntu
 
**THIS NEEDS TO BE TESTED IN FULL**

The PEM certificate should have a *.crt name. Rename the file containing the certificate if necessary. 
```
$ sudo apt-get install -y ca-certificates
$ sudo cp xx.crt /usr/local/share/ca-certificates
$ sudo update-ca-certificates
```
Check if the certificate is added to the trusted store:
```
$ ls -ltr /etc/ssl/certs/ | tail --lines 3
lrwxrwxrwx 1 root root     39 Apr  4 11:45 xx.pem -> /usr/local/share/ca-certificates/xx.crt
-rw-r--r-- 1 root root 210344 Apr  4 11:45 ca-certificates.crt
lrwxrwxrwx 1 root root      6 Apr  4 11:45 3f862d16.0 -> xx.pem
```


``` 
----
# Archive
Followed https://www.manageengine.com/products/active-directory-audit/kb/how-to/how-to-install-ssl-certificates-in-active-directory.html to extract the certificate. Got it to Linux. Still fails as below
```
attempting to connect:
connect success
TLS: could not load verify locations (file:`/var/lib/pgsql/ssl/client.crt',dir:`').
TLS: error:0B084088:x509 certificate routines:X509_load_cert_crl_file:no certificate or crl found crypto/x509/by_file.c:223
ldap_err2string
ldap_sasl_bind(SIMPLE): Can't contact LDAP server (-1)
```
Below fails similarly
```
$ openssl s_client -CAfile /var/lib/pgsql/ssl/client.crt 192.168.2.245:636
139999089805120:error:0B084088:x509 certificate routines:X509_load_cert_crl_file:no certificate or crl found:crypto/x509/by_file.c:223:
```
Hmm
```
$ openssl x509 -noout -modulus -in /var/lib/pgsql/ssl/client.crt
unable to load certificate
139647531489088:error:0909006C:PEM routines:get_name:no start line:crypto/pem/pem_lib.c:745:Expecting: TRUSTED CERTIFICATE
```
Following this https://secure.springshosting.net/knowledgebase/28/Exporting-SSL-certificates-from-Windows-to-Linux.html
.. Providing produced crt from pfx:
```
$ env LDAPTLS_CACERT=/etc/pki/ca-trust/source/anchors/frommmc.crt ldapsearch -d1 -x -S -LLL  -H ldaps://192.168.2.245:636  -D "CN=administrator,CN=Users,dc=bisoft,dc=net" -w '1234-Asd' -b "dc=bisoft,dc=net" "(&(objectCategory=User)(memberOf=cn=postgresl,cn=Users,dc=bisoft,dc=net))"
...

attempting to connect:
connect success
TLS: could not load verify locations (file:`/etc/pki/ca-trust/source/anchors/frommmc.crt',dir:`').
TLS: error:0B084088:x509 certificate routines:X509_load_cert_crl_file:no certificate or crl found crypto/x509/by_file.c:223
ldap_err2string
ldap_sasl_bind(SIMPLE): Can't contact LDAP server (-1)
```
Running without requiring certificate works fine:
```
env LDAPTLS_REQCERT=never ldapsearch -x -S -LLL  -H ldaps://192.168.2.245  -D "CN=administrator,CN=Users,dc=bisoft,dc=net" -w '1234-Asd' -b "dc=bisoft,dc=net" "(&(objectCategory=User)(memberOf=cn=postgresl,cn=Users,dc=bisoft,dc=net))"
```
LDAP auth is done as below: 
https://www.postgresql.org/docs/current/auth-ldap.html

PostgreSQL links in ldap.h and LDAP OS libraries and use respective library calls (instead of using actual Linux commands). 

Maybe it is better to do the config as in https://www.percona.com/blog/configuring-postgresql-and-ldap-using-starttls/ Though that is for Ubuntu.. Need to adapt it to CentoS.. Nooo.. PG and LDAP are on the same server.

bisoft-BISOFT-DC01-CA-1

After adding *.pem file to /etc/openldap/certs

```
$ openssl s_client -connect 192.168.2.245:636
CONNECTED(00000003)
Can't use SSL_get_servername
depth=0 CN = bisoft-dc01.bisoft.net
verify error:num=20:unable to get local issuer certificate
verify return:1
depth=0 CN = bisoft-dc01.bisoft.net
verify error:num=21:unable to verify the first certificate
verify return:1
```

It seems we could not produce a proper CA certificate (per https://medium.com/@shmilysyg/how-to-fix-openssl-verify-error-verify-error-unable-to-get-local-issuer-certificate-num-20-b9429fde5ec9)