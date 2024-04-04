

You have a ActiveDirectory server providing LDAP service an you want to use it on Linux. The following basic ldapsearch command works without SSL/TLS:
```
$ ldapsearch -x -S -LLL -H ldap://ActiveDirectory-hostname.mycompany.net -D "CN=administrator,CN=Users,dc=mycompany,dc=net" -w 'mypassword' -b "dc=mycompany,dc=net" "(&(objectCategory=User)(memberOf=cn=postgresl,cn=Users,dc=mycompany,dc=net))"
...
```
**But** it fails with SSL/TLS:

```
$ ldapsearch -x -S -LLL -H ldaps://ActiveDirectory-hostname.mycompany.net -D "CN=administrator,CN=Users,dc=mycompany,dc=net" -w 'mypassword' -b "dc=mycompany,dc=net" "(&(objectCategory=User)(memberOf=cn=postgresl,cn=Users,dc=mycompany,dc=net))"
ldap_sasl_bind(SIMPLE): Can't contact LDAP server (-1)
```
or
```
$ ldapsearch -x -S -LLL -ZZ -H ldap://ActiveDirectory-hostname.mycompany.net -D "CN=administrator,CN=Users,dc=mycompany,dc=net" -w 'mypassword' -b "dc=mycompany,dc=net" "(&(objectCategory=User)(memberOf=cn=postgresl,cn=Users,dc=mycompany,dc=net))"
ldap_sasl_bind(SIMPLE): Can't contact LDAP server (-1)
```
Running `ldapsearch` command in diag mode (**-d1**):

```
$ ldapsearch -d1 -x -S -LLL -H ldaps://ActiveDirectory-hostname.mycompany.net  -D "CN=administrator,CN=Users,dc=mycompany,dc=net" -w 'mypassword' -b "dc=mycompany,dc=net" "(&(objectCategory=User)(memberOf=cn=postgresl,cn=Users,dc=mycompany,dc=net))"
ldap_url_parse_ext(ldaps://ActiveDirectory-hostname.mycompany.net)
ldap_create
ldap_url_parse_ext(ldaps://ActiveDirectory-hostname.mycompany.net:636/??base)
ldap_sasl_bind
ldap_send_initial_request
ldap_new_connection 1 1 0
ldap_int_open_connection
ldap_connect_to_host: TCP ActiveDirectory-hostname.mycompany.net:636
ldap_new_socket: 3
ldap_prepare_socket: 3
ldap_connect_to_host: Trying ActiveDirectory-hostname.mycompany.net:636
ldap_pvt_connect: fd: 3 tm: -1 async: 0
attempting to connect:
connect success
TLS: peer cert untrusted or revoked (0x42)
TLS: can't connect: (unknown error code).
ldap_err2string
ldap_sasl_bind(SIMPLE): Can't contact LDAP server (-1)
```

Checking the service with `openssl` command:

```
$ openssl s_client -servername ActiveDirectory-hostname.mycompany.net -connect ActiveDirectory-hostname.mycompany.net:636
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
or

```
$ openssl s_client -servername ActiveDirectory-hostname.mycompany.net -connect ActiveDirectory-hostname.mycompany.net:636
CONNECTED(00000003)
depth=0 CN = ActiveDirectory-hostname.mycompany.net
verify error:num=20:unable to get local issuer certificate
verify return:1
depth=0 CN = ActiveDirectory-hostname.mycompany.net
verify error:num=21:unable to verify the first certificate
verify return:1
depth=0 CN = ActiveDirectory-hostname.mycompany.net
verify return:1
...
```

Running `ldapsearch` without requiring certificate works fine:

```
$ env LDAPTLS_REQCERT=never ldapsearch -x -S -LLL -H ldaps://ActiveDirectory-hostname.mycompany.net -D "CN=administrator,CN=Users,dc=mycompany,dc=net" -w 'mypassword' -b "dc=mycompany,dc=net" "(&(objectCategory=User)(memberOf=cn=postgresl,cn=Users,dc=mycompany,dc=net))"
```
**IMPORTANT:** SSL/TLS would not work with IP addresses as the certificates are usually generated based on hostname (FQDN)

At this point we need to get the trusted CA certificate from ActiveDirectory and import it on Linux. Follow steps below..

## Export Trusted CA Certificate from Windows / ActiveDirectory
#### Use MMC tool on Windows to export the certificate in DER format (.CER)

1. Click Start -> Windows System -> Run
2. Type `mmc` and click *OK*
3. Under *File* menu, select **C:\Windows\system32\certlm**
4. Expand and select *Certificates - Local Computer* -> *Trusted Root Certification Authorities* -> *Certificates*
5. On the right-hand-side right-click on the certificate to be imported to Linux.
6. Select *All Tasks* -> *Export...*
7. Click *Next* on **Certificate Export Wizard**
8. Select *DER encoded binary X.509 (.CER)*
9. Click *Next*
10. Provide new file name (e.g. ADTrustedCACert)
11. Click *Next*
12. Review selections and click *Finish*

### Copy Certificate to Linux
Copy the (.CER) file to your Linux environment

## Convert Exported Certificate to PEM
Convert file to PEM using `openssl` command:

```
$ openssl x509 -inform der -in ADTrustedCACert.cer -out ADTrustedCACert.pem
```
## Add Converted Certificate as Trusted to Linux
Add AD certificate file to trusted certificates
### Alma/Rocky/OL/RHEL/CentOS
```
$ sudo cp ADTrustedCACert.pem /etc/pki/ca-trust/source/anchors/
$ sudo update-ca-trust
```
You can check if the certificate is added as trusted below:
```
$ sudo head --lines 23 /etc/pki/ca-trust/extracted/openssl/ca-bundle.trust.crt
...
# mycompany-ActiveDirectory-hostname-CA
-----BEGIN TRUSTED CERTIFICATE-----
MIIDdTCCAl2gAwIB...
...
...
Q0E=
-----END TRUSTED CERTIFICATE-----
...
```
### Ubuntu

The PEM certificate should have a (.CRT) name. Rename the file containing the certificate accordingly. 

Then run below commands:

```
$ sudo apt install -y ca-certificates
...
$ sudo cp ADTrustedCACert.crt /usr/local/share/ca-certificates
$ sudo update-ca-certificates
Updating certificates in /etc/ssl/certs...
rehash: warning: skipping ca-certificates.crt,it does not contain exactly one certificate or CRL
1 added, 0 removed; done.
Running hooks in /etc/ca-certificates/update.d...

Replacing debian:ADTrustedCACert.pem
done.
done.

$
```
Check if the certificate is added to the trusted store:
```
$ ls -ltr /etc/ssl/certs/ | tail --lines 3
lrwxrwxrwx 1 root root     45 Apr  4 15:26 ADTrustedCACert.pem -> /usr/local/share/ca-certificates/ADTrustedCACert.crt
-rw-r--r-- 1 root root 211605 Apr  4 15:26 ca-certificates.crt
lrwxrwxrwx 1 root root     12 Apr  4 15:26 81ccb3ef.0 -> ADTrustedCACert.pem
```

## Test the Configuration

Test with `openssl` command:

```
$ openssl s_client -servername ActiveDirectory-hostname.mycompany.net -connect ActiveDirectory-hostname.mycompany.net:636
CONNECTED(00000003)
depth=1 DC = net, DC = mycompany, CN = mycompany-ActiveDirectory-hostname-CA
verify return:1
depth=0 CN = ActiveDirectory-hostname.mycompany.net
verify return:1
---
Certificate chain
 0 s:CN = ActiveDirectory-hostname.mycompany.net
   i:DC = net, DC = mycompany, CN = mycompany-ActiveDirectory-hostname-CA
   a:PKEY: rsaEncryption, 2048 (bit); sigalg: RSA-SHA256
   v:NotBefore: Mar 24 17:11:24 2024 GMT; NotAfter: Mar 24 17:11:24 2025 GMT
---
...
```

Test with `ldapsearch`:

```
ldapsearch -x -S -LLL -H ldaps://ActiveDirectory-hostname.mycompany.net -D "CN=administrator,CN=Users,dc=mycompany,dc=net" -w 'mypassword' -b "dc=mycompany,dc=net" "(&(objectCategory=User)(memberOf=cn=postgresl,cn=Users,dc=mycompany,dc=net))"
# extended LDIF
#
# LDAPv3
# base <dc=mycompany,dc=net> with scope subtree
# filter: (&(objectCategory=User)(memberOf=cn=postgresl,cn=Users,dc=mycompany,dc=net))
# requesting: ALL
#

# mehmet, Users, mycompany.net
dn: CN=mehmet,CN=Users,DC=mycompany,DC=net
objectClass: top
objectClass: person
objectClass: organizationalPerson
objectClass: user
cn: mehmet
...
```

